/*
 * Author: Mave Rick
 * Date:   2022/06/17
 * File:   transport.h
 * Desc:   FMP4 stream transport interface header
 */

#pragma once

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "common.h"
#include "error.h"
#include "fmp4.h"

#ifdef __cplusplus
extern "C"
{
#endif

    #define REGISTER_TRANSPORT(transport) \
        __attribute__((constructor)) static void register_##transport() \
        { \
            assert(transport_count < MAX_TRANSPORT_COUNT); \
            assert(transport_registry[transport_count] == NULL); \
            assert(transport.name != NULL); \
            assert(transport.desc != NULL); \
            assert(transport.context != NULL); \
            assert(transport.probe != NULL); \
            assert(transport.init != NULL); \
            assert(transport.connect != NULL); \
            assert(transport.recv != NULL); \
            assert(transport.fini != NULL); \
            transport_registry[transport_count++] = &transport; \
        }

    /* Transport-specific implementation function pointers types */
    typedef void * fmp4_transport_context_t;
    typedef fmp4_transport_context_t (*fmp4_transport_context_function_t)(
            error_context_t *errctx); // context needs be freed
    typedef bool (*fmp4_transport_probe_function_t)(const char *url);
    typedef bool (*fmp4_transport_init_function_t)(fmp4_transport_context_t ctx,
            const char *url, error_context_t *errctx);
    typedef bool (*fmp4_transport_connect_function_t)(fmp4_transport_context_t ctx,
            error_context_t *errctx);
    typedef bool (*fmp4_transport_recv_function_t)(fmp4_transport_context_t ctx,
            fmp4box_function_t callback, void *userdata, error_context_t *errctx);
    typedef void (*fmp4_transport_fini_function_t)(fmp4_transport_context_t ctx);

    /* Transport context definition */
    typedef struct fmp4_transport_t
    {
        const char                             *name;
        const char                             *desc;
        const fmp4_transport_context_function_t  context;
        const fmp4_transport_probe_function_t    probe;
        const fmp4_transport_init_function_t     init;
        const fmp4_transport_connect_function_t  connect;
        const fmp4_transport_recv_function_t     recv;
        const fmp4_transport_fini_function_t     fini;

    } fmp4_transport_t;

    /* Global transport registry and registered transport count */
    #define MAX_TRANSPORT_COUNT 16
    extern const fmp4_transport_t *transport_registry[MAX_TRANSPORT_COUNT];
    extern size_t transport_count;

    /* Returns FMP4 transport for the given URL */
    const fmp4_transport_t *fmp4_transport_class(const char *url);

#ifdef __cplusplus
}
#endif

