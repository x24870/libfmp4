/*
 * Author: Pu-Chen Mao
 * Date:   2018/11/15
 * File:   flv.h
 * Desc:   Source FLV interface header
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

    typedef struct flv_header_t
    {
        char     signature[3];
        uint8_t  version;
        uint8_t  flags;
        uint32_t header_size;

    } __attribute__ ((__packed__)) flv_header_t;

    typedef struct flv_tag_t
    {
        uint32_t type:            8;
        uint32_t length:          24;
        uint32_t timestamp_lower: 24;
        uint32_t timestamp_upper: 8;
        uint32_t stream_id:       24;
        uint8_t  payload[];

    } __attribute__ ((__packed__)) flv_tag_t;

    typedef struct flv_file_tag_t
    {
        uint32_t offset:          32; /* relative offset from previous tag */
        uint32_t type:            8;  /* 8: audio, 9: video, 18: script data */
        uint32_t length:          24;
        uint32_t timestamp_lower: 24;
        uint32_t timestamp_upper: 8;
        uint32_t stream_id:       24;
        uint8_t  payload[];

    } __attribute__ ((__packed__)) flv_file_tag_t;

    typedef struct flv_audio_tag_t
    {
        uint8_t sound_type:   1; /* mono: 0, stereo: 1 */
        uint8_t sound_size:   1; /* 8-bit: 0, 16-bit: 1 */
        uint8_t sound_rate:   2; /* 5.5 KHz: 0, 11 KHz: 1, 22 KHz: 2, 44 KHz: 3 */
        uint8_t sound_format: 4; /* LPCM-BE: 0, MP3: 2, LPCM-LE: 3, G.711 A-LAW: 7, G.711 u-LAW: 8, AAC: 10 */
        uint8_t packet_type;     /* AAC Sequence Header: 0, AAC Raw: 1 */
        uint8_t audio_frame[];

    } __attribute__ ((__packed__)) flv_audio_tag_t;

    typedef struct flv_video_tag_t
    {
        uint8_t  codec_id:         4;  /* 0x07 for H.264 */
        uint8_t  frame_type:       4;  /* intra-frame: 0x01, non-intra-frame: 0x02 */
        uint32_t packet_type:      8;  /* config parameter sets: 0x00, picture data: 0x01 */
        uint32_t composition_time: 24; /* for B-frames only, 0 for mainprofile */
        uint8_t  video_frame[];

    } __attribute__ ((__packed__)) flv_video_tag_t;

    typedef struct h264_config_t
    {
        uint8_t  version;
        uint8_t  profile_idc;
        uint8_t  profile_compatibility; /* constraint set 0~3 + reserved 4 bits */
        uint8_t  level_idc;
        uint8_t  length_type;
        uint8_t  sps_count;
        uint16_t sps_len;
        uint8_t  sps[];
        // uint8_t  pps_count;
        // uint16_t pps_len;
        // uint8_t  pps[];

    } __attribute__ ((__packed__)) h264_config_t;

    typedef struct h264_sps_t
    {
        uint8_t nalu_header;
        uint8_t profile_idc;
        uint8_t profile_compatibility; /* constraint set 0~3 + reserved 4 bits */
        uint8_t level_idc;
        uint8_t remaining[];

    } __attribute__ ((__packed__)) h264_sps_t;

    typedef struct audio_specific_config_t
    {
        uint16_t ga_specific_config:       3;
        uint16_t channel_configuration:    4;
        uint16_t sampling_frequency_index: 4;
        uint16_t audio_object_type:        5;

    } __attribute__ ((__packed__)) audio_specific_config_t;

    /* FLV stream context object */
    typedef void * flv_t;

    /* Callback for FLV tags */
    typedef bool (*flvtag_function_t)(const flv_tag_t *tag, void *userdata,
            error_context_t *errctx);

    /* FLV public functions */
    flv_t flv_create(const char *url, error_context_t *errctx);
    bool flv_connect(flv_t flv, error_context_t *errctx);
    bool flv_recv(flv_t flv, flvtag_function_t callback, void *userdata,
            error_context_t *errctx);
    void flv_destroy(flv_t *flv);
    uint64_t flv_parse_wallclock(const uint8_t *payload, size_t length,
            error_context_t *errctx);

#ifdef __cplusplus
}
#endif

