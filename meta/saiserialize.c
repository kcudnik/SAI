#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <byteswap.h>
#include <sai.h>
#include "saimetadatautils.h"
#include "saimetadata.h"
#include "saiserialize.h"

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

/*
uint8_t get_ip_mask(
        _In_ const uint8_t* mask,
        _In_ bool ipv6)
{
    uint8_t ones = 0;
    bool zeros = false;

    uint8_t count = ipv6 ? 128 : 32;

    for (uint8_t i = 0; i < count; i++)
    {
        bool bit = (mask[i/8]) & (1 << (7 - (i%8)));

        if (zeros && bit)
        {
            SWSS_LOG_ERROR("FATAL: invalid ipv%d mask", ipv6 ? 6 : 4);
            throw std::runtime_error("invalid ip mask");
        }

        zeros |= !bit;

        if (bit)
        {
            ones++;
        }
    }

    return ones;
}
*/

int sai_serialize_ipv4_mask(
        _Out_ char *buffer,
        _In_ sai_ip4_t mask)
{
    uint32_t n = 32;
    uint32_t tmp = 0xFFFFFFFF;

    mask = ntohl(mask);

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
    uint32_t n = 0;

    int i = 0;

    for (; i < 16; i++)
    {
        uint8_t b = mask[i];

        if (b == 0xFF)
        {
            n += 8;
            continue;
        }

        i++;

        switch (b)
        {
            case 0xFE: n++;
            case 0xFC: n++;
            case 0xF0: n++;
            case 0xE0: n++;
            case 0xC0: n++;
            case 0x80: n++;
            case 0x00: 
                       /* break for switch not loop WRONG ! XXX */
                       break;
            default:

                       SAI_META_LOG_WARN("ipv6 mask hs holes");
                       return SAI_SERIALIZE_ERROR;
        }
    }

    for (; i < 16; ++i)
    {
        if (mask[i] != 0)
        {
            SAI_META_LOG_WARN("ipv6 mask hs holes");
            return SAI_SERIALIZE_ERROR;
        }
    }

    return sai_serialize_u32(buffer, n);
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
