/*
 * Author: Mave Rick
 * Date:   2022/06/20
 * File:   error.h
 * Desc:   Error handling helper macros
 */

#ifndef _LIBFMP4_ERROR_H_
#define _LIBFMP4_ERROR_H_

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "common.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Macros to store error context */
    #define error_save(errctx, _errnum) \
        do \
        { \
            if (likely((errctx) && !(errctx)->saved)) \
            { \
                (errctx)->saved = true; \
                (errctx)->file = strrchr(__FILE__, '/'); \
                (errctx)->file = ((errctx)->file ? \
                    ((errctx)->file + 1) : __FILE__); \
                (errctx)->line = __LINE__; \
                (errctx)->errnum = _errnum; \
            } \
        } \
        while (0)

    #define error_clear(errctx) memset(errctx, 0, sizeof(error_context_t))

    #define error_save_return(errctx, errnum) \
        do \
        { \
            error_save((errctx), errnum); \
            return; \
        } \
        while (0)

    #define error_save_retval(errctx, errnum, retval) \
        do \
        { \
            error_save((errctx), errnum); \
            return retval; \
        } \
        while (0)

    #define error_save_jump(errctx, errnum, label) \
        do \
        { \
            error_save((errctx), errnum); \
            goto label; \
        } \
        while (0)

    #define error_save_break(errctx, errnum) \
        { \
            error_save((errctx), errnum); \
            break; \
        }

    #define error_save_return_if(condition, errctx, errnum) \
        if (condition) { \
            error_save_return((errctx), errnum); \
        }

    #define error_save_retval_if(condition, errctx, errnum, retval) \
        if (condition) { \
            error_save_retval((errctx), errnum, retval); \
        }

    #define error_save_jump_if(condition, errctx, errnum, label) \
        if (condition) { \
            error_save_jump((errctx), errnum, label); \
        }

    #define error_save_break_if(condition, errctx, errnum) \
        if (condition) { \
            error_save_break((errctx), errnum); \
        }

    #define error_log_saved(errctx, ...) \
        if ((errctx) && (errctx)->saved) { \
            int ret = fprintf(stderr, "%s", __VA_ARGS__""); \
            fprintf(stderr, "%s: [%s] (%s:%d)\n", \
                ret > 0 ? ", error" : "Error", \
                strerror((errctx)->errnum), \
                (errctx)->file, (errctx)->line); \
            memset((errctx), 0, sizeof(error_context_t)); \
        }

    typedef struct error_context_t
    {
        bool        saved;
        const char *file;
        int         line;
        int         errnum;

    } error_context_t;

#ifdef __cplusplus
}
#endif

#endif

