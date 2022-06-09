/*
 * Author: Pu-Chen Mao
 * Date:   2018/11/15
 * File:   websocket.c
 * Desc:   FLV stream over WebSocket transport implementation
 */

#include <stdio.h>

#include <cJSON.h>
#include <librtmp/rtmp.h>
#include <libwebsockets.h>

#include "common.h"
#include "error.h"
#include "transport.h"
#include "websocket.h"

static flv_transport_context_t flv_transport_websocket_context(
        error_context_t *errctx);
static bool flv_transport_websocket_probe(const char *url);
static int websocket_event_handler(struct lws *wsi,
        enum lws_callback_reasons reason, void *data, void *in, size_t length);
static bool websocket_traverse_frame(const context_t *wsctx,
        const uint8_t *frame, size_t length, error_context_t *errctx);

static flv_transport_t websocket =
{
    .name    = "websocket",
    .desc    = "FLV-over-WebSocket",
    .context = flv_transport_websocket_context,
    .probe   = flv_transport_websocket_probe,
    .init    = flv_transport_websocket_init,
    .connect = flv_transport_websocket_connect,
    .recv    = flv_transport_websocket_recv,
    .fini    = flv_transport_websocket_fini,
};

REGISTER_TRANSPORT(websocket);

bool
flv_transport_websocket_init(flv_transport_context_t  ctx,
                             const char              *url,
                             error_context_t         *errctx)
{
    context_t *wsctx   = NULL;
    size_t     url_len = 0;
    bool       use_ssl = false;
    bool       result  = false;
    int        ret     = 0;

    /* Sanity checks */
    if (!ctx || !url || !errctx)
        error_save_jump(errctx, EINVAL, CLEANUP);

    /* Cast transport context to internal context */
    wsctx = (context_t *)(ctx);

    /* Allocate buffer and copy URL string */
    url_len = strnlen(url, MAX_STR_LEN);
    wsctx->url = (char *)(calloc(url_len + 1, 1));
    error_save_jump_if(!wsctx->url, errctx, errno, CLEANUP);
    memcpy(wsctx->url, url, url_len);

    /* Parse URL components */
    websocket_parse_url(wsctx->url, &(wsctx->hostname),
            &(wsctx->port), &(wsctx->path));
    if (!wsctx->hostname || !wsctx->port || !wsctx->path)
        error_save_jump(errctx, ENOMEM, CLEANUP);

    /* Determine whether we should enable SSL */
    ret = strncmp(wsctx->url, "wss://", sizeof("wss://") - 1);
    use_ssl = (ret == 0);

    /* Setup WebSocket context info */
    if (use_ssl)
        (wsctx->ctx_info).options = LWS_SERVER_OPTION_DO_SSL_GLOBAL_INIT;
    (wsctx->ctx_info).port = CONTEXT_PORT_NO_LISTEN;
    (wsctx->ctx_info).protocols = wsctx->protocols;

    /* Setup WebSocket context */
    wsctx->lwsctx = lws_create_context(&(wsctx->ctx_info));
    error_save_jump_if(!wsctx->lwsctx, errctx, ENOMEM, CLEANUP);

    /* Setup WebSocket client connection info */
    (wsctx->conn_info).context = wsctx->lwsctx;
    (wsctx->conn_info).port = wsctx->port;
    (wsctx->conn_info).address = wsctx->hostname;
    (wsctx->conn_info).path = wsctx->path;
    (wsctx->conn_info).host = wsctx->hostname;
    (wsctx->conn_info).origin = wsctx->hostname;
    (wsctx->conn_info).protocol = (wsctx->protocols)[0].name;
    (wsctx->conn_info).pwsi = &(wsctx->wsi);
    if (use_ssl) {
        (wsctx->conn_info).ssl_connection = LCCSCF_USE_SSL;
        (wsctx->conn_info).ssl_connection |= LCCSCF_ALLOW_EXPIRED;
        (wsctx->conn_info).ssl_connection |= LCCSCF_ALLOW_SELFSIGNED;
        (wsctx->conn_info).ssl_connection |=
            LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    }

    result = true;

CLEANUP:

    if (!result)
    {
        FREE_AND_NULLIFY(wsctx->path);
        FREE_AND_NULLIFY(wsctx->hostname);
        FREE_AND_NULLIFY(wsctx->url);
        lws_context_destroy(wsctx->lwsctx);
        wsctx->lwsctx = NULL;
    }

    return result;
}

bool
flv_transport_websocket_connect(flv_transport_context_t  ctx,
                                error_context_t         *errctx)
{
    context_t *wsctx = NULL;
    int        ret   = 0;

    /* Sanity checks */
    if (!ctx || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast transport context to internal context */
    wsctx = (context_t *)(ctx);
    error_save_retval_if(!wsctx->url, errctx, EINVAL, false);

    /* Connect WebSocket connection, client is stored in pwsi */
    if (!lws_client_connect_via_info(&(wsctx->conn_info)))
        error_save_retval(errctx, ENOTCONN, false);

    /* Execute event loop until WebSocket state is connected */
    wsctx->errctx = errctx;
    while (ret >= 0 && wsctx->wsi && !wsctx->connected && !wsctx->error)
        ret = lws_service(wsctx->lwsctx, 10);
    error_save_retval_if(ret < 0, errctx, ENOTCONN, false);

    return true;
}

bool
flv_transport_websocket_recv(flv_transport_context_t  ctx,
                             flvtag_function_t        callback,
                             void                    *userdata,
                             error_context_t         *errctx)
{
    context_t *wsctx = NULL;
    int        ret   = 0;

    /* Sanity checks */
    if (!ctx || !callback || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast transport context to internal context */
    wsctx = (context_t *)(ctx);
    error_save_retval_if(!wsctx->url, errctx, EINVAL, false);

    /* Prepare WebSocket context for WebSocket callback invocation */
    wsctx->callback = callback;
    wsctx->userdata = userdata;
    wsctx->errctx = errctx;

    /* Execute one iteration of the WebSocket event loop */
    ret = lws_service(wsctx->lwsctx, 10);
    error_save_retval_if(ret < 0 || wsctx->error, errctx, ENOTCONN, false);

    return true;
}

void flv_transport_websocket_fini(flv_transport_context_t ctx)
{
    context_t *wsctx = NULL;

    /* Cast transport context to internal context */
    wsctx = (context_t *)(ctx);
    if (!wsctx)
        return;

    /* Free allocated resources */
    FREE_AND_NULLIFY(wsctx->path);
    FREE_AND_NULLIFY(wsctx->hostname);
    FREE_AND_NULLIFY(wsctx->url);
    lws_context_destroy(wsctx->lwsctx);
    wsctx->lwsctx = NULL;
}

void
websocket_parse_url(const char  *url,
                    char       **hostname,
                    uint32_t    *port,
                    char       **path)
{
    const char *host_head = NULL;
    const char *host_tail = NULL;
    const char *port_head = NULL;
    const char *port_tail = NULL;
    const char *path_head = NULL;

    /* Sanity checks */
    if (!url || !hostname || !port || !path)
        return;

    /* Find beginning of hostname portion of URL */
    host_head = strstr(url, "://");
    if (!host_head)
        return;
    host_head += sizeof("://") - 1;

    /* Find beginning of port portion of URL */
    port_head = strchr(host_head, ':');
    if (port_head)
    {
        host_tail = port_head;
        port_head++;
    }

    /* Find beginning of path portion of URL */
    path_head = port_head ? strchr(port_head, '/') : strchr(host_head, '/');
    if (path_head)
    {
        if (!port_head)
            host_tail = path_head;
        port_tail = path_head;
    }

    /* Copy portions */
    asprintf(hostname, "%.*s", (int)(host_tail - host_head), host_head);
    asprintf(path, "%s", path_head);

    /* Parse port to int */
    if (port_head)
    {
        char portbuf[6] = {0};
        memcpy(portbuf, port_head, port_tail - port_head);
        *port = strtoul(portbuf, NULL, 10);
    }
    else
    {
        if (strncmp(url, "ws://", sizeof("ws://") - 1) == 0)
            *port = 80;
        else if (strncmp(url, "wss://", sizeof("wss://") - 1) == 0)
            *port = 443;
    }
}

static flv_transport_context_t
flv_transport_websocket_context(error_context_t *errctx)
{
    /* Allocate WebSocket context */
    context_t *wsctx = (context_t *)(calloc(1, sizeof(context_t)));
    error_save_retval_if(!wsctx, errctx, errno, NULL);

    /* Setup WebSocket protocols */
    (wsctx->protocols)[0].name = "";
    (wsctx->protocols)[0].callback = websocket_event_handler;
    (wsctx->protocols)[0].per_session_data_size = sizeof(context_t);
    (wsctx->protocols)[0].rx_buffer_size = 4 * 1024 * 1024;
    (wsctx->protocols)[0].id = 0;
    (wsctx->protocols)[0].user = NULL;
    (wsctx->protocols)[0].tx_packet_size = 0;
    (wsctx->protocols)[0].user = wsctx;

    return (flv_transport_context_t)(wsctx);
}

static bool flv_transport_websocket_probe(const char *url)
{
    const char *dot = NULL;

    /* Sanity checks */
    if (!url)
        return false;

    /* Check if URL begins with WebSocket protocol scheme */
    if (strncmp(url, "ws://", sizeof("ws://") - 1) != 0 &&
        strncmp(url, "wss://", sizeof("wss://") - 1) != 0)
        return false;

    /* Check if URL ends with a .flv extension */
    dot = strrchr(url, '.');
    if (!dot || strncasecmp(dot, ".flv", sizeof(".flv") - 1) != 0)
        return false;

    return true;
}

static int
websocket_event_handler(struct lws                *wsi,
                        enum lws_callback_reasons  reason,
                        void                      *data,
                        void                      *in,
                        size_t                     length)
{
    const struct lws_protocols *protocol = NULL;
    context_t                  *wsctx    = NULL;
    const uint8_t              *frame    = (const uint8_t *)(in);

    /* Sanity checks */
    if (!wsi)
        return 0;

    /* Obtain WebSocket protocol context */
    protocol = lws_get_protocol(wsi);
    if (!protocol)
        return 0;

    /* Obtain WebSocket internal user context */
    wsctx = (context_t *)(protocol->user);
    if (!wsctx)
        return 0;

    /* Handle WebSocket event based on reason */
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            wsctx->connected = true;
        break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (!websocket_traverse_frame(wsctx, frame, length, wsctx->errctx))
                return -1;
            (wsctx->response_count)++;
        break;
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_WSI_DESTROY:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            wsctx->error = true;
            return -1;
        default: break;
    }

    return 0;
}

static bool
websocket_traverse_frame(const context_t *wsctx,
                         const uint8_t   *frame,
                         size_t           length,
                         error_context_t *errctx)
{
    const flv_tag_t *tag = NULL;
    const uint8_t   *end = frame + length;

    /* Sanity checks */
    if (!wsctx || !frame || !wsctx->errctx || !wsctx->callback)
        error_save_retval(errctx, EINVAL, false);

    /* Check whether the frame is an event response or a FLV frame */
    if (*frame == '{' && length < WEBSOCKET_MAX_CONTROL_MESSAGE_LENGTH)
        return true;

    /* Skip FLV header if this is the first frame */
    if (wsctx->response_count == 0)
        frame += sizeof(flv_header_t) + sizeof(uint32_t);

    /* Parse all FLV tags in this WebSocket frame */
    #define NEXT_TAG_ADDRESS() ((const uint8_t *)(tag) + sizeof(flv_tag_t) + \
            ntohu24(tag->length) + sizeof(uint32_t))
    for (tag = (const flv_tag_t *)(frame);
         tag < (const flv_tag_t *)(end);
         tag = (const flv_tag_t *)(NEXT_TAG_ADDRESS()))
    {
        /* Invoke user-provided callback with FLV tag */
        if (!wsctx->callback(tag, wsctx->userdata, wsctx->errctx))
            error_save_retval(wsctx->errctx, errno, false);
    }

    return true;
}

