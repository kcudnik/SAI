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

#define SAI_SERIALIZE_ERROR (-1)

/**
 * @brief Serialize object ID.
 *
 * @param[out] buffer Output buffer for serialized value.
 * @param[in] object_id Object ID to be serialized.
 *
 * @return Number of characters written to buffer excluding '\0',
 * or negative value on error.
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

int sai_serialize_ip_address(
        _Out_ char *buffer,
        _In_ const sai_ip_address_t *ip_address);

int sai_serialize_ip_prefix(
        _Out_ char *buffer,
        _In_ const sai_ip_prefix_t *ip_prefix);

int sai_serialize_ipv4_mask(
        _Out_ char *buffer,
        _In_ sai_ip4_t mask);

/**
 * @}
 */
#endif /** __SAISERIALIZE_H_ */
