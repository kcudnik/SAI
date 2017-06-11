#include <arpa/inet.h>
#include <byteswap.h>
#include <ctype.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sai.h>
#include "saimetadatautils.h"
#include "saimetadata.h"
#include "saiserialize.h"

#define PRIMITIVE_BUFFER_SIZE 128

int sai_serialize_bool(
        _Out_ char *buffer,
        _In_ bool flag)
{
    return sprintf(buffer, "%s", flag ? "true" : "false");
}

bool sai_serialize_is_allowed(
        _In_ char c)
{
    /*
     * When we will perform deserialize, we allow buffer string to be
     * terminated not only by zero, but also with json characters like:
     *
     * - end of quote
     * - comma, next item in array
     * - end of array
     *
     * This will be handy when performing deserialize.
     */

    return c == 0 || c == '"' || c == ',' || c == ']' || c == '}';
}

#define SAI_TRUE_LENGTH 4
#define SAI_FALSE_LENGTH 5

int sai_deserialize_bool(
        _In_ const char *buffer,
        _Out_ bool *flag)
{
    if (strncmp(buffer, "true", SAI_TRUE_LENGTH) == 0 &&
            sai_serialize_is_allowed(buffer[SAI_TRUE_LENGTH]))
    {
        *flag = true;
        return SAI_TRUE_LENGTH;
    }

    if (strncmp(buffer, "false", SAI_FALSE_LENGTH) == 0 &&
            sai_serialize_is_allowed(buffer[SAI_FALSE_LENGTH]))
    {
        *flag = false;
        return SAI_FALSE_LENGTH;
    }

    /*
     * Limit printf to maximum "false" length + 1 if there is invalid character
     * after "false" string.
     */

    SAI_META_LOG_WARN("failed to deserialize '%.*s' as bool",
            SAI_FALSE_LENGTH + 1,
            buffer);

    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_chardata(
        _Out_ char *buffer,
        _In_ const char data[SAI_CHARDATA_LENGTH])
{
    int idx;

    for (idx = 0; idx < SAI_CHARDATA_LENGTH; ++idx)
    {
        char c = data[idx];

        if (c == 0)
        {
            break;
        }

        if (isprint(c) && c != '\\' && c != '"')
        {
            buffer[idx] = c;
            continue;
        }

        SAI_META_LOG_WARN("invalid character 0x%x", c);
        return SAI_SERIALIZE_ERROR;
    }

    buffer[idx] = 0;

    return idx;
}

int sai_deserialize_chardata(
        _In_ char *buffer,
        _Out_ char data[SAI_CHARDATA_LENGTH])
{
    int idx;

    memset(data, 0, SAI_CHARDATA_LENGTH);

    for (idx = 0; idx < SAI_CHARDATA_LENGTH; ++idx)
    {
        char c = buffer[idx];

        if (c == 0)
        {
            break;
        }

        if (isprint(c) && c != '\\' && c != '"')
        {
            data[idx] = c;
            continue;
        }

        if (c == '"')
        {
            /*
             * We allow quote as last char since chardata will be serialized in
             * quotes.
             */

            break;
        }

        SAI_META_LOG_WARN("invalid character 0x%x", c);
        return SAI_SERIALIZE_ERROR;
    }

    if (sai_serialize_is_allowed(buffer[idx]))
    {
        return idx;
    }

    SAI_META_LOG_WARN("invalid character 0x%x", buffer[idx]);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_uint8(
        _Out_ char *buffer,
        _In_ uint8_t u8)
{
    return sprintf(buffer, "%u", u8);
}

int sai_deserialize_uint_helper(
        _In_ const char *buffer,
        _In_ uint64_t upper_limit,
        _Out_ uint64_t *u64)
{
    SAI_META_LOG_WARN("not implemented");
    return SAI_SERIALIZE_ERROR;
}

int sai_deserialize_uint8(
        _In_ const char *buffer,
        _Out_ uint8_t *u8)
{
    uint64_t u64;

    int res = sai_deserialize_uint_helper(buffer, UCHAR_MAX, &u64);

    if (res > 0)
    {
        *u8 = (uint8_t)u64;
    }

    return res;
}

int sai_serialize_int8(
        _Out_ char *buffer,
        _In_ int8_t u8)
{
    return sprintf(buffer, "%d", u8);
}

int sai_serialize_uint16(
        _Out_ char *buffer,
        _In_ uint16_t u16)
{
    return sprintf(buffer, "%u", u16);
}

int sai_deserialize_uint16(
        _In_ const char *buffer,
        _Out_ uint16_t *u16)
{
    uint64_t u64;

    int res = sai_deserialize_uint_helper(buffer, USHRT_MAX, &u64);

    if (res > 0)
    {
        *u16 = (uint16_t)u64;
    }

    return res;
}

int sai_serialize_int16(
        _Out_ char *buffer,
        _In_ int16_t s16)
{
    return sprintf(buffer, "%d", s16);
}

int sai_serialize_uint32(
        _Out_ char *buffer,
        _In_ uint32_t u32)
{
    return sprintf(buffer, "%u", u32);
}

int sai_deserialize_uint32(
        _In_ const char *buffer,
        _Out_ uint32_t *u32)
{
    uint64_t u64;

    int res = sai_deserialize_uint_helper(buffer, USHRT_MAX, &u64);

    if (res > 0)
    {
        *u32 = (uint32_t)u64;
    }

    return res;
}

int sai_serialize_int32(
        _Out_ char *buffer,
        _In_ int32_t s32)
{
    return sprintf(buffer, "%d", s32);
}

int sai_serialize_uint64(
        _Out_ char *buffer,
        _In_ uint64_t u64)
{
    return sprintf(buffer, "%lu", u64);
}

int sai_deserialize_uint64(
        _In_ const char *buffer,
        _Out_ uint64_t *u64)
{
    int idx = 0;
    uint64_t result = 0;

    while (isdigit(buffer[idx]))
    {
        char c = (char)(buffer[idx] - '0');

        /*
         * Base is 10 we can check, that if result is greater than (2^64-1)/10)
         * then next multiplication with 10 will cause overflow.
         */

        if (result > (ULONG_MAX/10) || ((result == ULONG_MAX/10) && (c > (char)(ULONG_MAX % 10))))
        {
            idx = 0;
            break;
        }

        result = result * 10 + (uint64_t)(c);

        idx++;
    }

    if (idx > 0 && sai_serialize_is_allowed(buffer[idx]))
    {
        *u64 = result;
        return idx;
    }

    SAI_META_LOG_WARN("failed to deserialize '%.*s...' as uint64", 21, buffer);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_int64(
        _Out_ char *buffer,
        _In_ int64_t s64)
{
    return sprintf(buffer, "%ld", s64);
}

int sai_serialize_size(
        _Out_ char *buffer,
        _In_ sai_size_t size)
{
    return sprintf(buffer, "%zu", size);
}

int sai_serialize_object_id(
        _Out_ char *buffer,
        _In_ sai_object_id_t oid)
{
    return sprintf(buffer, "oid:0x%lx", oid);
}

int sai_deserialize_object_id(
        _In_ const char *buffer,
        _Out_ sai_object_id_t *oid)
{
    int read;

    int n = sscanf(buffer, "oid:0x%16lx%n", oid, &read);

    if (n == 1 && sai_serialize_is_allowed(buffer[read]))
    {
        return read;
    }

    SAI_META_LOG_WARN("failed to deserialize '%.*s' as oid", 25, buffer);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_mac(
        _Out_ char *buffer,
        _In_ const sai_mac_t mac)
{
    return sprintf(buffer, "%02X:%02X:%02X:%02X:%02X:%02X",
            mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

#define SAI_MAC_ADDRESS_LENGTH 17

int sai_deserialize_mac(
        _In_ const char *buffer,
        _Out_ sai_mac_t mac)
{
    int arr[6];
    int read;

    int n = sscanf(buffer, "%2x:%2x:%2x:%2x:%2x:%2x%n",
            &arr[0], &arr[1], &arr[2], &arr[3], &arr[4], &arr[5], &read);

    if (n == 6 && read == SAI_MAC_ADDRESS_LENGTH && sai_serialize_is_allowed(buffer[read]))
    {
        for (n = 0; n < 6; n++)
        {
            mac[n] = (uint8_t)arr[n];
        }

        return SAI_MAC_ADDRESS_LENGTH;
    }

    SAI_META_LOG_WARN("failed to deserialize '%.*s' as mac address", 19, buffer);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_enum(
        _Out_ char *buffer,
        _In_ const sai_enum_metadata_t* meta,
        _In_ int32_t value)
{
    if (meta == NULL)
    {
        return sai_serialize_int32(buffer, value);
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

    return sai_serialize_int32(buffer, value);
}

static int sai_deserialize_ip(
        _In_ const char *buffer,
        _In_ int inet,
        _Out_ uint8_t *ip)
{
    /*
     * Since we want relaxed version of deserialize, after ip6 address there
     * may be '"' (quote), but inet_pton expects '\0' at the end, so copy at
     * most INET6 characters to local buffer.
     */

    char local[INET6_ADDRSTRLEN];

    int idx;

    for (idx = 0; idx < INET6_ADDRSTRLEN - 1; idx++)
    {
        char c = buffer[idx];

        if (isxdigit(c) || c == ':' || '.')
        {
            local[idx] = c;
            continue;
        }

        break;
    }

    local[idx] = 0;

    if (inet_pton(inet, local, ip) != 1)
    {
        /*
         * TODO we should not warn here, since we will use this method to
         * deserialize ip4 and ip6 and we will need to guess which one.
         */

        SAI_META_LOG_WARN("failed to deserialize '%.*s' as ip address, errno: %s",
                INET6_ADDRSTRLEN, buffer, strerror(errno));

        return SAI_SERIALIZE_ERROR;
    }

    return idx;
}

int sai_serialize_ip4(
        _Out_ char *buffer,
        _In_ sai_ip4_t ip4)
{
    if (inet_ntop(AF_INET, &ip4, buffer, INET_ADDRSTRLEN) == NULL)
    {
        SAI_META_LOG_WARN("failed to convert ipv4 address, errno: %s", strerror(errno));
        return SAI_SERIALIZE_ERROR;
    }

    return (int)strlen(buffer);
}

int sai_deserialize_ip4(
        _In_ const char *buffer,
        _Out_ sai_ip4_t *ip4)
{
    /* TODO we may need to reverse pointer */
    return sai_deserialize_ip(buffer, AF_INET, (uint8_t*)ip4);
}

int sai_serialize_ip6(
        _Out_ char *buffer,
        _In_ const sai_ip6_t ip6)
{
    if (inet_ntop(AF_INET6, ip6, buffer, INET6_ADDRSTRLEN) == NULL)
    {
        SAI_META_LOG_WARN("failed to convert ipv6 address, errno: %s", strerror(errno));
        return SAI_SERIALIZE_ERROR;
    }

    return (int)strlen(buffer);
}

int sai_deserialize_ip6(
        _In_ const char *buffer,
        _Out_ sai_ip6_t ip6)
{
    return sai_deserialize_ip(buffer, AF_INET6, ip6);
}

int sai_serialize_ip_address(
        _Out_ char *buffer,
        _In_ const sai_ip_address_t *ip_address)
{
    switch (ip_address->addr_family)
    {
        case SAI_IP_ADDR_FAMILY_IPV4:

            return sai_serialize_ip4(buffer, ip_address->addr.ip4);

        case SAI_IP_ADDR_FAMILY_IPV6:

            return sai_serialize_ip6(buffer, ip_address->addr.ip6);

        default:

            SAI_META_LOG_WARN("invalid ip address family: %d", ip_address->addr_family);
            return SAI_SERIALIZE_ERROR;
    }
}

int sai_deserialize_ip_address(
        _In_ const char *buffer,
        _Out_ sai_ip_address_t *ip_address)
{
    int res;

    /* try first deserialize ip4 then ip6 */

    res = sai_deserialize_ip(buffer, AF_INET, (uint8_t*)&ip_address->addr.ip4);

    if (res > 0)
    {
        ip_address->addr_family = SAI_IP_ADDR_FAMILY_IPV4;
        return res;
    }

    res = sai_deserialize_ip(buffer, AF_INET6, ip_address->addr.ip6);

    if (res > 0)
    {
        ip_address->addr_family = SAI_IP_ADDR_FAMILY_IPV6;
        return res;
    }

    SAI_META_LOG_WARN("failed to deserialize '%.*s' as ip address",
            INET6_ADDRSTRLEN, buffer);

    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_ip_prefix(
        _Out_ char *buffer,
        _In_ const sai_ip_prefix_t *ip_prefix)
{
    int ret = 0;

    char addr[PRIMITIVE_BUFFER_SIZE];
    char mask[PRIMITIVE_BUFFER_SIZE];

    switch (ip_prefix->addr_family)
    {
        case SAI_IP_ADDR_FAMILY_IPV4:

            ret |= sai_serialize_ip4(addr, ip_prefix->addr.ip4);
            ret |= sai_serialize_ip4_mask(mask, ip_prefix->mask.ip4);

            if (ret < 0)
            {
                SAI_META_LOG_WARN("failed to serialize ipv4");
                return SAI_SERIALIZE_ERROR;
            }

            break;

        case SAI_IP_ADDR_FAMILY_IPV6:

            ret |= sai_serialize_ip6(addr, ip_prefix->addr.ip6);
            ret |= sai_serialize_ip6_mask(mask, ip_prefix->mask.ip6);

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

int sai_serialize_ip4_mask(
        _Out_ char *buffer,
        _In_ sai_ip4_t mask)
{
    uint32_t n = 32;
    uint32_t tmp = 0xFFFFFFFF;

    mask = __builtin_bswap32(mask);

    for (; (tmp != mask) && tmp; tmp <<= 1, n--);

    if (tmp == mask)
    {
        return sai_serialize_uint32(buffer, n);
    }

    SAI_META_LOG_WARN("ipv4 mask 0x%X has holes", htonl(mask));
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_ip6_mask(
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
            return sai_serialize_uint32(buffer, 64 + n);
        }
    }
    else if (low == 0)
    {
        for (; (tmp != high) && tmp; tmp <<= 1, n--);

        if (tmp == high)
        {
            return sai_serialize_uint32(buffer, n);
        }
    }

    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_serialize_ip6(buf, mask);

    SAI_META_LOG_WARN("ipv6 mask %s has holes", buf);
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_hmac(
        _Out_ char *buffer,
        _In_ const sai_hmac_t *hmac)
{
    SAI_META_LOG_WARN("not implemented");
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_tlv(
        _Out_ char *buffer,
        _In_ const sai_tlv_t *tlv)
{
    SAI_META_LOG_WARN("not implemented");
    return SAI_SERIALIZE_ERROR;
}

int sai_serialize_attribute(
        _Out_ char *buffer,
        _In_ const sai_attr_metadata_t *meta,
        _In_ const sai_attribute_t *attr)
{
    SAI_META_LOG_WARN("not implemented");
    return SAI_SERIALIZE_ERROR;
}
