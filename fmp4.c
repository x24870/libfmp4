/*
 * Author: Mave Rick
 * Date:   2022/06/17
 * File:   fmp4.c
 * Desc:   Source FMP4 interface header
 */

#include "fmp4.h"
#include "transport.h"

typedef struct fmp4_internal_t
{
    const fmp4_transport_t   *transport;
    fmp4_transport_context_t  context;

} fmp4_internal_t;


fmp4_t fmp4_create(const char *url, error_context_t *errctx)
{
    fmp4_internal_t *fmp4ctx = NULL;
    bool            result = false;

    /* Sanity checks */
    if (!url || !errctx)
        error_save_jump(errctx, EINVAL, CLEANUP);

    /* Setup internal fmp4 context */
    fmp4ctx = (fmp4_internal_t *)(calloc(sizeof(fmp4_internal_t), 1));
    error_save_jump_if(!fmp4ctx, errctx, errno, CLEANUP);

    /* Get transport class for this URL */
    fmp4ctx->transport = fmp4_transport_class(url);
    error_save_jump_if(!fmp4ctx->transport, errctx, EPROTONOSUPPORT, CLEANUP);

    /* Create context for the transport class */
    fmp4ctx->context = fmp4ctx->transport->context(errctx);
    error_save_jump_if(!fmp4ctx->context, errctx, errno, CLEANUP);

    /* Initialize transport context */
    if (!fmp4ctx->transport->init(fmp4ctx->context, url, errctx))
        error_save_jump(errctx, errno, CLEANUP);

    result = true;

CLEANUP:

    if (!result && fmp4ctx)
        FREE_AND_NULLIFY(fmp4ctx->context);
    if (!result)
        FREE_AND_NULLIFY(fmp4ctx);

    return (fmp4_t)(fmp4ctx);
}

bool fmp4_connect(fmp4_t fmp4, error_context_t *errctx)
{
    fmp4_internal_t *fmp4ctx = NULL;

    /* Sanity checks */
    if (!fmp4 || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast to internal fmp4 context */
    fmp4ctx = (fmp4_internal_t *)(fmp4);
    if (!fmp4ctx->transport || !fmp4ctx->context)
        error_save_retval(errctx, EINVAL, false);

    /* Connect to the FMP4 stream source */
    if (!fmp4ctx->transport->connect(fmp4ctx->context, errctx))
        error_save_retval(errctx, errno, false);

    return true;
}

bool
fmp4_recv(fmp4_t              fmp4,
         fmp4box_function_t  callback,
         void              *userdata,
         error_context_t   *errctx)
{
    fmp4_internal_t *fmp4ctx = NULL;

    /* Sanity checks */
    if (!fmp4 || !callback || !errctx)
        error_save_retval(errctx, EINVAL, false);

    /* Cast to internal FMP4 context */
    fmp4ctx = (fmp4_internal_t *)(fmp4);
    if (!fmp4ctx->transport || !fmp4ctx->context)
        error_save_retval(errctx, EINVAL, false);

    /* Connect to the FMP4 stream source */
    if (!fmp4ctx->transport->recv(fmp4ctx->context, callback, userdata, errctx))
        error_save_retval(errctx, errno, false);

    return true;
}

void fmp4_destroy(fmp4_t *fmp4)
{
    fmp4_internal_t *fmp4ctx = NULL;

    /* Sanity checks */
    if (!fmp4 || !*fmp4)
        return;

    /* Cast to internal FMP4 context */
    fmp4ctx = (fmp4_internal_t *)(*fmp4);

    /* Deinitialize transport context */
    fmp4ctx->transport->fini(fmp4ctx->context);

    /* Free & clear allocated resources */
    FREE_AND_NULLIFY(fmp4ctx->context);
    FREE_AND_NULLIFY(*fmp4);
}

