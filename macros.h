/*
 * Author:  Pu-Chen Mao
 * Date:    2013/03/20
 * File:    common.h
 * Desc:    Macro definitions
 */

#ifndef _LIBFMP4_MACROS_H_
#define _LIBFMP4_MACROS_H_

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stddef.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C"
{
#endif

    #define MAX_STR_LEN 1024
    #define __gnuc_va_list va_list
    #define likely(x) __builtin_expect(!!(x), 1)
    #define unlikely(x) __builtin_expect(!!(x), 0)
    #define STRINGIFY(x) _STRINGIFY(x)
    #define _STRINGIFY(x) #x
    #define CLEAR_AND_NULLIFY(ptr, dtor) {if (likely(ptr)) {dtor(ptr); ptr = NULL;}}
    #define FREE_AND_NULLIFY(ptr) {if (likely(ptr)) {free(ptr); ptr = NULL;}}
    #define FCLOSE_AND_NULLIFY(ptr) {if (likely(ptr)) {fclose(ptr); ptr = NULL;}}
    #define SET_VAR_JMP_LBL(ret, err, lbl) {ret = err; goto lbl;}
    #define NULL_TERM_STRNCPY(buf, src, buf_len) {strncpy(buf, src, buf_len); buf[buf_len - 1] = '\0';}
    #define SWITCH_IO_STATE(loop, io, fd, state) {ev_io_stop(loop, (ev_io *)(io)); ev_io_set((ev_io *)(io), fd, state); ev_io_start(loop, (ev_io *)(io));}
    #define RETURN_IF_FAIL(x) {if (unlikely(!(x))) {log_error("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return;}}
    #define RETURN_DBG_IF_FAIL(x) {if (unlikely(!(x))) {log_debug("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return;}}
    #define RETURN_TRACE_IF_FAIL(x) {if (unlikely(!(x))) {log_trace("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return;}}
    #define RET_EXEC_IF_FAIL(x, statement) {if (unlikely(!(x))) {statement; return;}}
    #define RET_VAL_IF_FAIL(x, ret) {if (unlikely(!(x))) {log_error("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return ret;}}
    #define RET_VAL_DBG_IF_FAIL(x, ret) {if (unlikely(!(x))) {log_debug("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return ret;}}
    #define RET_VAL_TRACE_IF_FAIL(x, ret) {if (unlikely(!(x))) {log_trace("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return ret;}}
    #define RET_VAL_WARN_IF_FAIL(x, ret) {if (unlikely(!(x))) {log_warning("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); return ret;}}
    #define RET_SET_VAR_IF_FAIL(x, var, val) {if (unlikely(!(x))) {log_error("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); var = val; return;}}
    #define JMP_LBL_IF_FAIL(x, lbl) {if (unlikely(!(x))) {log_error("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); goto lbl;}}
    #define SET_VAR_JMP_LBL_IF_FAIL(x, var, val, lbl) {if (unlikely(!(x))) {log_error("!(%s) @ line %d of %s()\n", #x, __LINE__, __func__); SET_VAR_JMP_LBL(var, val, lbl);}}
    #define ntohu24(x) (ntohl((x) << 8))
    #define htonu64(x) ( \
        ((x & 0xFF00000000000000ULL) >> 56) | \
        ((x & 0x00FF000000000000ULL) >> 40) | \
        ((x & 0x0000FF0000000000ULL) >> 24) | \
        ((x & 0x000000FF00000000ULL) >>  8) | \
        ((x & 0x00000000FF000000ULL) <<  8) | \
        ((x & 0x0000000000FF0000ULL) << 24) | \
        ((x & 0x000000000000FF00ULL) << 40) | \
        ((x & 0x00000000000000FFULL) << 56))

#ifdef __cplusplus
}
#endif

#endif

