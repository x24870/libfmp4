/*
 * Author: Pu-Chen Mao
 * Date:   2018/11/15
 * File:   flv.c
 * Desc:   Source FLV interface header
 */

#include "fmp4.h"
#include "transport.h"

typedef struct flv_internal_t
{
    const flv_transport_t   *transport;
    flv_transport_context_t  context;

} flv_internal_t;


flv_t flv_create(const char *url, error_context_t *errctx)
{
    flv_internal_t *flvctx = NULL;
    bool            result = false;

    /* Sanity checks */
    if (!url || !errctx)
        error_save_jump(errctx, EINVAL, CLEANUP);

    /* Setup internal FLV context */
    flvctx = (flv_internal_t *)(calloc(sizeof(flv_internal_t), 1));
    error_save_jump_if(!flvctx, errctx, errno, CLEANUP);

    /* Get transport class for this URL */
    flvctx->transport = flv_transport_class(url);
    error_save_jump_if(!flvctx->transport, errctx, EPROTONOSUPPORT, CLEANUP);

    /* Create context for the transport class */
    flvctx->context = flvctx->transport->context(errctx);
    error_save_jump_if(!flvctx->context, errctx, errno, CLEANUP);

    /* Initialize transport context */
    if (!flvctx->transport->init(flvctx->context, url, errctx))
        error_save_jump(errctx, errno, CLEANUP);

    result = true;

CLEANUP:

    if (!result && flvctx)
        FREE_AND_NULLIFY(flvctx->context);
    if (!result)
        FREE_AND_NULLIFY(flvctx);

    return (flv_t)(flvctx);
}

bool flv_connect(flv_t flv, error_context_t *errctx)
{
    flv_internal_t *flvctx = NULL;

    /* Sanity checks */
    if (!flv || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast to internal FLV context */
    flvctx = (flv_internal_t *)(flv);
    if (!flvctx->transport || !flvctx->context)
        error_save_retval(errctx, EINVAL, false);

    /* Connect to the FLV stream source */
    if (!flvctx->transport->connect(flvctx->context, errctx))
        error_save_retval(errctx, errno, false);

    return true;
}

bool
flv_recv(flv_t              flv,
         flvtag_function_t  callback,
         void              *userdata,
         error_context_t   *errctx)
{
    flv_internal_t *flvctx = NULL;

    /* Sanity checks */
    if (!flv || !callback || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast to internal FLV context */
    flvctx = (flv_internal_t *)(flv);
    if (!flvctx->transport || !flvctx->context)
        error_save_retval(errctx, EINVAL, false);

    /* Connect to the FLV stream source */
    if (!flvctx->transport->recv(flvctx->context, callback, userdata, errctx))
        error_save_retval(errctx, errno, false);

    return true;
}

void flv_destroy(flv_t *flv)
{
    flv_internal_t *flvctx = NULL;

    /* Sanity checks */
    if (!flv || !*flv)
        return;

    /* Cast to internal FLV context */
    flvctx = (flv_internal_t *)(*flv);

    /* Deinitialize transport context */
    flvctx->transport->fini(flvctx->context);

    /* Free & clear allocated resources */
    FREE_AND_NULLIFY(flvctx->context);
    FREE_AND_NULLIFY(*flv);
}

uint64_t
flv_parse_wallclock(const uint8_t   *payload,
                    size_t           length,
                    error_context_t *errctx)
{
    const uint8_t *ptr = payload;
    uint64_t       val = 0;
    size_t         len = 0;
    size_t         idx = 0;

    /* Sanity checks */
    if (!payload || !errctx)
        error_save_retval(errctx, EINVAL, 0);

    /* Skip string marker + byte length */
    ptr += 1 + sizeof(uint16_t);

    /* Check if this is indeed an onTextData tag */
    if (memcmp(ptr, "onTextData", sizeof("onTextData") - 1))
        return 0;

    /* Find start of object members */
    ptr += sizeof("onTextData") - 1;
    ptr += sizeof(uint8_t);
    ptr += sizeof(uint32_t);

    #define PARSE_TYPE_MEMBER() \
        do { \
            ptr += sizeof(uint16_t); \
            ptr += sizeof("type") - 1; \
            ptr += sizeof(uint8_t); \
            ptr += sizeof(uint16_t); \
            ptr += sizeof("Text") - 1; \
        } while (false)

    #define PARSE_TEXT_MEMBER() \
        do { \
            ptr += sizeof(uint16_t); \
            ptr += sizeof("text") - 1; \
            ptr += sizeof(uint8_t); \
            if (ptr - payload > length) \
                error_save_retval(errctx, EBADMSG, 0); \
            len = ntohs(*(const uint16_t *)(ptr)); \
            ptr += sizeof(uint16_t); \
            if (ptr - payload + len > length) \
                error_save_retval(errctx, EBADMSG, 0); \
            for (idx = 0; idx < len; idx++) \
                val = 10 * val + (ptr[idx] - '0'); \
        } while (false)

    /* Parse object members "type" and "text", note we only expect these 2 */
    if ((ptr + sizeof(uint16_t)) + 4 - payload > length)
        error_save_retval(errctx, EBADMSG, 0);
    if (memcmp(ptr + sizeof(uint16_t), "type", sizeof("type") - 1) == 0) {
        PARSE_TYPE_MEMBER();
        PARSE_TEXT_MEMBER();
    } else if (memcmp(ptr + sizeof(uint16_t), "text", sizeof("text") - 1) == 0) {
        PARSE_TEXT_MEMBER();
        PARSE_TYPE_MEMBER();
    } else error_save_retval(errctx, EBADMSG, 0);

    return val;
}

