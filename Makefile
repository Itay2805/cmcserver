########################################################################################################################
# Build config
########################################################################################################################

#
# Enable debug stuff
#
DEBUG ?= 0

########################################################################################################################
# Build constants
########################################################################################################################

CC 		:= gcc

CFLAGS 	:= -Wall -Werror -Wno-format
CFLAGS 	+= -O2 -flto -g

LDFLAGS := $(CFLAGS)
LDFLAGS += -luring

ifeq ($(DEBUG), 1)
	BIN_DIR := out/bin/debug
	BUILD_DIR := out/build/debug

	CFLAGS += -D_DEBUG
	CFLAGS += -D__DEBUG__
	CFLAGS += -fsanitize=address
	CFLAGS += -fsanitize=undefined

	LDFLAGS += -lubsan
else
	BIN_DIR := out/bin/release
	BUILD_DIR := out/build/release

	CFLAGS += -DNDEBUG
endif

CFLAGS 	+= -Isrc -I$(BUILD_DIR)

# sources
SRCS := $(shell find src -name '*.c')
SRCS += $(BUILD_DIR)/minecraft_protodef.c

########################################################################################################################
# Phony
########################################################################################################################

.PHONY: all clean

all: $(BIN_DIR)/server.elf

# Generate the packet parser automatically
$(BUILD_DIR)/minecraft_protodef.c: scripts/protodef.py artifacts/protocol.json
	@mkdir -p $(@D)
	python3 ./scripts/protodef.py artifacts/protocol.json $(BUILD_DIR)/minecraft_protodef.h $(BUILD_DIR)/minecraft_protodef.c

########################################################################################################################
# Targets
########################################################################################################################

OBJS := $(SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:%.o=%.d)

-include $(DEPS)

$(BIN_DIR)/server.elf: $(OBJS)
	@echo LD $@
	@mkdir -p $(@D)
	@$(CC) $(OBJS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.c.o: %.c | $(BUILD_DIR)/minecraft_protodef.c
	@echo CC $@
	@mkdir -p $(@D)
	@$(CC) $(CFLAGS) -MMD -c $< -o $@

clean:
	rm -rf out
