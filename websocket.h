/*
 * Author: Pu-Chen Mao
 * Date:   2018/11/15
 * File:   websocket.h
 * Desc:   FMP4 stream over WebSocket transport public shared function headers
 */

#pragma once

#include "common.h"
#include "error.h"
#include "fmp4.h"
#include "transport.h"

#ifdef __cplusplus
extern "C"
{
#endif

    #define WEBSOCKET_MAX_CONTROL_MESSAGE_LENGTH 1024

    /* Internal WebSocket transport context */
    typedef struct context_t
    {
        /* libwebsocket context */
        struct lws_context_creation_info  ctx_info;
        struct lws_client_connect_info    conn_info;
        struct lws_protocols              protocols[2];
        struct lws_context               *lwsctx;
        struct lws                       *wsi;

        /* User callback & context */
        fmp4box_function_t  callback;
        void              *userdata;
        error_context_t   *errctx;

        /* WebSocket stream context */
        uint32_t request_count;
        uint32_t response_count;
        uint32_t ping_count;
        bool     connected;
        bool     error;

        /* URL context */
        char     *url;
        char     *hostname;
        char     *path;
        uint32_t  port;

    } context_t;

    /* Public exported functions */
    bool fmp4_transport_websocket_init(fmp4_transport_context_t  ctx,
            const char *url, error_context_t *errctx);
    bool fmp4_transport_websocket_connect(fmp4_transport_context_t ctx,
            error_context_t *errctx);
    bool fmp4_transport_websocket_recv(fmp4_transport_context_t ctx,
            fmp4box_function_t callback, void *userdata, error_context_t *errctx);
    void fmp4_transport_websocket_fini(fmp4_transport_context_t ctx);
    void websocket_parse_url(const char *url, char **hostname,
            uint32_t *port, char **path);

#ifdef __cplusplus
}
#endif

