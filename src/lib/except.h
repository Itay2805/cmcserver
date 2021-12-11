#pragma once

#include <stdint.h>
#include <stdio.h>
#include <errno.h>

#include "defs.h"

typedef enum err {
    /**
     * No error
     */
    NO_ERROR = 0,

    /**
     * A check has failed
     */
    ERROR_CHECK_FAILED = 1,

    /**
     * An error at the protocol level, usually related
     * to configurations of the server or invalid data
     * being sent over the protocol by the client
     */
    ERROR_PROTOCOL,
} err_t;

/**
 * Initialize printf handler for err_t
 */
void init_err_printf();

#define IS_ERROR(x) ((x) != NO_ERROR)

#define TRACE(fmt, ...) printf("[*] " fmt "\n", ## __VA_ARGS__)
#define WARN(fmt, ...) printf("[!] " fmt "\n", ## __VA_ARGS__)
#define ERROR(fmt, ...) printf("[-] " fmt "\n", ## __VA_ARGS__)

//----------------------------------------------------------------------------------------------------------------------
// A normal check
//----------------------------------------------------------------------------------------------------------------------

#define CHECK_ERROR_LABEL(check, error, label, ...) \
    do { \
        if (!(check)) { \
            err = error; \
            IF(HAS_ARGS(__VA_ARGS__))(ERROR(__VA_ARGS__)); \
            ERROR("Check failed with error %R in function %s (%s:%d)", err, __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_ERROR(check, error, ...)              CHECK_ERROR_LABEL(check, error, cleanup, ## __VA_ARGS__)
#define CHECK_LABEL(check, label, ...)              CHECK_ERROR_LABEL(check, ERROR_CHECK_FAILED, label, ## __VA_ARGS__)
#define CHECK(check, ...)                           CHECK_ERROR_LABEL(check, ERROR_CHECK_FAILED, cleanup, ## __VA_ARGS__)

//----------------------------------------------------------------------------------------------------------------------
// A quite check
//----------------------------------------------------------------------------------------------------------------------

#define CHECK_QUIET_ERROR_LABEL(check, error, label) \
    do { \
        if (!(check)) { \
            err = error; \
            goto label; \
        } \
    } while(0)

#define CHECK_QUIET_ERROR(check, error)              CHECK_ERROR_LABEL(check, error, cleanup)
#define CHECK_QUIET_LABEL(check, label)              CHECK_ERROR_LABEL(check, ERROR_CHECK_FAILED, label)
#define CHECK_QUIET(check)                           CHECK_ERROR_LABEL(check, ERROR_CHECK_FAILED, cleanup)

//----------------------------------------------------------------------------------------------------------------------
// A check that uses errno as error value
//----------------------------------------------------------------------------------------------------------------------

#define CHECK_ERRNO_LABEL(check, label, ...) \
    do { \
        if (!(check)) { \
            err = -errno; \
            IF(HAS_ARGS(__VA_ARGS__))(ERROR(__VA_ARGS__)); \
            ERROR("Check failed with error %R in function %s (%s:%d)", err, __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_ERRNO(check, ...)                           CHECK_ERRNO_LABEL(check, cleanup, ## __VA_ARGS__)

//----------------------------------------------------------------------------------------------------------------------
// A check that fails without a condition
//----------------------------------------------------------------------------------------------------------------------

#define CHECK_FAIL(...)                             CHECK_ERROR_LABEL(0, ERROR_CHECK_FAILED, cleanup, ## __VA_ARGS__)
#define CHECK_FAIL_ERROR(error, ...)                CHECK_ERROR_LABEL(0, error, cleanup, ## __VA_ARGS__)
#define CHECK_FAIL_LABEL(label, ...)                CHECK_ERROR_LABEL(0, ERROR_CHECK_FAILED, label, ## __VA_ARGS__)
#define CHECK_FAIL_ERROR_LABEL(error, label, ...)   CHECK_ERROR_LABEL(0, error, label, ## __VA_ARGS__)

//----------------------------------------------------------------------------------------------------------------------
// A check that fails if an error was returned, used around functions returning an error
//----------------------------------------------------------------------------------------------------------------------

#define CHECK_AND_RETHROW_LABEL(error, label) \
    do { \
        err = error; \
        if (IS_ERROR(err)) { \
            ERROR("\trethrown at %s (%s:%d)", __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_AND_RETHROW(error) CHECK_AND_RETHROW_LABEL(error, cleanup)
