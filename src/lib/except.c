#include "except.h"

#include <stdio.h>
#include <string.h>
#include <printf.h>

static const char* m_err_names[] = {
    [NO_ERROR] = "NO_ERROR",
    [ERROR_CHECK_FAILED] = "ERROR_CHECK_FAILED",
    [ERROR_PROTOCOL] = "ERROR_PROTOCOL",
};

int print_err(FILE* stream, const struct printf_info* info, const void* const* args) {
    const int err = *((int*)args[0]);

    if (err < 0) {
        return fprintf(stream, "%*s", (info->left ? -info->wide : info->wide), strerror(-err));
    } else {
        if (err < ARRAY_LEN(m_err_names)) {
            return fprintf(stream, "%*s", (info->left ? -info->wide : info->wide), m_err_names[err]);
        } else {
            return fprintf(stream, "%*d", (info->left ? -info->wide : info->wide), err);
        }
    }
}

int print_err_arginfo_size_function(const struct printf_info* info, size_t n, int* argtypes, int* size) {
    if (n > 0)
    {
        argtypes[0] = PA_INT;
        size[0] = sizeof (int);
    }
    return 1;
}

void init_err_printf() {
    register_printf_specifier('R', print_err, print_err_arginfo_size_function);
}
