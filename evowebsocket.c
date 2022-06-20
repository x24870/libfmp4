/*
 * Author: Mave Rick
 * Date:   2022/06/17
 * File:   websocket.c
 * Desc:   FMP4 stream over WebSocket transport implementation
 */

#include <stdio.h>

#include <cJSON.h>
#include <librtmp/rtmp.h>
#include <libwebsockets.h>

#include "common.h"
#include "error.h"
#include "transport.h"
#include "websocket.h"

static fmp4_transport_context_t fmp4_transport_evowebsocket_context(
        error_context_t *errctx);
static bool fmp4_transport_evowebsocket_probe(const char *url);
static int evowebsocket_event_handler(struct lws *wsi,
        enum lws_callback_reasons reason, void *data, void *in, size_t length);
static bool evowebsocket_send_event(const context_t *evowsctx,
        const char *event_type, error_context_t *errctx);
static bool evowebsocket_traverse_frame(const context_t *evowsctx,
        const uint8_t *frame, size_t length, error_context_t *errctx);

static fmp4_transport_t evowebsocket =
{
    .name    = "evowebsocket",
    .desc    = "Reactive FMP4-over-WebSocket",
    .context = fmp4_transport_evowebsocket_context,
    .probe   = fmp4_transport_evowebsocket_probe,
    .init    = fmp4_transport_websocket_init,
    .connect = fmp4_transport_websocket_connect,
    .recv    = fmp4_transport_websocket_recv,
    .fini    = fmp4_transport_websocket_fini,
};

REGISTER_TRANSPORT(evowebsocket);

static fmp4_transport_context_t
fmp4_transport_evowebsocket_context(error_context_t *errctx)
{
    /* Allocate WebSocket context */
    context_t *wsctx = (context_t *)(calloc(1, sizeof(context_t)));
    error_save_retval_if(!wsctx, errctx, errno, NULL);

    /* Setup WebSocket protocols */
    (wsctx->protocols)[0].name = "";
    (wsctx->protocols)[0].callback = evowebsocket_event_handler;
    (wsctx->protocols)[0].per_session_data_size = sizeof(context_t);
    (wsctx->protocols)[0].rx_buffer_size = 4 * 1024 * 1024;
    (wsctx->protocols)[0].id = 0;
    (wsctx->protocols)[0].user = NULL;
    (wsctx->protocols)[0].tx_packet_size = 0;
    (wsctx->protocols)[0].user = wsctx;

    return (fmp4_transport_context_t)(wsctx);
}

static bool fmp4_transport_evowebsocket_probe(const char *url)
{
    const char *last = NULL;

    /* Sanity checks */
    if (!url)
        return false;

    /* Check if URL begins with secure WebSocket protocol scheme */
    if (strncmp(url, "wss://", sizeof("wss://") - 1) != 0)
        return false;

    /* Check if URL ends with 'websocketstream' */
    last = strrchr(url, '/');
    #define WSS "websocketstream"
    if (!last || strncasecmp(last + 1, WSS, sizeof(WSS) - 1) != 0)
        return false;

    return true;
}

static int
evowebsocket_event_handler(struct lws                *wsi,
                           enum lws_callback_reasons  reason,
                           void                      *data,
                           void                      *in,
                           size_t                     length)
{
    const struct lws_protocols *protocol = NULL;
    context_t                  *evowsctx = NULL;
    const uint8_t              *frame    = (const uint8_t *)(in);

    /* Sanity checks */
    if (!wsi)
        return 0;

    /* Obtain WebSocket protocol context */
    protocol = lws_get_protocol(wsi);
    if (!protocol)
        return 0;

    /* Obtain WebSocket internal user context */
    evowsctx = (context_t *)(protocol->user);
    if (!evowsctx)
        return 0;

    /* Handle WebSocket event based on reason */
    switch (reason)
    {
        case LWS_CALLBACK_CLIENT_ESTABLISHED:
            if (!evowebsocket_send_event(evowsctx, "PLAY", evowsctx->errctx))
                return -1;
            (evowsctx->request_count)++;
            evowsctx->connected = true;
        break;
        case LWS_CALLBACK_CLIENT_WRITEABLE:
            if (!evowebsocket_send_event(evowsctx, "PING", evowsctx->errctx))
                return -1;
            (evowsctx->request_count)++;
        break;
        case LWS_CALLBACK_CLIENT_RECEIVE:
            if (!evowebsocket_traverse_frame(evowsctx, frame, length, evowsctx->errctx))
                return -1;
            (evowsctx->response_count)++;
        break;
        case LWS_CALLBACK_CLOSED:
        case LWS_CALLBACK_PROTOCOL_DESTROY:
        case LWS_CALLBACK_WSI_DESTROY:
        case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
            evowsctx->error = true;
            return -1;
        default: break;
    }

    return 0;
}

static bool
evowebsocket_send_event(const context_t *evowsctx,
                        const char      *event_type,
                        error_context_t *errctx)
{
    /* JSON instances & flag */
    const cJSON *node     = NULL;
    cJSON       *root     = NULL;
    uint8_t     *buffer   = NULL;
    char        *json     = NULL;
    size_t       json_len = 0;
    int          ret      = 0;
    bool         result   = false;

    /* Sanity checks */
    if (!evowsctx || !event_type || !errctx || !evowsctx->wsi)
        error_save_jump(errctx, EINVAL, CLEANUP);

    /* Compose & serialize event JSON object */
    root = cJSON_CreateObject();
    error_save_jump_if(!root, errctx, ENOMEM, CLEANUP);
    node = cJSON_AddStringToObject(root, "eventType", event_type);
    error_save_jump_if(!node, errctx, ENOMEM, CLEANUP);
    node = cJSON_AddNumberToObject(root, "requestId", evowsctx->request_count);
    error_save_jump_if(!node, errctx, ENOMEM, CLEANUP);
    node = cJSON_AddNumberToObject(root, "timeStamp", current_time_milliseconds());
    error_save_jump_if(!node, errctx, ENOMEM, CLEANUP);
    json = cJSON_PrintUnformatted(root);
    error_save_jump_if(!json, errctx, ENOMEM, CLEANUP);

    /* Send the JSON event object */
    json_len = strnlen(json, MAX_STR_LEN);
    buffer = (uint8_t *)(calloc(1, LWS_PRE + json_len));
    error_save_jump_if(!buffer, errctx, ENOMEM, CLEANUP);
    memcpy(buffer + LWS_PRE, json, json_len);
    ret = lws_write(evowsctx->wsi, buffer + LWS_PRE, json_len, LWS_WRITE_TEXT);
    error_save_jump_if(ret < 0, errctx, ENOMEM, CLEANUP);

    result = true;

CLEANUP:

    FREE_AND_NULLIFY(buffer);
    FREE_AND_NULLIFY(json);
    cJSON_Delete(root); // null check inside
    root = NULL;

    return result;
}

static bool
evowebsocket_traverse_frame(const context_t *evowsctx,
                            const uint8_t   *frame,
                            size_t           length,
                            error_context_t *errctx)
{
    const fmp4_box_t *box = NULL;
    const uint8_t   *end = frame + length;

    /* Sanity checks */
    if (!evowsctx || !frame || !evowsctx->errctx || !evowsctx->callback)
        error_save_retval(errctx, EINVAL, false);

    /* Check whether the frame is an event response or a FMP4 frame */
    if (*frame == '{' && length < WEBSOCKET_MAX_CONTROL_MESSAGE_LENGTH)
        return true;

    /* Parse all FMP4 boxes in this WebSocket frame */
    #define NEXT_BOX_ADDRESS() ((const uint8_t *)(box) + ntohl(box->size))
    for (box = (const fmp4_box_t *)(frame);
         box < (const fmp4_box_t *)(end);
         box = (const fmp4_box_t *)(NEXT_BOX_ADDRESS()))
    {
        /* Invoke user-provided callback with FMP4 box */
        if (!evowsctx->callback(box, evowsctx->userdata, evowsctx->errctx))
            error_save_retval(evowsctx->errctx, errno, false);
    }

    return true;
}

