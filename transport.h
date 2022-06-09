/*
 * Author: Pu-Chen Mao
 * Date:   2018/11/15
 * File:   transport.h
 * Desc:   FLV stream transport interface header
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
    typedef void * flv_transport_context_t;
    typedef flv_transport_context_t (*flv_transport_context_function_t)(
            error_context_t *errctx); // context needs be freed
    typedef bool (*flv_transport_probe_function_t)(const char *url);
    typedef bool (*flv_transport_init_function_t)(flv_transport_context_t ctx,
            const char *url, error_context_t *errctx);
    typedef bool (*flv_transport_connect_function_t)(flv_transport_context_t ctx,
            error_context_t *errctx);
    typedef bool (*flv_transport_recv_function_t)(flv_transport_context_t ctx,
            flvtag_function_t callback, void *userdata, error_context_t *errctx);
    typedef void (*flv_transport_fini_function_t)(flv_transport_context_t ctx);

    /* Transport context definition */
    typedef struct flv_transport_t
    {
        const char                             *name;
        const char                             *desc;
        const flv_transport_context_function_t  context;
        const flv_transport_probe_function_t    probe;
        const flv_transport_init_function_t     init;
        const flv_transport_connect_function_t  connect;
        const flv_transport_recv_function_t     recv;
        const flv_transport_fini_function_t     fini;

    } flv_transport_t;

    /* Global transport registry and registered transport count */
    #define MAX_TRANSPORT_COUNT 16
    extern const flv_transport_t *transport_registry[MAX_TRANSPORT_COUNT];
    extern size_t transport_count;

    /* Returns FLV transport for the given URL */
    const flv_transport_t *flv_transport_class(const char *url);

#ifdef __cplusplus
}
#endif

