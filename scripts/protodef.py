import argparse
import json
import os
import random
import re
import sys
from typing import *
from copy import deepcopy


class ProtoDefBase:

    def __init__(self, size: int):
        self._size = size
        self._parent: ProtoDefBase = None

    def get_parent(self):
        return self._parent

    def is_variable(self) -> bool:
        return self._size <= 0

    def get_size(self) -> int:
        return self._size

    def gen_read_code(self, destination: str) -> str:
        raise NotImplementedError()

    def gen_write_code(self, source: str) -> str:
        raise NotImplementedError()

    def gen_definition_code(self) -> str:
        raise NotImplementedError()

    def find_field(self, name: str) -> Tuple[str, object]:
        assert False, f"Could not find field {name}"


class ProtoDefTypedef(ProtoDefBase):

    def __init__(self, name: str, typ: ProtoDefBase):
        super(ProtoDefTypedef, self).__init__(typ.get_size())
        self._name = name
        self._type = typ

    def get_base_type(self):
        return self._type

    def get_name(self):
        return self._name

    def gen_read_code(self, destination: str) -> str:
        if self.is_variable():
            return f'read_size = protocol_read_{self._name}(arena, data, size, &{destination});' \
                   f'if (read_size < 0) return -1;' \
                   f'data += read_size;' \
                   f'size -= read_size;'
        else:
            return f'{destination} = protocol_read_{self._name}(data);' \
                   f'data += {self._size};'

    def gen_write_code(self, source: str) -> str:
        if self.is_variable():
            return f'write_size = protocol_write_{self._name}(data, size, &{source});' \
                   f'if (write_size < 0) return -1;' \
                   f'data += write_size;' \
                   f'size -= write_size;'
        else:
            return f'protocol_write_{self._name}(data, &{source});' \
                   f'data += {self._size};'

    def gen_definition_code(self) -> str:
        return f'{self._name}_t'

    def __repr__(self):
        return f'ProtoDefTypedef(name={repr(self._name)}, type={repr(self._type)})'


class ProtoDefNative(ProtoDefBase):

    def __init__(self, name: str, ctype: str, size: int):
        super(ProtoDefNative, self).__init__(size)
        self._name = name
        self._ctype = ctype

    def get_name(self):
        return self._name

    def gen_read_code(self, destination: str) -> str:
        if self.is_variable():
            return f'read_size = protocol_read_{self._name}(data, size, &{destination});' \
                   f'if (read_size < 0) return -1;' \
                   f'data += read_size;' \
                   f'size -= read_size;'
        else:
            return f'{destination} = protocol_read_{self._name}(data);' \
                   f'data += {self._size};'

    def gen_write_code(self, source: str) -> str:
        if self.is_variable():
            return f'write_size = protocol_write_{self._name}(data, size, {source});' \
                   f'if (write_size < 0) return -1;' \
                   f'data += write_size;' \
                   f'size -= write_size;'
        else:
            return f'protocol_write_{self._name}(data, {source});' \
                   f'data += {self._size};'

    def gen_definition_code(self) -> str:
        return self._ctype

    def __repr__(self):
        return f'ProtoDefNative(name={repr(self._name)}, ctype={repr(self._ctype)}, size={repr(self._size)})'


def get_parent_field(name: str):
    if '.' in name:
        return '.'.join(name.split('.')[:-1])
    elif '->' in name:
        return name.split('->')[0]
    else:
        return name


def access_field(source: str, name: str):
    if name == '':
        return source

    if '->' in source or '[' in source or '*' in source:
        return source + '.' + name
    else:
        return source + '->' + name


class ProtoDefSwitchDefault:
    pass


class ProtoDefSkipFunctionDef(Exception):
    pass


class ProtoDefVoid(ProtoDefBase):

    def __init__(self):
        super(ProtoDefVoid, self).__init__(0)


def get_c_value_for_type(value: object, typ: ProtoDefBase):
    if isinstance(value, ProtoDefSwitchDefault):
        return 'default'
    elif isinstance(typ, ProtoDefNative):
        if typ.get_name() in ['u8', 'u16', 'i8', 'i16', 'i32', 'i64', 'varint', 'varlong']:
            return str(int(value, 0))
        elif typ.get_name() == 'bool':
            if value == 'true':
                return 'true'
            elif value == 'false':
                return 'false'
            else:
                assert False, f"Invalid value `{value}` for boolean"
        else:
            assert False, f"Invalid native type `{typ.get_name()}` for switch"
    else:
        assert False, f"Invalid type `{typ}` for switch"


class ProtoDefSwitch(ProtoDefBase):

    def __init__(self):
        super(ProtoDefSwitch, self).__init__(0)
        self._name = None
        self._fields: List[Tuple[str, ProtoDefBase, object]] = []
        self._compare_to_name: str = None
        self._is_abstract = False

    def add_field(self, name: str, typ: ProtoDefBase, value: object):
        if typ.is_variable():
            self._size = -1
        if not self.is_variable():
            if self._size < typ.get_size():
                self._size = typ.get_size()
        typ._parent = self
        self._fields.append((name, typ, value))

    def set_compare_to(self, name):
        assert self._compare_to_name is None, "Already set compare to"
        self._compare_to_name = name
        if name == '$compare_to':
            self._is_abstract = True

    def create_instance(self, name, compare_to):
        assert self.is_abstract(), "Must be abstract to create instance"
        typ = deepcopy(self)
        typ._name = name
        typ._compare_to_name = compare_to
        return typ

    def is_abstract(self):
        return self._is_abstract

    def is_abstract_instance(self):
        return self.is_abstract() and self._compare_to_name != '$compare_to'

    def gen_read_code(self, destination: str) -> str:
        assert self._compare_to_name is not None, "Missing compare to field"

        if self.is_abstract():
            if not self.is_abstract_instance():
                raise ProtoDefSkipFunctionDef()

        compare_to_name, compare_to_typ = self.get_parent().find_field(self._compare_to_name)

        if isinstance(compare_to_typ, ProtoDefTypedef) and compare_to_typ.get_name():
            default_name: str = None
            default_typ: ProtoDefBase = None
            c = f''
            for field in self._fields:
                name, typ, value = field
                if isinstance(value, ProtoDefSwitchDefault):
                    default_typ = typ
                    default_name = name
                    continue
                c += f'if ({access_field(get_parent_field(destination), compare_to_name)}.length == {len(value)} && strncmp("{value}", {access_field(get_parent_field(destination), compare_to_name)}.elements, {len(value)}) == 0) '
                c += '{'
                if not isinstance(typ, ProtoDefVoid):
                    if not typ.is_variable():
                        c += f'if (size < {typ.get_size()}) return -1;'
                        c += f'size -= {typ.get_size()};'
                    c += typ.gen_read_code(access_field(destination, name))
                c += '}\n'
                c += 'else '
            c += '{'
            if default_name is not None and default_typ is not None:
                if not default_typ.is_variable():
                    c += f'if (size < {default_typ.get_size()}) return -1;'
                    c += f'size -= {default_typ.get_size()};'
                c += default_typ.gen_read_code(access_field(destination, default_name))
            else:
                c += 'return -1;'
            c += '}\n'

        else:
            has_default = False
            c = f'switch ({access_field(get_parent_field(destination), compare_to_name)}) {{'
            for field in self._fields:
                name, typ, value = field
                case = get_c_value_for_type(value, compare_to_typ)
                if case == 'default':
                    has_default = True
                    c += f'{case}:'
                else:
                    c += f'case {case}:'
                c += '{'
                if not isinstance(typ, ProtoDefVoid):
                    if not typ.is_variable():
                        c += f'if (size < {typ.get_size()}) return -1;'
                        c += f'size -= {typ.get_size()};'
                    c += typ.gen_read_code(access_field(destination, name))
                c += '} break;'

            if not has_default:
                c += 'default: {' \
                     'return -1;' \
                     '} break;'

            c += '}\n'

        return c

    def gen_write_code(self, source: str) -> str:
        assert self._compare_to_name is not None, "Missing compare to field"

        if self.is_abstract():
            if not self.is_abstract_instance():
                raise ProtoDefSkipFunctionDef()

        compare_to_name, compare_to_typ = self.get_parent().find_field(self._compare_to_name)

        if isinstance(compare_to_typ, ProtoDefTypedef) and compare_to_typ.get_name():
            default_name: str = None
            default_typ: ProtoDefBase = None
            c = f''
            for field in self._fields:
                name, typ, value = field
                if isinstance(value, ProtoDefSwitchDefault):
                    default_typ = typ
                    default_name = name
                    continue
                c += f'if ({access_field(get_parent_field(source), compare_to_name)}.length == {len(value)} && strncmp("{value}", {access_field(get_parent_field(source), compare_to_name)}.elements, {len(value)}) == 0) '
                c += '{'
                if not isinstance(typ, ProtoDefVoid):
                    if not typ.is_variable():
                        c += f'if (size < {typ.get_size()}) return -1;'
                        c += f'size -= {typ.get_size()};'
                    c += typ.gen_write_code(access_field(source, name))
                c += '}\n'
                c += 'else '
            c += '{'
            if default_name is not None and default_typ is not None:
                if not default_typ.is_variable():
                    c += f'if (size < {default_typ.get_size()}) return -1;'
                    c += f'size -= {default_typ.get_size()};'
                c += default_typ.gen_write_code(access_field(source, default_name))
            else:
                c += 'return -1;'
            c += '}\n'

        else:
            has_default = False
            c = f'switch ({access_field(get_parent_field(source), compare_to_name)}) {{'
            for field in self._fields:
                name, typ, value = field
                case = get_c_value_for_type(value, compare_to_typ)
                if case == 'default':
                    has_default = True
                    c += f'{case}:'
                else:
                    c += f'case {case}:'
                c += '{'
                if not isinstance(typ, ProtoDefVoid):
                    if not typ.is_variable():
                        c += f'if (size < {typ.get_size()}) return -1;'
                        c += f'size -= {typ.get_size()};'
                    c += typ.gen_write_code(access_field(source, name))
                c += '} break;'

            if not has_default:
                c += 'default: {' \
                     'return -1;' \
                     '} break;'

            c += '}\n'

        return c

    def gen_definition_code(self) -> str:
        if self.is_abstract_instance():
            return self._name
        else:
            c = 'union {'
            for field in self._fields:
                name, typ, value = field
                if isinstance(typ, ProtoDefVoid):
                    continue
                c += f'{typ.gen_definition_code()} {name};'
            c += '}'
            return c


def get_fixed_fields(fields: List[Tuple[str, ProtoDefBase]]) -> List[Tuple[str, ProtoDefBase]]:
    """
    Get a list of variables that are of fixed length
    """
    fixed_fields = []
    for field in fields:
        _, typ = field
        if typ.is_variable():
            break
        fixed_fields.append(field)
    return fixed_fields


class ProtoDefContainer(ProtoDefBase):

    def __init__(self):
        super(ProtoDefContainer, self).__init__(0)
        self._fields: List[Tuple[str, ProtoDefBase]] = []

    def add_field(self, name: str, typ: ProtoDefBase):
        if typ.is_variable():
            self._size = -1
        if not self.is_variable():
            self._size += typ.get_size()
        typ._parent = self
        self._fields.append((name, typ))

    def gen_read_code(self, destination: str) -> str:
        c = ''

        field_offset = 0
        while field_offset < len(self._fields):
            # Combine all the fixed fields in single length check and do
            # the reads
            fixed_fields = get_fixed_fields(self._fields[field_offset:])
            if len(fixed_fields) > 0:
                field_offset += len(fixed_fields)
                total_size = sum([field[1].get_size() for field in fixed_fields])
                assert total_size > 0
                c += f'if (size < {total_size}) return -1;'
                for field in fixed_fields:
                    name, typ = field
                    c += typ.gen_read_code(access_field(destination, name))
                c += f'size -= {total_size};'

            # now generate a variable read
            if field_offset < len(self._fields):
                name, typ = self._fields[field_offset]
                field_offset += 1
                c += typ.gen_read_code(access_field(destination, name))

        return c

    def gen_write_code(self, source: str) -> str:
        c = ''
        field_offset = 0
        while field_offset < len(self._fields):
            # Combine all the fixed fields in single length check and do
            # the reads
            fixed_fields = get_fixed_fields(self._fields[field_offset:])
            if len(fixed_fields) > 0:
                field_offset += len(fixed_fields)
                total_size = sum([field[1].get_size() for field in fixed_fields])
                assert total_size > 0
                # Check we have enough space for this
                c += f'if (size < {total_size}) return -1;'
                for field in fixed_fields:
                    name, typ = field
                    c += typ.gen_write_code(access_field(source, name))
                c += f'size -= {total_size};'

            # now generate a variable read
            if field_offset < len(self._fields):
                name, typ = self._fields[field_offset]
                field_offset += 1
                c += typ.gen_write_code(access_field(source, name))

        return c

    def find_field(self, name: str):
        if name.startswith('../'):
            name = name[len('../'):]
            return self.get_parent().find_field(name)
        else:
            for field in self._fields:
                if field[0] == name:
                    return field
        assert False, f"Could not find field {name}"

    def gen_definition_code(self) -> str:
        c = ''
        c += 'struct {'
        for field in self._fields:
            name, typ = field
            if isinstance(typ, ProtoDefVoid):
                continue
            c += f'{typ.gen_definition_code()} {name};'
        c += '}'
        return c


class ProtoDefArray(ProtoDefBase):

    def __init__(self, count_type: ProtoDefBase, element_type: ProtoDefBase):
        super(ProtoDefArray, self).__init__(-1)
        self._count_type = count_type
        self._element_type = element_type

    def gen_read_code(self, destination: str) -> str:
        c = ''
        c += '{'

        length_access = access_field(destination, "length")
        elements_access = access_field(destination, "elements")

        if self._count_type is not None:
            if not self._count_type.is_variable():
                c += f'if ({self._count_type.get_size()} > size) return -1;'
                c += f'size -= {self._count_type.get_size()};'
            c += self._count_type.gen_read_code(length_access)
        else:
            # This is a rest data, it goes all the way to the end
            # of the packet unconditionally
            c += f'{length_access} = size / sizeof({self._element_type.get_size()});'
            c += f'size -= {length_access};'

        if self._count_type is not None:
            if not self._element_type.is_variable():
                c += f'if ({length_access} * {self._element_type.get_size()} > size) return -1;'
                c += f'size -= {length_access} * {self._element_type.get_size()};'

        c += f'{elements_access} = packet_arena_alloc_unlocked(arena, {length_access} * sizeof(*{elements_access}));'
        c += f'if ({elements_access} == NULL) return -1;'
        i = 'i' + str(random.randint(0, 1000))
        c += f'for (int {i} = 0; {i} < {length_access}; {i}++)'
        c += '{'
        c += self._element_type.gen_read_code(f'{elements_access}[{i}]')
        c += '};'

        c += '};'
        return c

    def gen_write_code(self, source: str) -> str:
        c = ''
        c += '{'

        length_access = access_field(source, "length")
        elements_access = access_field(source, "elements")

        if self._count_type is not None:
            if not self._count_type.is_variable():
                c += f'if ({self._count_type.get_size()} > size) return -1;'
                c += f'size -= {self._count_type.get_size()};'
            c += self._count_type.gen_write_code(length_access)

        if not self._element_type.is_variable():
            c += f'if ({length_access} * {self._element_type.get_size()} > size) return -1;'
            c += f'size -= {length_access} * {self._element_type.get_size()};'

        i = 'i' + str(random.randint(0, 1000))
        c += f'for (int {i} = 0; {i} < {length_access}; {i}++)'
        c += '{'
        c += self._element_type.gen_write_code(f'{elements_access}[{i}]')
        c += '};'
        c += '};'
        return c

    def gen_definition_code(self) -> str:
        # return f'{self._element_type.gen_definition_code()}*'
        c = ''
        c += 'struct {'
        if self._count_type is None:
            c += f'int length;'
        else:
            c += f'{self._count_type.gen_definition_code()} length;'
        c += f'{self._element_type.gen_definition_code()}* elements;'
        c += '}'
        return c

    def __repr__(self):
        return f'ProtoDefArray(count_type={repr(self._count_type)}, element_type={repr(self._element_type)})'


VALID_BITS = {8, 16, 32, 64}


def get_bytes(bits):
    assert bits <= 64, "Too many bits in bitfield"
    while bits not in VALID_BITS:
        bits += 1
    return bits // 8


class ProtoDefBitfield(ProtoDefBase):

    def __init__(self):
        super(ProtoDefBitfield, self).__init__(0)
        self._fields: List[Tuple[str, int, bool]] = []
        self._total_bits = 0

    def add_field(self, name, size, signed):
        self._fields.append((name, size, signed))
        self._total_bits += size
        self._size = get_bytes(self._total_bits)

    def gen_read_code(self, destination: str) -> str:
        c = '{'
        c += f'uint{self.get_size() * 8}_t packed_value = protocol_read_u{self.get_size() * 8}(data);'
        c += f'data += {self.get_size()};'
        offset = 0
        for field in self._fields:
            name, size, _ = field
            c += access_field(destination, name) + f' = (packed_value >> {offset}) & {hex((1 << size) - 1)};'
            offset += size
        c += '};'
        return c

    def gen_write_code(self, source: str) -> str:
        c = '{'
        c += f'uint{self.get_size() * 8}_t packed_value = 0;'
        offset = 0
        for field in self._fields:
            name, size, _ = field
            c += f'packed_value |= ((uint{self.get_size() * 8}_t){access_field(source, name)} & {hex((1 << size) - 1)}) << {offset};'
            offset += size
        c += f'protocol_write_u{self.get_size() * 8}(data, packed_value);'
        c += f'data += {self.get_size()};'
        c += '};'
        return c

    def gen_definition_code(self) -> str:
        c = 'struct {'
        for field in self._fields:
            name, size, signed = field
            c += f'{"" if signed else "u"}int{get_bytes(size) * 8}_t {name};'
        c += '}'
        return c


class ProtoDefOption(ProtoDefBase):

    def __init__(self, typ: ProtoDefBase):
        super(ProtoDefOption, self).__init__(-1)
        self._type = typ

    def gen_read_code(self, destination: str) -> str:
        c = ''
        # Insert the present boolean
        c += 'if (size < 1) return -1;'
        c += 'size--;'
        c += f'{access_field(destination, "present")} = protocol_read_u8(data);'
        c += 'data++;'

        # Now insert the field if needed
        c += f'if ({access_field(destination, "present")})'
        c += ' {'
        if not self._type.is_variable():
            c += f'if (size < {self._type.get_size()}) return -1;'
            c += f'size -= {self._type.get_size()};'
        self._type.gen_read_code(access_field(destination, 'value'))
        c += '}'
        return c

    def gen_write_code(self, source: str) -> str:
        c = ''
        # Insert the present boolean
        c += 'if (size < 1) return -1;'
        c += 'size--;'
        c += f'protocol_write_u8(data, {access_field(source, "present")});'
        c += 'data++;'

        # Now insert the field if needed
        c += f'if ({access_field(source, "present")})'
        c += ' {'
        if not self._type.is_variable():
            c += f'if (size < {self._type.get_size()}) return -1;'
            c += f'size -= {self._type.get_size()};'
        self._type.gen_write_code(access_field(source, 'value'))
        c += '}'
        return c

    def gen_definition_code(self) -> str:
        c = 'struct {'
        c += 'bool present;'
        c += f'{self._type.gen_definition_code()} value;'
        c += '}'
        return c


TYPES = {
    'char': ProtoDefNative('i8', 'char', 1),
    'u8': ProtoDefNative('u8', 'uint8_t', 1),
    'u16': ProtoDefNative('u16', 'uint16_t', 2),
    'i8': ProtoDefNative('i8', 'int8_t', 1),
    'i16': ProtoDefNative('i16', 'int16_t', 2),
    'i32': ProtoDefNative('i32', 'int32_t', 4),
    'i64': ProtoDefNative('i64', 'int64_t', 8),
    'f32': ProtoDefNative('f32', 'float', 4),
    'f64': ProtoDefNative('f64', 'double', 8),
    'bool': ProtoDefNative('bool', 'bool', 1),
    'UUID': ProtoDefNative('uuid', 'uuid_t', 16),
    'varint': ProtoDefNative('varint', 'int32_t', -1),
    'varlong': ProtoDefNative('varlong', 'int64_t', -1),
    'void': ProtoDefVoid(),
    'nbt': ProtoDefNative('nbt', 'nbt_t', -1),
    'optionalNbt': ProtoDefNative('nbt', 'nbt_t', -1),
    'restBuffer': ProtoDefArray(None, ProtoDefNative('u8', 'uint8_t', 1))
}


class ProtoDefTypeNotFoundException(Exception):

    def __init__(self, name):
        self.name = name


def get_type(name):
    if name not in TYPES:
        raise ProtoDefTypeNotFoundException(name)
    return deepcopy(TYPES[name])


SNAKE_CASE_PATTERN = re.compile(r'(?<!^)(?=[A-Z])')


def snake_case(name):
    return SNAKE_CASE_PATTERN.sub('_', name).lower()


def beautify(s):
    fs = ''

    depth = 0
    for c in s:
        if c == '{':
            depth += 1
            fs += c
            fs += '\n'
            fs += '    ' * depth
        elif c == '}':
            depth -= 1
            fs = fs[:-4]
            fs += c
        elif c == ';':
            fs += c
            fs += '\n'
            fs += '    ' * depth
        elif c == '\n':
            fs += c
            fs += '    ' * depth
        else:
            fs += c

    return fs


def generate_definition(name: str, typ: ProtoDefBase):
    c = 'typedef '
    c += typ.gen_definition_code()
    c += ' '
    c += name
    c += '_t'
    c += ';'
    return beautify(c)


def generate_write(name: str, typ: ProtoDefBase):
    try:
        if isinstance(typ, ProtoDefSwitch) or isinstance(typ, ProtoDefContainer) or isinstance(typ, ProtoDefBitfield):
            source = 'packet'
        else:
            source = '(*packet)'

        if typ.is_variable():
            h = f'int protocol_write_{name}(uint8_t* data, int size, {name}_t* packet);'

            c = f'int protocol_write_{name}(uint8_t* data, int size, {name}_t* packet)'
            c += ' {'
            c += 'int original_size = size;'
            c += 'int write_size;'
        else:
            h = f'void protocol_write_{name}(uint8_t* data, {name}_t* packet);'

            c = f'void protocol_write_{name}(uint8_t* data, {name}_t* packet)'
            c += ' {'

        c += typ.gen_write_code(source)

        if typ.is_variable():
            c += 'return original_size - size;'

        c += '}'

        return beautify(c), h
    except ProtoDefSkipFunctionDef:
        return '', ''


def generate_read(name: str, typ: ProtoDefBase):
    try:
        if typ.is_variable():
            if isinstance(typ, ProtoDefSwitch) or isinstance(typ, ProtoDefContainer) or isinstance(typ, ProtoDefBitfield):
                source = 'packet'
            else:
                source = '(*packet)'

            h = f'int protocol_read_{name}(packet_arena_t* arena, uint8_t* data, int size, {name}_t* packet);'

            c = f'int protocol_read_{name}(packet_arena_t* arena, uint8_t* data, int size, {name}_t* packet)'
            c += ' {'
            c += '(void)arena;'
            c += 'int original_size = size;'
            c += 'int read_size = 0;'
            c += '(void)read_size;'
            c += typ.gen_read_code(source)
            c += 'return original_size - size;'
        else:
            h = f'{name}_t protocol_read_{name}(uint8_t* data);'

            c = f'{name}_t protocol_read_{name}(uint8_t* data)'
            c += ' {'
            c += f'{name}_t packet;'
            c += typ.gen_read_code('(&packet)')
            c += 'return packet;'
        c += '}'

        return beautify(c), h
    except ProtoDefSkipFunctionDef:
        return '', ''


def generate_type(typ, switch_names={}):
    if isinstance(typ, list):
        kind, subtyp = typ

        # Handle the container type
        if kind == 'container':
            container = ProtoDefContainer()
            for field in subtyp:
                # Handle anonymous items
                if 'anon' in field and field['anon']:
                    name = ''
                else:
                    name = field['name']
                typ = generate_type(field['type'])
                container.add_field(snake_case(name), typ)
            return container

        # handle the switch type
        elif kind == 'switch':
            switch = ProtoDefSwitch()

            switch.set_compare_to(snake_case(subtyp['compareTo']))

            # TODO: UGLY HACK I HATE THIS PROTOCOL FORMAT AAAAAAAA
            if subtyp['compareTo'] == 'positionType':
                switch_names = {
                    'block': 'block_position',
                    'entity': 'entity_id',
                }

            for value in subtyp['fields']:
                if value in switch_names:
                    name = switch_names[value]
                else:
                    name = ''

                typ = generate_type(subtyp['fields'][value])
                switch.add_field(snake_case(name), typ, value)

            # Add the default field
            if 'default' in subtyp:
                switch.add_field('', generate_type(subtyp['default']), ProtoDefSwitchDefault())

            return switch

        elif kind == 'pstring':
            count_type = generate_type(subtyp['countType'])
            return ProtoDefArray(count_type, get_type('char'))

        elif kind == 'array':
            count_type = generate_type(subtyp['countType'])
            typ = generate_type(subtyp['type'])
            return ProtoDefArray(count_type, typ)

        elif kind == 'buffer':
            count_type = generate_type(subtyp['countType'])
            return ProtoDefArray(count_type, get_type('u8'))

        elif kind == 'bitfield':
            bitfield = ProtoDefBitfield()
            for field in subtyp:
                name = field['name']
                size = field['size']
                signed = field['signed']
                bitfield.add_field(snake_case(name), size, signed)
            return bitfield

        elif kind == 'option':
            return ProtoDefOption(generate_type(subtyp))

        # These are special, we have a general typedef for them, but we need to
        # instantiate them per new type
        elif kind in ['particleData']:
            typedef = get_type(snake_case(kind))
            assert isinstance(typedef, ProtoDefTypedef)
            typ = typedef.get_base_type()
            assert isinstance(typ, ProtoDefSwitch)
            return typ.create_instance(typedef.get_name() + '_t', snake_case(subtyp['compareTo']))

        elif kind == 'entityMetadataLoop':
            return ProtoDefNative('entity_metadata_loop', 'void*', -1)

        else:
            assert False, f"Unknown kind {kind} ({typ})"
    else:
        return get_type(typ)


def generate_base_types(protocol_types):
    code = []
    header = []

    types = []
    for name in protocol_types:
        typ = protocol_types[name]
        types.append((typ, snake_case(name)))

    seen = {}

    while len(types) != 0:
        typ, name = types.pop(0)
        if typ == 'native' or name == 'entity_metadata':
            continue

        if name not in seen:
            seen[name] = 0
        else:
            seen[name] += 1
            if seen[name] > 2:
                print(f'Cant compile type {name}')
                continue

        # Try to get the type, if not found then queue
        # it again and wait until we can queue it
        try:
            # Figure names for special types, this is a weakness of protodef because
            # it should contain that information by itself...
            switch_names = {}
            if name == 'entity_metadata_item':
                switch_names = {
                    '0': 'byte',
                    '1': 'varint',
                    '2': 'float_value',
                    '3': 'string',
                    '4': 'chat',
                    '5': 'opt_chat',
                    '6': 'slot',
                    '7': 'boolean',
                    '8': 'rotation',
                    '9': 'position',
                    '10': 'opt_position',
                    '11': 'direction',
                    '12': 'opt_uuid',
                    '13': 'opt_block_id',
                    '14': 'nbt',
                    '15': 'particle',
                    '16': 'villager_data',
                    '17': 'opt_varint',
                    '18': 'pose',
                }
            elif name == 'particle_data':
                switch_names = {
                    '2': 'block',
                    '3': 'block_marker',
                    '14': 'dust',
                    '15': 'dust_color_transition',
                    '24': 'falling_dust',
                    '35': 'item',
                    '36': 'vibration',
                }

            typ = generate_type(typ, switch_names)
        except ProtoDefTypeNotFoundException as e:
            # print(f'Missing type definitions for {e.name}, (required by {name})')
            types.append((typ, name))
            continue

        TYPES[name] = ProtoDefTypedef(name, typ)
        header.append(generate_definition(name, typ).strip())

        c, h = generate_read(name, typ)
        code.append(c.strip())
        header.append(h.strip())

        c, h = generate_write(name, typ)
        code.append(c.strip())
        header.append(h.strip())

    return code, header


def generate_packet_parser(protocol, phase, code, header):
    packets = protocol[phase]['toServer']['types']
    for packet_name in packets:
        if packet_name == 'packet':
            continue

        if packet_name in {'packet_legacy_server_list_ping'}:
            continue

        name = phase + '_' + packet_name
        typ = generate_type(packets[packet_name])

        header.append(generate_definition(name, typ).strip())
        c, h = generate_read(name, typ)
        code.append(c.strip())
        header.append(h.strip())

        header.append(f'err_t process_{name}(client_t* client, {name}_t* packet);')

        c = ''
        c += f'err_t dispatch_{name}(client_t* client, uint8_t* data, int size)'
        c += '{'
        c += 'err_t err = NO_ERROR;'
        c += f'{name}_t packet = {{ 0 }};'
        if typ.is_variable():
            c += 'packet_arena_t* arena = get_packet_arena();'
            c += f'int read_size = protocol_read_{name}(arena, data, size, &packet);'
            c += f'CHECK_ERROR(read_size == size, ERROR_PROTOCOL, "Got a packet that is bigger than expected!");'
        else:
            c += f'packet = protocol_read_{name}(data, size);'
        c += '\n'
        c += f'CHECK_AND_RETHROW(process_{name}(client, &packet));'
        c += '\n'
        c += 'cleanup:\n'
        if typ.is_variable():
            c += 'return_packet_arena(arena);'
        c += 'return err;'
        c += '}'
        code.append(beautify(c))

    return code, header


def generate_dispatcher(protocol, code, header):
    generate_packet_parser(protocol, 'handshaking', code, header)
    generate_packet_parser(protocol, 'status', code, header)
    generate_packet_parser(protocol, 'login', code, header)
    # generate_packet_parser(protocol, 'play', code, header)

    c = ''
    c += 'err_t dispatch_packet(client_t* client, uint8_t* data, int size) {'
    c += 'err_t err = NO_ERROR;'
    c += '\n'
    c += 'int packet_id = 0;'
    c += 'int read_size = protocol_read_varint(data, size, &packet_id);'
    c += 'CHECK_ERROR(read_size >= 0, ERROR_PROTOCOL, "Packet id was not a valid varint");'
    c += 'data += read_size;'
    c += 'size -= read_size;'
    c += '\n'
    c += 'switch (client->state) {'
    for phase in [
        'handshaking',
        'status',
        'login',
        # 'play'
    ]:
        mapper = protocol[phase]['toServer']['types']['packet'][1][0]['type'][1]['mappings']
        c += f'case PROTOCOL_{phase.upper()}: {{'
        c += 'switch (packet_id) {'
        for id in mapper:
            if mapper[id] in {'legacy_server_list_ping'}:
                continue
            c += f'case {id}: {{CHECK_AND_RETHROW(dispatch_{phase}_packet_{mapper[id]}(client, data, size));}} break;'
        c += 'default: CHECK_FAIL_ERROR(ERROR_PROTOCOL, "Got unknown packet id: %d", packet_id);'
        c += '}\n'
        c += '} break;'
    c += 'default: CHECK_FAIL("Got to invalid state: %d", client->state);'
    c += '}\n'
    c += 'cleanup:\n'
    c += 'return err;'
    c += '}'

    code.append(beautify(c))


if len(sys.argv) != 4:
    print(f'Usage: {sys.argv[0]} <protocol json> <header file> <source file>')
    exit(-1)

path, header_path, code_path = sys.argv[1:]
protocol = json.load(open(path))

code, header = generate_base_types(protocol['types'])
generate_dispatcher(protocol, code, header)

header = '\n\n'.join(list(filter(None, header)))
code = '\n\n'.join(list(filter(None, code)))

h = '#pragma once\n'
h += '\n'
h += '// This file is automatically generated by protodef.py\n'
h += '// Do not modify this file -- YOUR CHANGES WILL BE ERASED!\n'
h += '\n'
h += '#include <minecraft/protocol/packet_arena.h>\n'
h += '#include <minecraft/protocol/protocol.h>\n'
h += '#include <minecraft/protocol/nbt.h>\n'
h += '\n'
h += '#include <lib/except.h>\n'
h += '#include <net/client.h>\n'
h += '\n'
h += '#include <stdbool.h>\n'
h += '#include <stdint.h>\n'
h += '\n'
h += 'err_t dispatch_packet(client_t* client, uint8_t* data, int size);\n'
h += '\n'
h += header + '\n'

c = '#include "minecraft_protodef.h"\n'
c += '\n'
c += '// This file is automatically generated by protodef.py\n'
c += '// Do not modify this file -- YOUR CHANGES WILL BE ERASED!\n'
c += '\n'
c += '// For simplified code gen\n'
c += '#pragma GCC diagnostic ignored "-Wswitch-bool"\n'
c += '\n'
c += '#include <string.h>\n'
c += '\n'
c += code + '\n'
c += '\n'

open(header_path, 'w').write(h)
open(code_path, 'w').write(c)
