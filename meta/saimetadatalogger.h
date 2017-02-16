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
 *    FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 *    See the Apache Version 2.0 License for specific language governing
 *    permissions and limitations under the License.
 *
 *    Microsoft would like to thank the following companies for their review and
 *    assistance with these files: Intel Corporation, Mellanox Technologies Ltd,
 *    Dell Products, L.P., Facebook, Inc
 *
 * @file    saimetadatatypes.h
 *
 * @brief   This module defines SAI Metadata Types
 */

#ifndef __SAI_METADATA_LOGGER_H__
#define __SAI_METADATA_LOGGER_H__

#include <stdio.h>

/**
 * @defgroup SAIMETADATALOGGER SAI Metadata Types Definitions
 *
 * @{
 */

/**
 * @brief Log level function definition.
 *
 * User can sepcify his own function thah will be called when message log level
 * will be greater or equal to #sai_meta_log_level.
 */
typedef void (*sai_meta_log_fn)(
        _In_ sai_log_level_t log_level,
        _In_ const char *file,
        _In_ int line,
        _In_ const char *func,
        _In_ const char *format,
        ...);

/**
 * @brief User specified log function.
 *
 * TODO: add a set function to update this?
 */
extern volatile sai_meta_log_fn sai_meta_log;

/**
 * @brief Log level for sai metadat macros.
 *
 * Log level can be changed by user at any time.
 *
 * TODO: add a set function to update this?
 */
extern volatile sai_log_level_t sai_meta_log_level;

/**
 * @brief Helper log macro definition
 *
 * If logger function is NULL, stderr is used to print messages. Also fprintf
 * function will validate parameters at compilation time.
 */
#define SAI_META_LOG(loglevel,format,...)\
    if (loglevel >= sai_meta_log_level)\
{\
    if (sai_meta_log == NULL) /* or syslog? */ \
        fprintf(stderr, "%s:%d %s: " format "\n", __FILE__, __LINE__, __func__, ##__VA_ARGS__);\
    else\
        sai_meta_log(loglevel, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__);\
}

/*
 * Helper macros.
 */

#define SAI_META_LOG_DEBUG(format,...)      SAI_META_LOG(SAI_LOG_LEVEL_DEBUG,    ":- %s: " format, __func__, ##__VA_ARGS__)
#define SAI_META_LOG_INFO(format,...)       SAI_META_LOG(SAI_LOG_LEVEL_INFO,     ":- %s: " format, __func__, ##__VA_ARGS__)
#define SAI_META_LOG_NOTICE(format,...)     SAI_META_LOG(SAI_LOG_LEVEL_NOTICE,   ":- %s: " format, __func__, ##__VA_ARGS__)
#define SAI_META_LOG_WARN(format,...)       SAI_META_LOG(SAI_LOG_LEVEL_WARN,     ":- %s: " format, __func__, ##__VA_ARGS__)
#define SAI_META_LOG_ERROR(format,...)      SAI_META_LOG(SAI_LOG_LEVEL_ERROR,    ":- %s: " format, __func__, ##__VA_ARGS__)
#define SAI_META_LOG_CRITICAL(format,...)   SAI_META_LOG(SAI_LOG_LEVEL_CRITICAL, ":- %s: " format, __func__, ##__VA_ARGS__)

#define SAI_META_LOG_ENTER()                SAI_META_LOG(SAI_LOG_LEVEL_DEBUG, ":> %s: enter", __func__);
#define SAI_META_LOG_EXIT()                 SAI_META_LOG(SAI_LOG_LEVEL_DEBUG, ":< %s: exit", __func__);

#ifdef __cplusplus

#undef  SAI_META_LOG_ENTER
#undef  SAI_META_LOG_EXIT

namespace sai {
namespace meta {

class ScopeLogger
{
    public:

        ScopeLogger(char *file, int line, const char *func, const char *pretty)
            : m_file(file), m_line(line), m_func(func), m_pretty(pretty)
        {
            if (SAI_LOG_LEVEL_DEBUG >= sai_meta_log_level)
            {
                if (sai_meta_log == NULL)
                {
                    fprintf(stderr, "%s:%d %s: :> %s: enter\n", m_file, m_line, m_func, m_func);
                }
                else
                {
                    sai_meta_log(SAI_LOG_LEVEL_DEBUG, m_file, m_line, m_func, ":> %s: enter", m_func);
                }
            } 
        }

        ~ScopeLogger()
        {
            if (SAI_LOG_LEVEL_DEBUG >= sai_meta_log_level)
            {
                if (sai_meta_log == NULL)
                {
                    fprintf(stderr, "%s:%d %s: :< %s: exit\n", m_file, m_line, m_func, m_func);
                }
                else
                {
                    sai_meta_log(SAI_LOG_LEVEL_DEBUG, m_file, m_line, m_func, ":< %s: exit", m_func);
                }
            } 
        }

    private:
        const char *m_file;
        const int   m_line;
        const char *m_func;
        const char *m_pretty;
};

} /* sai namespace */
} /* meta namespace */

#define SAI_META_LOG_ENTER() sai::meta::ScopeLogger logger ## __LINE__ (__FILE__, __LINE__, __func__, __PRETTY_FUNCTION__);
#define SAI_META_LOG_EXIT()

#endif

/**
 * @}
 */
#endif /** __SAI_METADATA_LOGGER_H__ */
