/*
 * Author: Pu-Chen Mao
 * Date:   2016/04/07
 * File:   common.h
 * Desc:   Common macros & type definitions
 */

#ifndef _LIBFLV_COMMON_H_
#define _LIBFLV_COMMON_H_

#include <stddef.h>
#include <stdarg.h>
#include <stdint.h>
#include <sys/time.h>
#include <time.h>

#include "macros.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* Return the current system timestamp in milliseconds */
    static inline int64_t current_time_milliseconds()
    {
        struct timeval tv = {};
        gettimeofday(&tv, NULL);
        return tv.tv_sec * 1000LL + tv.tv_usec / 1000LL;
    }

#ifdef __cplusplus
}
#endif

#endif

