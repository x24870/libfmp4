/*
 * Author: Mave Rick
 * Date:   2022/06/17
 * File:   transport.c
 * Desc:   FMP4 stream transport interface implementation
 */

#include "transport.h"

/* Global transport registry and registered transport count */
const fmp4_transport_t *transport_registry[MAX_TRANSPORT_COUNT] = {};
size_t transport_count = 0;

const fmp4_transport_t *fmp4_transport_class(const char *url)
{
    size_t idx = 0;

    /* Sanity checks */
    if (!url)
        return NULL;

    for (idx = 0; idx < transport_count; idx++)
    {
        if (!transport_registry[idx] || !transport_registry[idx]->probe)
            continue;
        if (transport_registry[idx]->probe(url))
            return transport_registry[idx];
    }

    return NULL;
}

