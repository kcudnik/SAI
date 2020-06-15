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
 *    Dell Products, L.P., Facebook, Inc., Marvell International Ltd.
 *
 * @file    saiportextensions.h
 *
 * @brief   This module defines SAI Port interface
 */

#if !defined (__SAIPORTEXTENSIONS_H_)
#define __SAIPORTEXTENSIONS_H_

#include <saitypes.h>
#include <saiport.h>

/**
 * @defgroup SAIPORTEXTENSIONS SAI - Port specific API definitions
 *
 * @{
 */

/**
 * @brief Attribute data for #SAI_PORT_ATTR_TYPE
 *
 * @flags
 */
typedef enum _sai_port_type_extensions_t
{
    SAI_PORT_TYPE_EXTENSIONS_RANGE_START = SAI_PORT_TYPE_MAX,

    SAI_PORT_TYPE_CUSTOM1 = SAI_PORT_TYPE_EXTENSIONS_RANGE_START,

    SAI_PORT_TYPE_EXTENSIONS_RANGE_END,

} sai_port_type_extensions_t;

/**
 * @}
 */
#endif /** __SAIPORTEXTENSIONS_H_ */
