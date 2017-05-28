/**
 * Copyright (c) 2014 Microsoft Open Technologies, Inc.
 *
 *    Licensed under the Apache License, Version 2.0 (the "License"); you may
 *    not use this file except in compliance with the License. You may obtain
 *    a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 *    THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 *    CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 *    LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 *    FOR A PARTICULAR PURPOSE, MERCHANTABILITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 *    Microsoft would like to thank the following companies for their review and
 *    assistance with these files: Intel Corporation, Mellanox Technologies Ltd,
 *    Dell Products, L.P., Facebook, Inc
 *
 * @file    saiserialize.h
 *
 * @brief   This module defines SAI Serialize methods
 */

#ifndef __SAISERIALIZE_H_
#define __SAISERIALIZE_H_

/**
 * @defgroup SAISERIALIZE SAI - Serialize Definitions
 *
 * @{
 */

/**
 * @def SAI_SERIALIZE_ERROR
 *
 * Returned from serialize/deserialize methods on any error.
 */
#define SAI_SERIALIZE_ERROR (-1)

/**
 * @brief Serialize object ID.
 *
 * @param[out] buffer Output buffer for serialized value.
 * @param[in] object_id Object ID to be serialized.
 *
 * @return Number of characters written to buffer excluding '\0',
 * or #SAI_SERIALIZE_ERROR on error.
 */
int sai_serialize_object_id(
        _Out_ char *buffer,
        _In_ sai_object_id_t object_id);

int sai_serialize_object_type(
        _Out_ char *buffer,
        _In_ sai_object_type_t object_type);

int sai_serialize_mac(
        _Out_ char *buffer,
        _In_ const sai_mac_t mac);

int sai_serialize_enum(
        _Out_ char *buffer,
        _In_ const sai_enum_metadata_t *meta,
        _In_ int32_t value);

int sai_serialize_ipv4(
        _Out_ char *buffer,
        _In_ const sai_ip4_t ip);

int sai_serialize_ipv6(
        _Out_ char *buffer,
        _In_ const sai_ip6_t ip);

int sai_serialize_ip_address(
        _Out_ char *buffer,
        _In_ const sai_ip_address_t *ip_address);

int sai_serialize_ip_prefix(
        _Out_ char *buffer,
        _In_ const sai_ip_prefix_t *ip_prefix);

int sai_serialize_ipv4_mask(
        _Out_ char *buffer,
        _In_ sai_ip4_t mask);

int sai_serialize_ipv6_mask(
        _Out_ char *buffer,
        _In_ const sai_ip6_t mask);

/* deserialize functions */

int sai_deserialize_u8(
        _In_ const char *buffer,
        _Out_ uint8_t *u8);

int sai_deserialize_u16(
        _In_ const char *buffer,
        _Out_ uint16_t *u16);

int sai_deserialize_u32(
        _In_ const char *buffer,
        _Out_ uint32_t *u32);

int sai_deserialize_u64(
        _In_ const char *buffer,
        _Out_ uint64_t *u64);

/**
 * @brief Deserialize signed 8 bit integer.
 *
 * @note Buffer don't need to end with zero character, it can end at any
 * non-digit character and function will still succeed. This is helpful when
 * parsing larger combined data.
 *
 * @param[in] buffer Buffer to be examined.
 * @param[out] s8 Deserialized signed 8 bit integer.
 *
 * @return Length of the examined characters in the buffer or
 * #SAI_SERIALIZE_ERROR on error.
 */
int sai_deserialize_s8(
        _In_ const char *buffer,
        _Out_ int8_t *s8);

int sai_deserialize_s16(
        _In_ const char *buffer,
        _Out_ int16_t *s16);

int sai_deserialize_s32(
        _In_ const char *buffer,
        _Out_ int32_t *s32);

int sai_deserialize_s64(
        _In_ const char *buffer,
        _Out_ int64_t *s64);

/**
 * @brief Deserialize enum value.
 *
 * @note Buffer don't need to end with zero character, it can end at any
 * non-alpha and non-underscore character and function will still succeed. This
 * is helpful when parsing larger combined data.
 *
 * @param[in] buffer Buffer to be examined.
 * @param[in] meta Enum metadata to be checked against.
 * @param[out] value Deserialized value.
 *
 * @return Length of the examined characters in the buffer or
 * #SAI_SERIALIZE_ERROR on error.
 */
int sai_deserialize_enum(
        _In_ const char *buffer,
        _In_ const sai_enum_metadata_t *meta,
        _Out_ int32_t *value);

/**
 * @}
 */
#endif /** __SAISERIALIZE_H_ */
