#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <ctype.h>
#include <byteswap.h>
#include <inttypes.h>
#include <sai.h>
#include "saimetadatautils.h"
#include "saimetadata.h"
#include "saiserialize.h"

#define MAX_PRINT_CHARS 60

int sai_serialize_bool(
        _Out_ char *buffer,
        _In_ bool b)
{
    return sprintf(buffer, "%s", b ? "true" : "false");
}

int sai_serialize_u8(
        _Out_ char *buffer,
        _In_ uint8_t u8)
{
    return sprintf(buffer, "%u", u8);
}

int sai_serialize_s8(
        _Out_ char *buffer,
        _In_ int8_t u8)
{
    return sprintf(buffer, "%d", u8);
}

int sai_serialize_u16(
        _Out_ char *buffer,
        _In_ uint16_t u16)
{
    return sprintf(buffer, "%u", u16);
}

int sai_serialize_s16(
        _Out_ char *buffer,
        _In_ int16_t s16)
{
    return sprintf(buffer, "%d", s16);
}

int sai_serialize_u32(
        _Out_ char *buffer,
        _In_ uint32_t u32)
{
    return sprintf(buffer, "%u", u32);
}

int sai_serialize_s32(
        _Out_ char *buffer,
        _In_ int32_t s32)
{
    return sprintf(buffer, "%d", s32);
}

int sai_serialize_u64(
        _Out_ char *buffer,
        _In_ uint64_t u64)
{
    return sprintf(buffer, "%lu", u64);
}

int sai_serialize_s64(
        _Out_ char *buffer,
        _In_ int64_t s64)
{
    return sprintf(buffer, "%ld", s64);
}

int sai_serialize_enum(
        _Out_ char *buffer,
        _In_ const sai_enum_metadata_t* meta,
        _In_ int32_t value)
{
    if (meta == NULL)
    {
        return sai_serialize_s32(buffer, value);
    }

    size_t i = 0;

    for (; i < meta->valuescount; ++i)
    {
        if (meta->values[i] == value)
        {
            return sprintf(buffer, "%s", meta->valuesnames[i]);
        }
    }

    SAI_META_LOG_WARN("enum value %d not found in enum %s", value, meta->name);

    return sai_serialize_s32(buffer, value);
}

int sai_serialize_object_type(
        _Out_ char *buffer,
        _In_ sai_object_type_t object_type)
{
    return sai_serialize_enum(buffer, &sai_metadata_enum_sai_object_type_t, object_type);
}

int sai_serialize_mac(
        _Out_ char *buffer,
        _In_ const sai_mac_t mac)
{
    return sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

int sai_serialize_ipv4(
        _Out_ char *buffer,
        _In_ sai_ip4_t ip)
{
    struct sockaddr_in sa;

    memcpy(&sa.sin_addr, &ip, 4);

    if (inet_ntop(AF_INET, &(sa.sin_addr), buffer, INET_ADDRSTRLEN) == NULL)
    {
        SAI_META_LOG_WARN("failed to convert ipv4 address, errno: %s", strerror(errno));
        return SAI_SERIALIZE_ERROR;
    }

    return (int)strlen(buffer);
}

int sai_serialize_ipv6(
        _Out_ char *buffer,
        _In_ const sai_ip6_t ip)
{
    struct sockaddr_in6 sa6;

    memcpy(&sa6.sin6_addr, ip, 16);

    if (inet_ntop(AF_INET6, &(sa6.sin6_addr), buffer, INET6_ADDRSTRLEN) == NULL)
    {
        SAI_META_LOG_WARN("failed to convert ipv6 address, errno: %s", strerror(errno));
        return SAI_SERIALIZE_ERROR;
    }

    return (int)strlen(buffer);
}

int sai_serialize_ip_address(
        _Out_ char *buffer,
        _In_ const sai_ip_address_t *ip_address)
{
    switch (ip_address->addr_family)
    {
        case SAI_IP_ADDR_FAMILY_IPV4:

            return sai_serialize_ipv4(buffer, ip_address->addr.ip4);

        case SAI_IP_ADDR_FAMILY_IPV6:

            return sai_serialize_ipv6(buffer, ip_address->addr.ip6);

        default:

            SAI_META_LOG_WARN("invalid ip address family: %d", ip_address->addr_family);
            return SAI_SERIALIZE_ERROR;
    }
}

int sai_serialize_ipv4_mask(
        _Out_ char *buffer,
        _In_ sai_ip4_t mask)
{
    uint32_t n = 32;
    uint32_t tmp = 0xFFFFFFFF;

    mask = __builtin_bswap32(mask);

    for (; (tmp != mask) && tmp; tmp <<= 1, n--);

    if (tmp == mask)
    {
        return sai_serialize_u32(buffer, n);
    }

    SAI_META_LOG_WARN("ipv4 mask 0x%X has holes", htonl(mask));
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_ipv6_mask(
        _Out_ char *buffer,
        _In_ const sai_ip6_t mask)
{
    uint32_t n = 64;
    uint64_t tmp = 0xFFFFFFFFFFFFFFFFUL;

    uint64_t high = *((const uint64_t*)mask);
    uint64_t low  = *((const uint64_t*)mask + 1);

    high = __builtin_bswap64(high);
    low = __builtin_bswap64(low);

    if (high == tmp)
    {
        for (; (tmp != low) && tmp; tmp <<= 1, n--);

        if (tmp == low)
        {
            return sai_serialize_u32(buffer, 64 + n);
        }
    }
    else if (low == 0)
    {
        for (; (tmp != high) && tmp; tmp <<= 1, n--);

        if (tmp == high)
        {
            return sai_serialize_u32(buffer, n);
        }
    }

    char buf[128];

    sai_serialize_ipv6(buf, mask);

    SAI_META_LOG_WARN("ipv6 mask %s has holes", buf);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_ip_prefix(
        _Out_ char *buffer,
        _In_ const sai_ip_prefix_t *ip_prefix)
{
    int ret = 0;

    char addr[100];
    char mask[100];

    switch (ip_prefix->addr_family)
    {
        case SAI_IP_ADDR_FAMILY_IPV4:

            ret |= sai_serialize_ipv4(buffer, ip_prefix->addr.ip4);
            ret |= sai_serialize_ipv4_mask(buffer, ip_prefix->mask.ip4);

            if (ret < 0)
            {
                SAI_META_LOG_WARN("failed to serialize ipv4");
                return SAI_SERIALIZE_ERROR;
            }

            break;

        case SAI_IP_ADDR_FAMILY_IPV6:

            ret |= sai_serialize_ipv6(buffer, ip_prefix->addr.ip6);
            ret |= sai_serialize_ipv6_mask(buffer, ip_prefix->mask.ip6);

            if (ret < 0)
            {
                SAI_META_LOG_WARN("failed to serialize ipv6");
                return SAI_SERIALIZE_ERROR;
            }

            break;

        default:

            SAI_META_LOG_WARN("invalid ip address family: %d", ip_prefix->addr_family);
            return SAI_SERIALIZE_ERROR;
    }

    return sprintf(buffer, "%s/%s", addr, mask);
}

int sai_serialize_object_id(
        _Out_ char *buffer,
        _In_ sai_object_id_t oid)
{
    return sprintf(buffer, "oid:0x%lx", oid);
}

int sai_serialize_object_list(
        _Out_ char *buffer,
        _In_ const sai_object_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        buffer[index++] = '"';

        index += sai_serialize_object_id(buffer + index, list->list[i]);

        buffer[index++] = '"';

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_u8_list(
        _Out_ char *buffer,
        _In_ const sai_u8_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_u8(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_s8_list(
        _Out_ char *buffer,
        _In_ const sai_s8_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_s8(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}


int sai_serialize_u16_list(
        _Out_ char *buffer,
        _In_ const sai_u16_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_u16(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_s16_list(
        _Out_ char *buffer,
        _In_ const sai_s16_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_s16(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_u32_list(
        _Out_ char *buffer,
        _In_ const sai_u32_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_u32(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_s32_list(
        _Out_ char *buffer,
        _In_ const sai_s32_list_t *list,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_s32(buffer + index, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_enum_list(
        _Out_ char *buffer,
        _In_ const sai_s32_list_t *list,
        _In_ const sai_enum_metadata_t* meta,
        _In_ bool count_only)
{
    if (list->list == NULL || list->count == 0 || count_only)
    {
        return sprintf(buffer, "{\"count\":%u,\"list\":null}", list->count);
    }

    /* this is wrongjson since enum needs quotes on strings*/

    int index = sprintf(buffer, "{\"count\":%u,\"list\":[", list->count);

    uint32_t i = 0;

    for (; i < list->count; ++i)
    {
        index += sai_serialize_enum(buffer + index, meta, list->list[i]);

        if (i != list->count - 1)
        {
            buffer[index++] = ',';
        }
    }

    return index + sprintf(buffer + index, "]}");
}

int sai_serialize_u32_range(
        _Out_ char *buffer,
        _In_ const sai_u32_range_t *range)
{
    return sprintf(buffer, "{\"min\":%u,\"max\":%u}", range->min, range->max);
}

int sai_serialize_s32_range(
        _Out_ char *buffer,
        _In_ const sai_u32_range_t *range)
{
    return sprintf(buffer, "{\"min\":%d,\"max\":%d}", range->min, range->max);
}

/* deserialize functions */

static int sai_deserialize_uint(
        _In_ const char *buffer,
        _Out_ uint64_t *u64,
        _In_ uint64_t limit)
{
    int len = 0;
    uint64_t result = 0;
    uint64_t last = 0;

    while (isdigit(*buffer))
    {
        result = result * 10 + (uint64_t)(*buffer - '0');

        if (result > limit || last > result) /* overflow */
        {
            buffer -= len;
            len = 0;
            break;
        }

        len++;
        last = result;
        buffer++;
    }

    if (len > 0)
    {
        *u64 = result;
        return len;
    }

    SAI_META_LOG_WARN("parse '%.*s...' as uint with limit 0x%lX failed", MAX_PRINT_CHARS, buffer, limit);
    return SAI_SERIALIZE_ERROR;
}

int sai_deserialize_u64(
        _In_ const char *buffer,
        _Out_ uint64_t *u64)
{
     return sai_deserialize_uint(buffer, u64, ULONG_MAX);
}

int sai_deserialize_u32(
        _In_ const char *buffer,
        _Out_ uint32_t *u32)
{
    uint64_t u64;
    int res = sai_deserialize_uint(buffer, &u64, UINT_MAX);

    if (res > 0)
    {
        *u32 = (uint32_t)u64;
    }

    return res;
}

int sai_deserialize_u16(
        _In_ const char *buffer,
        _Out_ uint16_t *u16)
{
    uint64_t u64;
    int res = sai_deserialize_uint(buffer, &u64, USHRT_MAX);

    if (res > 0)
    {
        *u16 = (uint16_t)u64;
    }

    return res;
}

int sai_deserialize_u8(
        _In_ const char *buffer,
        _Out_ uint8_t *u8)
{
    uint64_t u64;
    int res = sai_deserialize_uint(buffer, &u64, UCHAR_MAX);

    if (res > 0)
    {
        *u8 = (uint8_t)u64;
    }

    return res;
}

static int sai_deserialize_int(
        _In_ const char *buffer,
        _Out_ int64_t *s64,
        _In_ int64_t lowetlimit,
        _In_ int64_t upperlimit)

{
    /* TODO  populate */
    return -1;
}

int sai_deserialize_s64(
        _In_ const char *buffer,
        _Out_ int64_t *s64)
{
     return sai_deserialize_int(buffer, s64, LONG_MIN, LONG_MAX);
}

int sai_deserialize_s32(
        _In_ const char *buffer,
        _Out_ int32_t *s32)
{
    int64_t s64;
    int res = sai_deserialize_int(buffer, &s64, INT_MIN, INT_MAX);

    if (res > 0)
    {
        *s32 = (int32_t)s64;
    }

    return res;
}

int sai_deserialize_s16(
        _In_ const char *buffer,
        _Out_ int16_t *s16)
{
    int64_t s64;
    int res = sai_deserialize_int(buffer, &s64, SHRT_MIN, SHRT_MAX);

    if (res > 0)
    {
        *s16 = (int16_t)s64;
    }

    return res;
}

int sai_deserialize_s8(
        _In_ const char *buffer,
        _Out_ int8_t *s8)
{
    int64_t s64;
    int res = sai_deserialize_int(buffer, &s64, CHAR_MIN, CHAR_MAX);

    if (res > 0)
    {
        *s8 = (int8_t)s64;
    }

    return res;
}

int sai_deserialize_enum(
        _In_ const char *buffer,
        _In_ const sai_enum_metadata_t *meta,
        _Out_ int32_t *value)
{
    if (meta == NULL)
    {
        return sai_deserialize_s32(buffer, value);
    }

    size_t i;

    int len = 0;

    while (isalnum(buffer[len]) || buffer[len] == '_')
    {
        len++;
    }

    for (i = 0; i < meta->valuescount; ++i)
    {
        if (strncmp(buffer, meta->valuesnames[i], (size_t)len) == 0 &&
               meta->valuesnames[i][len] == 0)
        {
            *value = meta->values[i];
            return len;
        }
    }

    SAI_META_LOG_WARN("enum %.*s... not found in enum %s", MAX_PRINT_CHARS, buffer, meta->name);

    return sai_deserialize_s32(buffer, value);
}
