/*
 * Author: Mave Rick
 * Date:   2022/06/17
 * File:   fmp4.h
 * Desc:   Source FMP4 interface header
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "common.h"
#include "error.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct fmp4_box_t
    {
        uint32_t size: 32;
        uint32_t type: 32;
        uint8_t  body[];
    } __attribute__ ((__packed__)) fmp4_box_t;

    typedef struct fmp4_full_box_t
    {
        uint32_t size:    32;
        uint32_t type:    32;
        uint32_t version:  8;
        uint32_t flags:   24;
        uint8_t  body[];
    } __attribute__ ((__packed__)) fmp4_full_box_t;

    typedef struct fmp4_large_box_t
    {
        uint32_t size:      32;
        uint32_t type:      32;
        uint64_t largeSize: 64;
        uint8_t  body[];
    } __attribute__ ((__packed__)) fmp4_large_box_t;

        typedef struct fmp4_large_full_box_t
    {
        uint32_t size:      32;
        uint32_t type:      32;
        uint64_t largeSize: 64;
        uint32_t version:    8;
        uint32_t flags:     24;
        uint8_t  body[];
    } __attribute__ ((__packed__)) fmp4_large_full_box_t;

    /* FMP4 stream context object */
    typedef void * fmp4_t;

    /* Callback for FMP4 boxes */
    typedef bool (*fmp4box_function_t)(const fmp4_box_t *box, void *userdata,
            error_context_t *errctx);

    /* FMP4 public functions */
    fmp4_t fmp4_create(const char *url, error_context_t *errctx);
    bool fmp4_connect(fmp4_t fmp4, error_context_t *errctx);
    bool fmp4_recv(fmp4_t fmp4, fmp4box_function_t callback, void *userdata,
            error_context_t *errctx);
    void fmp4_destroy(fmp4_t *fmp4);
    uint64_t fmp4_parse_wallclock(const uint8_t *body, size_t size,
            error_context_t *errctx);

#ifdef __cplusplus
}
#endif

