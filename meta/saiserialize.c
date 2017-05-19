#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sai.h>
#include "saimetadatautils.h"
#include "saimetadata.h"
#include "saiserialize.h"

/*
 * lists and uint's can be generated
 */

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
        _In_ const sai_ip4_t mask)
{
    uint32_t n = 32;
    uint32_t tmp = 0xFFFFFFFF;

    /* TODO we may need to reverse bits to use network byte order*/

    for (; (tmp != mask) && tmp; tmp <<= 1);
    
    if (tmp == mask)
    {
        return sai_serialize_u32(buffer, n);
    }

    SAI_META_LOG_WARN("ipv4 mask 0x%X has holes", mask);

    return SAI_SERIALIZE_ERROR;
    /* return sai_serialize_ipv4(buffer, mask); */
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

/*

    sai_vlan_list_t vlanlist;
    sai_acl_field_data_t aclfield;
    sai_acl_action_data_t aclaction;
    sai_qos_map_list_t qosmap;
    sai_tunnel_map_list_t tunnelmap;
    sai_acl_capability_t aclcapability;
*/

/*

}


#include "saiserialize.h"
#include "meta/sai_meta.h"
#include "swss/tokenize.h"
#include "swss/json.hpp"

#include <vector>
#include <climits>

#include <arpa/inet.h>
#include <errno.h>

using json = nlohmann::json;

int char_to_int(
        _In_ const char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';

    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;

    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;

    SWSS_LOG_ERROR("unable to convert char %d to int", c);
    throw std::runtime_error("unable to convert char to int");
}

template<class T, typename U>
T* sai_alloc_n_of_ptr_type(U count, T*)
{
    return new T[count];
}

template<typename T, typename U>
void sai_alloc_list(
        _In_ T count,
        _In_ U &element)
{
    element.count = count;
    element.list = sai_alloc_n_of_ptr_type(count, element.list);
}

template<typename T>
void sai_free_list(
        _In_ T &element)
{
    delete[] element.list;
    element.list = NULL;
}

template<typename T>
void transfer_primitive(
        _In_ const T &src_element,
        _In_ T &dst_element)
{
    const unsigned char* src_mem = reinterpret_cast<const unsigned char*>(&src_element);
    unsigned char* dst_mem = reinterpret_cast<unsigned char*>(&dst_element);

    memcpy(dst_mem, src_mem, sizeof(T));
}

template<typename T>
sai_status_t transfer_list(
        _In_ const T &src_element,
        _In_ T &dst_element,
        _In_ bool countOnly)
{
    if (countOnly || dst_element.count == 0)
    {
        transfer_primitive(src_element.count, dst_element.count);
        return SAI_STATUS_SUCCESS;
    }

    if (dst_element.list == NULL)
    {
        SWSS_LOG_ERROR("destination list is null, unable to transfer elements");

        return SAI_STATUS_FAILURE;
    }

    if (dst_element.count >= src_element.count)
    {
        if (src_element.list == NULL && src_element.count > 0)
        {
            SWSS_LOG_ERROR("source list is NULL when count is %u, wrong db insert?", src_element.count);
            throw std::runtime_error("source list is NULL when count is not zero, wrong db insert?");
        }

        transfer_primitive(src_element.count, dst_element.count);

        for (size_t i = 0; i < src_element.count; i++)
        {
            transfer_primitive(src_element.list[i], dst_element.list[i]);
        }

        return SAI_STATUS_SUCCESS;
    }

    // input buffer is too small to get all list elements, so return count only
    transfer_primitive(src_element.count, dst_element.count);

    return SAI_STATUS_BUFFER_OVERFLOW;
}

#define RETURN_ON_ERROR(x)\
{\
    sai_status_t s = (x);\
    if (s != SAI_STATUS_SUCCESS)\
        return s;\
}

sai_status_t transfer_attribute(
        _In_ sai_attr_serialization_type_t serialization_type,
        _In_ sai_attribute_t &src_attr,
        _In_ sai_attribute_t &dst_attr,
        _In_ bool countOnly)
{
    switch (serialization_type)
    {
        case SAI_SERIALIZATION_TYPE_BOOL:
            transfer_primitive(src_attr.value.booldata, dst_attr.value.booldata);
            break;

        case SAI_SERIALIZATION_TYPE_CHARDATA:
            transfer_primitive(src_attr.value.chardata, dst_attr.value.chardata);
            break;

        case SAI_SERIALIZATION_TYPE_UINT8:
            transfer_primitive(src_attr.value.u8, dst_attr.value.u8);
            break;

        case SAI_SERIALIZATION_TYPE_INT8:
            transfer_primitive(src_attr.value.s8, dst_attr.value.s8);
            break;

        case SAI_SERIALIZATION_TYPE_UINT16:
            transfer_primitive(src_attr.value.u16, dst_attr.value.u16);
            break;

        case SAI_SERIALIZATION_TYPE_INT16:
            transfer_primitive(src_attr.value.s16, dst_attr.value.s16);
            break;

        case SAI_SERIALIZATION_TYPE_UINT32:
            transfer_primitive(src_attr.value.u32, dst_attr.value.u32);
            break;

        case SAI_SERIALIZATION_TYPE_INT32:
            transfer_primitive(src_attr.value.s32, dst_attr.value.s32);
            break;

        case SAI_SERIALIZATION_TYPE_UINT64:
            transfer_primitive(src_attr.value.u64, dst_attr.value.u64);
            break;

        case SAI_SERIALIZATION_TYPE_INT64:
            transfer_primitive(src_attr.value.s64, dst_attr.value.s64);
            break;

        case SAI_SERIALIZATION_TYPE_MAC:
            transfer_primitive(src_attr.value.mac, dst_attr.value.mac);
            break;

        case SAI_SERIALIZATION_TYPE_IP4:
            transfer_primitive(src_attr.value.ip4, dst_attr.value.ip4);
            break;

        case SAI_SERIALIZATION_TYPE_IP6:
            transfer_primitive(src_attr.value.ip6, dst_attr.value.ip6);
            break;

        case SAI_SERIALIZATION_TYPE_IP_ADDRESS:
            transfer_primitive(src_attr.value.ipaddr, dst_attr.value.ipaddr);
            break;

        case SAI_SERIALIZATION_TYPE_OBJECT_ID:
            transfer_primitive(src_attr.value.oid, dst_attr.value.oid);
            break;

        case SAI_SERIALIZATION_TYPE_OBJECT_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.objlist, dst_attr.value.objlist, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_UINT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u8list, dst_attr.value.u8list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_INT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s8list, dst_attr.value.s8list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_UINT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u16list, dst_attr.value.u16list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_INT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s16list, dst_attr.value.s16list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_UINT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u32list, dst_attr.value.u32list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_INT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s32list, dst_attr.value.s32list, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_UINT32_RANGE:
            transfer_primitive(src_attr.value.u32range, dst_attr.value.u32range);
            break;

        case SAI_SERIALIZATION_TYPE_INT32_RANGE:
            transfer_primitive(src_attr.value.s32range, dst_attr.value.s32range);
            break;

        case SAI_SERIALIZATION_TYPE_VLAN_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.vlanlist, dst_attr.value.vlanlist, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_QOS_MAP_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.qosmap, dst_attr.value.qosmap, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_TUNNEL_MAP_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.tunnelmap, dst_attr.value.tunnelmap, countOnly));
            break;

            // ACL FIELD DATA

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.data.booldata, dst_attr.value.aclfield.data.booldata);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u8, dst_attr.value.aclfield.mask.u8);
            transfer_primitive(src_attr.value.aclfield.data.u8, dst_attr.value.aclfield.data.u8);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s8, dst_attr.value.aclfield.mask.s8);
            transfer_primitive(src_attr.value.aclfield.data.s8, dst_attr.value.aclfield.data.s8);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u16, dst_attr.value.aclfield.mask.u16);
            transfer_primitive(src_attr.value.aclfield.data.u16, dst_attr.value.aclfield.data.u16);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s16, dst_attr.value.aclfield.mask.s16);
            transfer_primitive(src_attr.value.aclfield.data.s16, dst_attr.value.aclfield.data.s16);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u16, dst_attr.value.aclfield.mask.u16);
            transfer_primitive(src_attr.value.aclfield.data.u16, dst_attr.value.aclfield.data.u16);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s32, dst_attr.value.aclfield.mask.s32);
            transfer_primitive(src_attr.value.aclfield.data.s32, dst_attr.value.aclfield.data.s32);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.mac, dst_attr.value.aclfield.mask.mac);
            transfer_primitive(src_attr.value.aclfield.data.mac, dst_attr.value.aclfield.data.mac);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.ip4, dst_attr.value.aclfield.mask.ip4);
            transfer_primitive(src_attr.value.aclfield.data.ip4, dst_attr.value.aclfield.data.ip4);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.ip6, dst_attr.value.aclfield.mask.ip6);
            transfer_primitive(src_attr.value.aclfield.data.ip6, dst_attr.value.aclfield.data.ip6);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.data.oid, dst_attr.value.aclfield.data.oid);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclfield.data.objlist, dst_attr.value.aclfield.data.objlist, countOnly));
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclfield.mask.u8list, dst_attr.value.aclfield.mask.u8list, countOnly));
            transfer_list(src_attr.value.aclfield.data.u8list, dst_attr.value.aclfield.data.u8list, countOnly);
            break;

            // ACL ACTION DATA

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u8, dst_attr.value.aclaction.parameter.u8);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s8, dst_attr.value.aclaction.parameter.s8);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u16, dst_attr.value.aclaction.parameter.u16);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s16, dst_attr.value.aclaction.parameter.s16);
            break;


        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u32, dst_attr.value.aclaction.parameter.u32);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s32, dst_attr.value.aclaction.parameter.s32);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.mac, dst_attr.value.aclaction.parameter.mac);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.ip4, dst_attr.value.aclaction.parameter.ip4);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.ip6, dst_attr.value.aclaction.parameter.ip6);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.oid, dst_attr.value.aclaction.parameter.oid);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclaction.parameter.objlist, dst_attr.value.aclaction.parameter.objlist, countOnly));
            break;

        default:
            return SAI_STATUS_NOT_IMPLEMENTED;
    }

    return SAI_STATUS_SUCCESS;
}

sai_status_t transfer_attributes(
        _In_ sai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ sai_attribute_t *src_attr_list,
        _In_ sai_attribute_t *dst_attr_list,
        _In_ bool countOnly)
{
    for (uint32_t i = 0; i < attr_count; i++)
    {
        sai_attribute_t &src_attr = src_attr_list[i];
        sai_attribute_t &dst_attr = dst_attr_list[i];

        auto meta = get_attribute_metadata(object_type, src_attr.id);

        if (src_attr.id != dst_attr.id)
        {
            SWSS_LOG_ERROR("src vs dst attr id don't match GET mismatch");
            throw std::runtime_error("src vs dst attr id don't match GET mismatch");
        }

        if (meta == NULL)
        {
            SWSS_LOG_ERROR("unable to get metadata for object type %x, attribute %x", object_type, src_attr.id);
            throw std::runtime_error("unable to get metadata");
        }

        RETURN_ON_ERROR(transfer_attribute(meta->serializationtype, src_attr, dst_attr, countOnly));
    }

    return SAI_STATUS_SUCCESS;
}

// util

uint8_t get_ip_mask(
        _In_ const uint8_t* mask,
        _In_ bool ipv6)
{
    SWSS_LOG_ENTER();

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

uint8_t get_ipv4_mask(
        _In_ const sai_ip4_t mask)
{
    SWSS_LOG_ENTER();

    return get_ip_mask((const uint8_t*)&mask, false);
}

int get_ipv6_mask(
        _In_ const sai_ip6_t &mask)
{
    SWSS_LOG_ENTER();

    return get_ip_mask(mask, true);
}

void sai_populate_ip_mask(
        _In_ uint8_t bits,
        _Out_ uint8_t *mask,
        _In_ bool ipv6)
{
    SWSS_LOG_ENTER();

    if (mask == NULL)
    {
        SWSS_LOG_ERROR("mask pointer is null");
        throw std::runtime_error("mask pointer is null");
    }

    if ((ipv6 && (bits > 128)) || (!ipv6 && (bits > 32)))
    {
        SWSS_LOG_ERROR("invalid ip mask bits %u", bits);
        throw std::runtime_error("invalid ip mask bits");
    }

    // zero all mask
    memset(mask, 0, ipv6 ? 16 : 4);

    // set full bytes to all ones
    for (uint8_t i = 0; i < bits/8; i++)
    {
        mask[i] = 0xFF;
    }

    int rem = bits % 8;

    if (rem)
    {
        // set last non zero byte
        mask[bits/8] =(uint8_t)(0xFF << (8 - rem));
    }
}


// new methods

std::string sai_serialize_bool(
        _In_ bool b)
{
    SWSS_LOG_ENTER();

    return b ? "true" : "false";
}

#define CHAR_LEN 32

std::string sai_serialize_chardata(
        _In_ const char data[CHAR_LEN])
{
    SWSS_LOG_ENTER();

    std::string s;

    char buf[8];

    size_t len = strnlen(data, CHAR_LEN);

    for (size_t i = 0; i < len; ++i)
    {
        unsigned char c = (unsigned char)data[i];

        if ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
        {
            s += c;
            continue;
        }

        if (c == '\\')
        {
            s += "\\\\";
            continue;
        }

        snprintf(buf, sizeof(buf), "\\x%02X", (int)c);

        s += buf;
    }

    return s;
}

template <typename T>
std::string sai_serialize_number(
        _In_ const T number,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    if (hex)
    {
        char buf[32];

        snprintf(buf, sizeof(buf), "0x%lx", (uint64_t)number);

        return buf;
    }

    return std::to_string(number);
}

std::string sai_serialize_number(
        _In_ const uint32_t number,
        _In_ bool hex)
{
    return sai_serialize_number<uint32_t>(number, hex);
}

std::string sai_serialize_attr_id(
        _In_ const sai_attr_metadata_t& meta)
{
    SWSS_LOG_ENTER();

    return meta.attridname;
}

std::string sai_serialize_status(
        _In_ const sai_status_t status)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(status, &metadata_enum_sai_status_t);
}

std::string sai_serialize_object_type(
        _In_ const sai_object_type_t object_type)
{
    return sai_serialize_enum(object_type, &metadata_enum_sai_object_type_t);
}

std::string sai_serialize_packet_color(
        _In_ sai_packet_color_t color)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(color, &metadata_enum_sai_packet_color_t);
}

std::string sai_serialize_vlan_id(
        _In_ sai_vlan_id_t vlan_id)
{
    SWSS_LOG_ENTER();

    return sai_serialize_number(vlan_id);
}

std::string sai_serialize_neighbor_entry(
        _In_ const sai_neighbor_entry_t &ne)
{
    SWSS_LOG_ENTER();

    json j;

    j["rif"] = sai_serialize_object_id(ne.rif_id);
    j["ip"] = sai_serialize_ip_address(ne.ip_address);

    return j.dump();
}

std::string sai_serialize_route_entry(
        _In_ const sai_unicast_route_entry_t& route_entry)
{
    SWSS_LOG_ENTER();

    json j;

    j["vr"] = sai_serialize_object_id(route_entry.vr_id);
    j["dest"] = sai_serialize_ip_prefix(route_entry.destination);

    return j.dump();
}

std::string sai_serialize_fdb_entry(
        _In_ const sai_fdb_entry_t& fdb_entry)
{
    SWSS_LOG_ENTER();

    json j;

    j["mac"] = sai_serialize_mac(fdb_entry.mac_address);
    j["vlan"] = sai_serialize_vlan_id(fdb_entry.vlan_id);

    return j.dump();
}

std::string sai_serialize_port_stat(
        _In_ const sai_port_stat_counter_t counter)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(counter, &metadata_enum_sai_port_stat_t);
}

std::string sai_serialize_hostif_trap_id(
        _In_ const sai_hostif_trap_id_t hostif_trap_id)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(hostif_trap_id, &metadata_enum_sai_hostif_trap_type_t);
}

std::string sai_serialize_switch_oper_status(
        _In_ sai_switch_oper_status_t status)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(status, &metadata_enum_sai_switch_oper_status_t);
}


template <typename T>
std::string sai_serialize_number_list(
        _In_ const T& list,
        _In_ bool countOnly,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    return sai_serialize_list(list, countOnly, [&](decltype(*list.list)& item) { return sai_serialize_number(item, hex);} );
}

json sai_serialize_qos_map_params(
        _In_ const sai_qos_map_params_t& params)
{
    json j;

    j["tc"]     = params.tc;
    j["dscp"]   = params.dscp;
    j["dot1p"]  = params.dot1p;
    j["prio"]   = params.prio;
    j["pg"]     = params.pg;
    j["qidx"]   = params.queue_index;
    j["color"]  = sai_serialize_packet_color(params.color);

    return j;
}

json sai_serialize_qos_map(
        _In_ const sai_qos_map_t& qosmap)
{
    json j;

    j["key"]    = sai_serialize_qos_map_params(qosmap.key);
    j["value"]  = sai_serialize_qos_map_params(qosmap.value);;

    return j;
}

std::string sai_serialize_qos_map_list(
        _In_ const sai_qos_map_list_t& qosmap,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    json j;

    j["count"] = qosmap.count;

    if (qosmap.list == NULL || countOnly)
    {
        j["list"] = nullptr;

        return j.dump();
    }

    json arr = json::array();

    for (uint32_t i = 0; i < qosmap.count; ++i)
    {
        json item = sai_serialize_qos_map(qosmap.list[i]);

        arr.push_back(item);
    }

    j["list"] = arr;

    return j.dump();
}

json sai_serialize_tunnel_map_params(
        _In_ const sai_tunnel_map_params_t& params)
{
    json j;

    j["oecn"] = params.oecn;
    j["uecn"] = params.uecn;
    j["vni"]  = params.vni_id;
    j["vlan"] = sai_serialize_vlan_id(params.vlan_id);

    return j;
}

json sai_serialize_tunnel_map(
        _In_ const sai_tunnel_map_t& tunnelmap)
{
    json j;

    j["key"]    = sai_serialize_tunnel_map_params(tunnelmap.key);
    j["value"]  = sai_serialize_tunnel_map_params(tunnelmap.value);;

    return j;
}

std::string sai_serialize_tunnel_map_list(
        _In_ const sai_tunnel_map_list_t& tunnelmap,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    json j;

    j["count"] = tunnelmap.count;

    if (tunnelmap.list == NULL || countOnly)
    {
        j["list"] = nullptr;

        return j.dump();
    }

    json arr = json::array();

    for (uint32_t i = 0; i < tunnelmap.count; ++i)
    {
        json item = sai_serialize_tunnel_map(tunnelmap.list[i]);

        arr.push_back(item);
    }

    j["list"] = arr;

    return j.dump();
}

template <typename T>
std::string sai_serialize_range(
        _In_ const T& range)
{
    SWSS_LOG_ENTER();

    return sai_serialize_number(range.min) + "," + sai_serialize_number(range.max);
}

std::string sai_serialize_acl_action(
        _In_ const sai_attr_metadata_t& meta,
        _In_ const sai_acl_action_data_t& action,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    if (action.enable == false)
    {
        // parameter is not needed when action is disabled
        return "disabled";
    }

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
            return sai_serialize_number(action.parameter.u8);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
            return sai_serialize_number(action.parameter.s8);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
            return sai_serialize_number(action.parameter.u16);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
            return sai_serialize_number(action.parameter.s16);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
            return sai_serialize_number(action.parameter.u32);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
            return sai_serialize_enum(action.parameter.s32, meta.enummetadata);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
            return sai_serialize_mac(action.parameter.mac);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
            return sai_serialize_ipv4(action.parameter.ip4);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
            return sai_serialize_ipv6(action.parameter.ip6);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
            return sai_serialize_object_id(action.parameter.oid);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            return sai_serialize_oid_list(action.parameter.objlist, countOnly);

        default:
            SWSS_LOG_ERROR("FATAIL: invalid serialization type %d", meta.serializationtype);
            throw std::runtime_error("serialization type is not supported");
    }
}

std::string sai_serialize_acl_field(
        _In_ const sai_attr_metadata_t& meta,
        _In_ const sai_acl_field_data_t& field,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    if (field.enable == false)
    {
        // parameter is not needed when field is disabled
        return "disabled";
    }

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
            return sai_serialize_bool(field.data.booldata);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
            return sai_serialize_number(field.data.u8) + "&mask:" + sai_serialize_number(field.mask.u8, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
            return sai_serialize_number(field.data.s8) + "&mask:" + sai_serialize_number(field.mask.s8, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
            return sai_serialize_number(field.data.u16) + "&mask:" + sai_serialize_number(field.mask.u16, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
            return sai_serialize_number(field.data.s16) + "&mask:" + sai_serialize_number(field.mask.s16, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
            return sai_serialize_number(field.data.u32) + "&mask:" + sai_serialize_number(field.mask.u32, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
            return sai_serialize_enum(field.data.s32, meta.enummetadata) + "&mask:" + sai_serialize_number(field.mask.s32, true);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
            return sai_serialize_mac(field.data.mac) +"&mask:" + sai_serialize_mac(field.mask.mac);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
            return sai_serialize_ipv4(field.data.ip4) +"&mask:" + sai_serialize_ipv4(field.mask.ip4);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
            return sai_serialize_ipv6(field.data.ip6) +"&mask:" + sai_serialize_ipv6(field.mask.ip6);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
            return sai_serialize_object_id(field.data.oid);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
            return sai_serialize_oid_list(field.data.objlist, countOnly);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            return sai_serialize_number_list(field.data.u8list, countOnly) + "&mask:" + sai_serialize_number_list(field.mask.u8list, countOnly, true);

        default:
            SWSS_LOG_ERROR("FATAIL: invalid serialization type %d", meta.serializationtype);
            throw std::runtime_error("serialization type is not supported");
    }
}

std::string sai_serialize_attr_value(
        _In_ const sai_attr_metadata_t& meta,
        _In_ const sai_attribute_t &attr,
        _In_ const bool countOnly)
{
    SWSS_LOG_ENTER();

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_BOOL:
            return sai_serialize_bool(attr.value.booldata);

        case SAI_SERIALIZATION_TYPE_CHARDATA:
            return sai_serialize_chardata(attr.value.chardata);

        case SAI_SERIALIZATION_TYPE_UINT8:
            return sai_serialize_number(attr.value.u8);

        case SAI_SERIALIZATION_TYPE_INT8:
            return sai_serialize_number(attr.value.s8);

        case SAI_SERIALIZATION_TYPE_UINT16:
            return sai_serialize_number(attr.value.u16);

        case SAI_SERIALIZATION_TYPE_INT16:
            return sai_serialize_number(attr.value.s16);

        case SAI_SERIALIZATION_TYPE_UINT32:
            return sai_serialize_number(attr.value.u32);

        case SAI_SERIALIZATION_TYPE_INT32:
            return sai_serialize_enum(attr.value.s32, meta.enummetadata);

        case SAI_SERIALIZATION_TYPE_UINT64:
            return sai_serialize_number(attr.value.u64);

        case SAI_SERIALIZATION_TYPE_INT64:
            return sai_serialize_number(attr.value.s64);

        case SAI_SERIALIZATION_TYPE_MAC:
            return sai_serialize_mac(attr.value.mac);

        case SAI_SERIALIZATION_TYPE_IP4:
            return sai_serialize_ipv4(attr.value.ip4);

        case SAI_SERIALIZATION_TYPE_IP6:
            return sai_serialize_ipv6(attr.value.ip6);

        case SAI_SERIALIZATION_TYPE_IP_ADDRESS:
            return sai_serialize_ip_address(attr.value.ipaddr);

        case SAI_SERIALIZATION_TYPE_OBJECT_ID:
            return sai_serialize_object_id(attr.value.oid);

        case SAI_SERIALIZATION_TYPE_OBJECT_LIST:
            return sai_serialize_oid_list(attr.value.objlist, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT8_LIST:
            return sai_serialize_number_list(attr.value.u8list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT8_LIST:
            return sai_serialize_number_list(attr.value.s8list, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT16_LIST:
            return sai_serialize_number_list(attr.value.u16list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT16_LIST:
            return sai_serialize_number_list(attr.value.s16list, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT32_LIST:
            return sai_serialize_number_list(attr.value.u32list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT32_LIST:
            return sai_serialize_enum_list(attr.value.s32list, meta.enummetadata, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT32_RANGE:
            return sai_serialize_range(attr.value.u32range);

        case SAI_SERIALIZATION_TYPE_INT32_RANGE:
            return sai_serialize_range(attr.value.s32range);

        case SAI_SERIALIZATION_TYPE_VLAN_LIST:
            return sai_serialize_number_list(attr.value.vlanlist, countOnly);

        case SAI_SERIALIZATION_TYPE_QOS_MAP_LIST:
            return sai_serialize_qos_map_list(attr.value.qosmap, countOnly);

        case SAI_SERIALIZATION_TYPE_TUNNEL_MAP_LIST:
            return sai_serialize_tunnel_map_list(attr.value.tunnelmap, countOnly);

            // ACL FIELD DATA

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            return sai_serialize_acl_field(meta, attr.value.aclfield, countOnly);

            // ACL ACTION DATA

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            return sai_serialize_acl_action(meta, attr.value.aclaction, countOnly);

        default:
            SWSS_LOG_ERROR("FATAIL: invalid serialization type %d", meta.serializationtype);
            throw std::runtime_error("serialization type is not supported");
    }
}

std::string sai_serialize_ip_prefix(
        _In_ const sai_ip_prefix_t& prefix)
{
    SWSS_LOG_ENTER();

    switch (prefix.addr_family)
    {
        case SAI_IP_ADDR_FAMILY_IPV4:

            return sai_serialize_ipv4(prefix.addr.ip4) + "/" + sai_serialize_number((get_ipv4_mask(prefix.mask.ip4)));

        case SAI_IP_ADDR_FAMILY_IPV6:

            return sai_serialize_ipv6(prefix.addr.ip6) + "/" + sai_serialize_number(get_ipv6_mask(prefix.mask.ip6));

        default:

            SWSS_LOG_ERROR("FATAL: invalid ip prefix address family: %d", prefix.addr_family);
            throw std::runtime_error("invalid ip address family");
    }
}

std::string sai_serialize_port_oper_status(
        _In_ sai_port_oper_status_t status)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(status, &metadata_enum_sai_port_oper_status_t);
}

std::string sai_serialize_port_event(
        _In_ sai_port_event_t event)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(event, &metadata_enum_sai_port_event_t);
}

std::string sai_serialize_fdb_event(
        _In_ sai_fdb_event_t event)
{
    SWSS_LOG_ENTER();

    return sai_serialize_enum(event, &metadata_enum_sai_fdb_event_t);
}

json sai_serialize_json_fdb_event_notification_data(
        _In_ const sai_fdb_event_notification_data_t& fdb_event)
{
    SWSS_LOG_ENTER();

    json j;

    j["fdb_event"] = sai_serialize_fdb_event(fdb_event.event_type);
    j["fdb_entry"] = sai_serialize_fdb_entry(fdb_event.fdb_entry);

    json arr = json::array();

    for (uint32_t i = 0; i < fdb_event.attr_count; ++i)
    {
        auto meta = get_attribute_metadata(SAI_OBJECT_TYPE_FDB, fdb_event.attr[i].id);

        if (meta == NULL)
        {
            SWSS_LOG_ERROR("unable to get metadata for object type %x, attribute %x", SAI_OBJECT_TYPE_FDB, fdb_event.attr[i].id);
            throw std::runtime_error("unable to get metadata");
        }

        json item;

        item["id"] = meta->attridname;
        item["value"] = sai_serialize_attr_value(*meta, fdb_event.attr[i]);

        arr.push_back(item);
    }

    j["list"] = arr;

    // we don't need count since it can be deduced
    return j;
}

std::string sai_serialize_fdb_event_ntf(
        _In_ uint32_t count,
        _In_ const sai_fdb_event_notification_data_t* fdb_event)
{
    SWSS_LOG_ENTER();

    if (fdb_event == NULL)
    {
        SWSS_LOG_ERROR("fdb_event pointer is null");
        throw std::runtime_error("fdb_event pointer is null");
    }

    json j = json::array();

    for (uint32_t i = 0; i < count; ++i)
    {
        json item = sai_serialize_json_fdb_event_notification_data(fdb_event[i]);

        j.push_back(item);
    }

    // we don't need count since it can be deduced
    return j.dump();
}

std::string sai_serialize_port_oper_status_ntf(
        _In_ uint32_t count,
        _In_ const sai_port_oper_status_notification_t* port_oper_status)
{
    SWSS_LOG_ENTER();

    if (port_oper_status == NULL)
    {
        SWSS_LOG_ERROR("port_oper_status pointer is null");
        throw std::runtime_error("port_oper_status pointer is null");
    }

    json j = json::array();

    for (uint32_t i = 0; i < count; ++i)
    {
        json item;

        item["port_id"] = sai_serialize_object_id(port_oper_status[i].port_id);
        item["port_state"] = sai_serialize_port_oper_status(port_oper_status[i].port_state);

        j.push_back(item);
    }

    // we don't need count since it can be deduced
    return j.dump();
}

std::string sai_serialize_port_event_ntf(
        _In_ uint32_t count,
        _In_ const sai_port_event_notification_t* port_event)
{
    SWSS_LOG_ENTER();

    if (port_event == NULL)
    {
        SWSS_LOG_ERROR("port_event pointer is null");
        throw std::runtime_error("port_event pointer is null");
    }

    json j = json::array();

    for (uint32_t i = 0; i < count; ++i)
    {
        json item;

        item["port_id"] = sai_serialize_object_id(port_event[i].port_id);
        item["port_event"] = sai_serialize_port_event(port_event[i].port_event);

        j.push_back(item);
    }

    // we don't need count since it can be deduced
    return j.dump();
}

// deserialize

void sai_deserialize_bool(
        _In_ const std::string& s,
        _Out_ bool& b)
{
    SWSS_LOG_ENTER();

    if (s == "true")
    {
        b = true;
        return;
    }

    if (s == "false")
    {
        b = false;
        return;
    }

    SWSS_LOG_ERROR("failed to deserialize '%s' as bool", s.c_str());

    throw std::runtime_error("bool deserialize failed");
}

void sai_deserialize_chardata(
        _In_ const std::string& s,
        _Out_ char chardata[CHAR_LEN])
{
    SWSS_LOG_ENTER();

    memset(chardata, 0, CHAR_LEN);

    std::string deserialized;

    size_t len = s.length();

    for (size_t i = 0; i < len; ++i)
    {
        unsigned char c = (unsigned char)s[i];

        if (c == '\\')
        {
            if (i+1 >= len || ((s[i+1] != '\\') && (s[i+1] != 'x')))
            {
                SWSS_LOG_ERROR("invalid chardata %s", s.c_str());
                throw std::runtime_error("invalid chardata");
            }

            if (s[i+1] == '\\')
            {
                deserialized += "\\";
                i++;
                continue;
            }

            i++;

            if (i + 2 >= len)
            {
                SWSS_LOG_ERROR("invalid chardata %s", s.c_str());
                throw std::runtime_error("invalid chardata");
            }

            int h = char_to_int(s[i+1]);
            int l = char_to_int(s[i+2]);

            int r = (h << 4) | l;

            c = (unsigned char)r;

            i += 2;
        }

        deserialized += c;
    }

    len = deserialized.length();

    if (len > CHAR_LEN)
    {
        SWSS_LOG_ERROR("invalid chardata %s", s.c_str());
        throw std::runtime_error("invalid chardata");
    }

    memcpy(chardata, deserialized.data(), len);
}

template<typename T>
void sai_deserialize_number(
        _In_ const std::string& s,
        _Out_ T& number,
        _In_ bool hex = false)
{
    errno = 0;

    char *endptr = NULL;

    number = (T)strtoull(s.c_str(), &endptr, hex ? 16 : 10);

    if (errno != 0 || endptr != s.c_str() + s.length())
    {
        SWSS_LOG_ERROR("invalid number %s", s.c_str());
        throw std::runtime_error("invalid number");
    }
}

void sai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint32_t& number,
        _In_ bool hex)
{
    sai_deserialize_number<uint32_t>(s, number, hex);
}

void sai_deserialize_enum(
        _In_ const std::string& s,
        _In_ const sai_enum_metadata_t *meta,
        _Out_ int32_t& value)
{
    if (meta == NULL)
    {
        return sai_deserialize_number(s, value);
    }

    for (size_t i = 0; i < meta->valuescount; ++i)
    {
        if (strcmp(s.c_str(), meta->valuesnames[i]) == 0)
        {
            value = meta->values[i];
            return;
        }
    }

    SWSS_LOG_WARN("enum %s not found in enum %s", s.c_str(), meta->name);

    sai_deserialize_number(s, value);
}

void sai_deserialize_mac(
        _In_ const std::string& s,
        _Out_ sai_mac_t& mac)
{
    SWSS_LOG_ENTER();

    if (s.length() != (6*2+5))
    {
        SWSS_LOG_ERROR("invalid mac address %s", s.c_str());
        throw std::runtime_error("invalid mac number");
    }

    int i = 0;

    for (int j = 0; j < 6 ; j++, i += 3)
    {
        int h = char_to_int(s[i+0]);
        int l = char_to_int(s[i+1]);

        int r = (h << 4) | l;

        mac[j] = (unsigned char)r;
    }
}

void sai_deserialize_object_id(
        _In_ const std::string& s,
        _Out_ sai_object_id_t& oid)
{
    if (s.find("oid:0x") != 0)
    {
        SWSS_LOG_ERROR("invalid oid %s", s.c_str());
        throw std::runtime_error("invalid oid");
    }

    errno = 0;

    char *endptr = NULL;

    oid = (sai_object_id_t)strtoull(s.c_str()+4, &endptr, 16);

    if (errno != 0 || endptr != s.c_str() + s.length())
    {
        SWSS_LOG_ERROR("invalid oid %s", s.c_str());
        throw std::runtime_error("invalid oid");
    }
}

template<typename T, typename F>
void sai_deserialize_list(
        _In_ const std::string& s,
        _Out_ T& list,
        _In_ bool countOnly,
        F deserialize_item)
{
    SWSS_LOG_ENTER();

    if (countOnly)
    {
        sai_deserialize_number(s, list.count);
        return;
    }

    auto pos = s.find(":");

    if (pos == std::string::npos)
    {
        SWSS_LOG_ERROR("invalid list %s", s.c_str());
        throw std::runtime_error("invalid list");
    }

    std::string scount = s.substr(0, pos);

    sai_deserialize_number(scount, list.count);

    std::string slist = s.substr(pos + 1);

    if (slist == "null")
    {
        list.list = NULL;
        return;
    }

    auto tokens = swss::tokenize(slist, ',');

    if (tokens.size() != list.count)
    {
        SWSS_LOG_ERROR("invalid list count %lu != %u", tokens.size(), list.count);
        throw std::runtime_error("invalid list count");
    }

    // list.list = sai_alloc_list(list.count, list);
    list.list = sai_alloc_n_of_ptr_type(list.count, list.list);

    for (uint32_t i = 0; i < list.count; ++i)
    {
        deserialize_item(tokens[i], list.list[i]);
    }
}

void sai_deserialize_oid_list(
        _In_ const std::string& s,
        _Out_ sai_object_list_t& objlist,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    sai_deserialize_list(s, objlist, countOnly, [&](const std::string sitem, sai_object_id_t& item) { sai_deserialize_object_id(sitem, item);} );
}

void sai_deserialize_enum_list(
        _In_ const std::string& s,
        _In_ const sai_enum_metadata_t* meta,
        _Out_ sai_s32_list_t& list,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    sai_deserialize_list(s, list, countOnly, [&](const std::string sitem, int32_t& item) { sai_deserialize_enum(sitem, meta, item);} );
}

template <typename T>
void sai_deserialize_number_list(
        _In_ const std::string& s,
        _Out_ T& list,
        _In_ bool countOnly,
        _In_ bool hex = false)
{
    SWSS_LOG_ENTER();

    sai_deserialize_list(s, list, countOnly, [&](const std::string sitem, decltype(*list.list)& item) { sai_deserialize_number(sitem, item, hex);} );
}

void sai_deserialize_packet_color(
        _In_ const std::string& s,
        _Out_ sai_packet_color_t& color)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_packet_color_t, (int32_t&)color);
}

void sai_deserialize_qos_map_params(
        _In_ const json& j,
        _Out_ sai_qos_map_params_t& params)
{
    SWSS_LOG_ENTER();

    params.tc             = j["tc"];
    params.dscp           = j["dscp"];
    params.dot1p          = j["dot1p"];
    params.prio           = j["prio"];
    params.pg             = j["pg"];
    params.queue_index    = j["qidx"];

    sai_deserialize_packet_color(j["color"], params.color);
}

void sai_deserialize_qos_map(
        _In_ const json& j,
        _Out_ sai_qos_map_t& qosmap)
{
    SWSS_LOG_ENTER();

    sai_deserialize_qos_map_params(j["key"], qosmap.key);
    sai_deserialize_qos_map_params(j["value"], qosmap.value);
}

void sai_deserialize_qos_map_list(
        _In_ const std::string& s,
        _Out_ sai_qos_map_list_t& qosmap,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    qosmap.count = j["count"];

    if (countOnly)
    {
        return;
    }

    if (j["list"] == nullptr)
    {
        qosmap.list = NULL;
        return;
    }

    json arr = j["list"];

    if (arr.size() != (size_t)qosmap.count)
    {
        SWSS_LOG_ERROR("qos map count mismatch %lu vs %u", arr.size(), qosmap.count);
        throw std::runtime_error("qos map count mismatch");
    }

    qosmap.list = sai_alloc_n_of_ptr_type(qosmap.count, qosmap.list);

    for (uint32_t i = 0; i < qosmap.count; ++i)
    {
        const json& item = arr[i];

        sai_deserialize_qos_map(item, qosmap.list[i]);
    }
}

void sai_deserialize_tunnel_map_params(
        _In_ const json& j,
        _Out_ sai_tunnel_map_params_t& params)
{
    SWSS_LOG_ENTER();

    SWSS_LOG_DEBUG("%s", j.dump().c_str());

    params.oecn   = j["oecn"];
    params.uecn   = j["uecn"];
    params.vni_id = j["vni"];

    sai_deserialize_vlan_id(j["vlan"], params.vlan_id);
}

void sai_deserialize_tunnel_map(
        _In_ const json& j,
        _Out_ sai_tunnel_map_t& tunnelmap)
{
    SWSS_LOG_ENTER();

    sai_deserialize_tunnel_map_params(j["key"], tunnelmap.key);
    sai_deserialize_tunnel_map_params(j["value"], tunnelmap.value);
}

void sai_deserialize_tunnel_map_list(
        _In_ const std::string& s,
        _Out_ sai_tunnel_map_list_t& tunnelmap,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    tunnelmap.count = j["count"];

    if (countOnly)
    {
        return;
    }

    if (j["list"] == nullptr)
    {
        tunnelmap.list = NULL;
        return;
    }

    json arr = j["list"];

    if (arr.size() != (size_t)tunnelmap.count)
    {
        SWSS_LOG_ERROR("tunnel map count mismatch %lu vs %u", arr.size(), tunnelmap.count);
        throw std::runtime_error("tunnel map count mismatch");
    }

    tunnelmap.list = sai_alloc_n_of_ptr_type(tunnelmap.count, tunnelmap.list);

    for (uint32_t i = 0; i < tunnelmap.count; ++i)
    {
        const json& item = arr[i];

        sai_deserialize_tunnel_map(item, tunnelmap.list[i]);
    }
}

void sai_deserialize_ipv6(
        _In_ const std::string& s,
        _Out_ sai_ip6_t& ipaddr)
{
    SWSS_LOG_ENTER();

    if (inet_pton(AF_INET6, s.c_str(), ipaddr) != 1)
    {
        SWSS_LOG_ERROR("invalid ip address %s", s.c_str());
        throw std::runtime_error("invalid ip addredd");
    }
}

void sai_deserialize_ipv4(
        _In_ const std::string& s,
        _Out_ sai_ip4_t& ipaddr)
{
    SWSS_LOG_ENTER();

    if (inet_pton(AF_INET, s.c_str(), &ipaddr) != 1)
    {
        SWSS_LOG_ERROR("invalid ip address %s", s.c_str());
        throw std::runtime_error("invalid ip address");
    }
}

void sai_deserialize_ip_address(
        _In_ const std::string& s,
        _Out_ sai_ip_address_t& ipaddr)
{
    SWSS_LOG_ENTER();

    if (inet_pton(AF_INET, s.c_str(), &ipaddr.addr.ip4) == 1)
    {
        ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
        return;
    }

    if (inet_pton(AF_INET6, s.c_str(), ipaddr.addr.ip6) == 1)
    {
        ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
        return;
    }

    SWSS_LOG_ERROR("invalid ip address %s", s.c_str());
    throw std::runtime_error("invalid ip address");
}

template <typename T>
void sai_deserialize_range(
        _In_ const std::string& s,
        _Out_ T& range)
{
    SWSS_LOG_ENTER();

    auto tokens = swss::tokenize(s, ',');

    if (tokens.size() != 2)
    {
        SWSS_LOG_ERROR("invalid range %s", s.c_str());
        throw std::runtime_error("invalid range");
    }

    sai_deserialize_number(tokens[0], range.min);
    sai_deserialize_number(tokens[1], range.max);
}

void sai_deserialize_acl_field(
        _In_ const std::string& s,
        _In_ const sai_attr_metadata_t& meta,
        _In_ sai_acl_field_data_t& field,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    if (s == "disabled")
    {
        field.enable = false;
        return;
    }

    field.enable = true;

    auto pos = s.find("&mask:");

    std::string sfield;
    std::string smask;

    if (pos == std::string::npos)
    {
        sfield = s;
    }
    else
    {
        sfield = s.substr(0, pos);
        smask = s.substr(pos + 6); // 6 = "&mask:" length
    }

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
            return sai_deserialize_bool(sfield, field.data.booldata);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
            sai_deserialize_number(sfield, field.data.u8);
            sai_deserialize_number(smask, field.mask.u8, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
            sai_deserialize_number(sfield, field.data.s8);
            sai_deserialize_number(smask, field.mask.s8, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
            sai_deserialize_number(sfield, field.data.u16);
            sai_deserialize_number(smask, field.mask.u16, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
            sai_deserialize_number(sfield, field.data.s16);
            sai_deserialize_number(smask, field.mask.s16, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
            sai_deserialize_number(sfield, field.data.u32);
            sai_deserialize_number(smask, field.mask.u32, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
            sai_deserialize_enum(sfield, meta.enummetadata, field.data.s32);
            sai_deserialize_number(smask, field.mask.s32, true);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
            sai_deserialize_mac(sfield, field.data.mac);
            sai_deserialize_mac(smask, field.mask.mac);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
            sai_deserialize_ipv4(sfield, field.data.ip4);
            sai_deserialize_ipv4(smask, field.mask.ip4);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
            sai_deserialize_ipv6(sfield, field.data.ip6);
            sai_deserialize_ipv6(smask, field.mask.ip6);
            return;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
            return sai_deserialize_object_id(sfield, field.data.oid);

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
            return sai_deserialize_oid_list(sfield, field.data.objlist, countOnly);

               case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
               return sai_deserialize_number_list(field.data.u8list, countOnly) + "&mask:" + sai_deserialize_uint8_hex_list(field.mask.u8list, countOnly);

        default:
            SWSS_LOG_ERROR("FATAIL: invalid serialization type %d", meta.serializationtype);
            throw std::runtime_error("serialization type is not supported");
    }
}

void sai_deserialize_acl_action(
        _In_ const std::string& s,
        _In_ const sai_attr_metadata_t& meta,
        _In_ sai_acl_action_data_t& action,
        _In_ bool countOnly)
{
    SWSS_LOG_ENTER();

    if (s == "disabled")
    {
        action.enable = false;
        return;
    }

    action.enable = true;

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
            return sai_deserialize_number(s, action.parameter.u8);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
            return sai_deserialize_number(s, action.parameter.s8);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
            return sai_deserialize_number(s, action.parameter.u16);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
            return sai_deserialize_number(s, action.parameter.s16);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
            return sai_deserialize_number(s, action.parameter.u32);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
            return sai_deserialize_enum(s, meta.enummetadata, action.parameter.s32);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
            return sai_deserialize_mac(s, action.parameter.mac);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
            return sai_deserialize_ipv4(s, action.parameter.ip4);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
            return sai_deserialize_ipv6(s, action.parameter.ip6);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
            return sai_deserialize_object_id(s, action.parameter.oid);

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            return sai_deserialize_oid_list(s, action.parameter.objlist, countOnly);

        default:
            SWSS_LOG_ERROR("FATAIL: invalid serialization type %d", meta.serializationtype);
            throw std::runtime_error("serialization type is not supported");
    }
}

void sai_deserialize_attr_value(
        _In_ const std::string& s,
        _In_ const sai_attr_metadata_t& meta,
        _Out_ sai_attribute_t &attr,
        _In_ const bool countOnly)
{
    SWSS_LOG_ENTER();

    memset(&attr.value, 0, sizeof(attr.value));

    switch (meta.serializationtype)
    {
        case SAI_SERIALIZATION_TYPE_BOOL:
            return sai_deserialize_bool(s, attr.value.booldata);

        case SAI_SERIALIZATION_TYPE_CHARDATA:
            return sai_deserialize_chardata(s, attr.value.chardata);

        case SAI_SERIALIZATION_TYPE_UINT8:
            return sai_deserialize_number(s, attr.value.u8);

        case SAI_SERIALIZATION_TYPE_INT8:
            return sai_deserialize_number(s, attr.value.s8);

        case SAI_SERIALIZATION_TYPE_UINT16:
            return sai_deserialize_number(s, attr.value.u16);

        case SAI_SERIALIZATION_TYPE_INT16:
            return sai_deserialize_number(s, attr.value.s16);

        case SAI_SERIALIZATION_TYPE_UINT32:
            return sai_deserialize_number(s, attr.value.u32);

        case SAI_SERIALIZATION_TYPE_INT32:
            return sai_deserialize_enum(s, meta.enummetadata, attr.value.s32);

        case SAI_SERIALIZATION_TYPE_UINT64:
            return sai_deserialize_number(s, attr.value.u64);

        case SAI_SERIALIZATION_TYPE_INT64:
            return sai_deserialize_number(s, attr.value.s64);

        case SAI_SERIALIZATION_TYPE_MAC:
            return sai_deserialize_mac(s, attr.value.mac);

        case SAI_SERIALIZATION_TYPE_IP4:
            return sai_deserialize_ipv4(s, attr.value.ip4);

        case SAI_SERIALIZATION_TYPE_IP6:
            return sai_deserialize_ipv6(s, attr.value.ip6);

        case SAI_SERIALIZATION_TYPE_IP_ADDRESS:
            return sai_deserialize_ip_address(s, attr.value.ipaddr);

        case SAI_SERIALIZATION_TYPE_OBJECT_ID:
            return sai_deserialize_object_id(s, attr.value.oid);

        case SAI_SERIALIZATION_TYPE_OBJECT_LIST:
            return sai_deserialize_oid_list(s, attr.value.objlist, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT8_LIST:
            return sai_deserialize_number_list(s, attr.value.u8list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT8_LIST:
            return sai_deserialize_number_list(s, attr.value.s8list, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT16_LIST:
            return sai_deserialize_number_list(s, attr.value.u16list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT16_LIST:
            return sai_deserialize_number_list(s, attr.value.s16list, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT32_LIST:
            return sai_deserialize_number_list(s, attr.value.u32list, countOnly);

        case SAI_SERIALIZATION_TYPE_INT32_LIST:
            return sai_deserialize_enum_list(s, meta.enummetadata, attr.value.s32list, countOnly);

        case SAI_SERIALIZATION_TYPE_UINT32_RANGE:
            return sai_deserialize_range(s, attr.value.u32range);

        case SAI_SERIALIZATION_TYPE_INT32_RANGE:
            return sai_deserialize_range(s, attr.value.s32range);

        case SAI_SERIALIZATION_TYPE_VLAN_LIST:
            return sai_deserialize_number_list(s, attr.value.vlanlist, countOnly);

        case SAI_SERIALIZATION_TYPE_QOS_MAP_LIST:
            return sai_deserialize_qos_map_list(s, attr.value.qosmap, countOnly);

        case SAI_SERIALIZATION_TYPE_TUNNEL_MAP_LIST:
            return sai_deserialize_tunnel_map_list(s, attr.value.tunnelmap, countOnly);

            // ACL FIELD DATA

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            return sai_deserialize_acl_field(s, meta, attr.value.aclfield, countOnly);

            // ACL ACTION DATA

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            return sai_deserialize_acl_action(s, meta, attr.value.aclaction, countOnly);

        default:
            SWSS_LOG_ERROR("deserialize type %d is not supportd yet FIXME", meta.serializationtype);
            throw std::runtime_error("deserialize type is not supported yet FIXME");
    }
}

void sai_deserialize_ip_prefix(
        _In_ const std::string &s,
        _Out_ sai_ip_prefix_t &ip_prefix)
{
    SWSS_LOG_ENTER();

    auto tokens = swss::tokenize(s, '/');

    if (tokens.size() != 2)
    {
        SWSS_LOG_ERROR("invalid ip prefix %s", s.c_str());
        throw std::runtime_error("invalid ip prefix");
    }

    uint8_t mask;
    sai_deserialize_number(tokens[1], mask);

    const std::string &ip = tokens[0];

    if (inet_pton(AF_INET, ip.c_str(), &ip_prefix.addr.ip4) == 1)
    {
        ip_prefix.addr_family = SAI_IP_ADDR_FAMILY_IPV4;

        sai_populate_ip_mask(mask, (uint8_t*)&ip_prefix.mask.ip4, false);
    }
    else if (inet_pton(AF_INET6, ip.c_str(), ip_prefix.addr.ip6) == 1)
    {
        ip_prefix.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

        sai_populate_ip_mask(mask, ip_prefix.mask.ip6, true);
    }
    else
    {
        SWSS_LOG_ERROR("invalid ip prefix %s", s.c_str());
        throw std::runtime_error("invalid ip prefix");
    }
}

void sai_deserialize_status(
        _In_ const std::string& s,
        _Out_ sai_status_t& status)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_status_t, status);
}

void sai_deserialize_port_oper_status(
        _In_ const std::string& s,
        _Out_ sai_port_oper_status_t& status)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_port_oper_status_t, (int32_t&)status);
}

void sai_deserialize_fdb_event(
        _In_ const std::string& s,
        _Out_ sai_fdb_event_t& event)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_fdb_event_t, (int32_t&)event);
}

void sai_deserialize_switch_oper_status(
        _In_ const std::string& s,
        _Out_ sai_switch_oper_status_t& status)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_switch_oper_status_t, (int32_t&)status);
}

void sai_deserialize_port_event(
        _In_ const std::string& s,
        _Out_ sai_port_event_t& port_event)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_port_event_t, (int32_t&)port_event);
}

void sai_deserialize_hostif_trap_id(
        _In_ const std::string& s,
        _In_ sai_hostif_trap_id_t& hostif_trap_id)
{
    SWSS_LOG_ENTER();

    sai_deserialize_enum(s, &metadata_enum_sai_hostif_trap_type_t, (int32_t&)hostif_trap_id);
}

void sai_deserialize_object_type(
        _In_ const std::string& s,
        _Out_ sai_object_type_t& object_type)
{
    sai_deserialize_enum(s, &metadata_enum_sai_object_type_t, (int32_t&)object_type);
}

void sai_deserialize_vlan_id(
        _In_ const std::string& s,
        _In_ sai_vlan_id_t& vlan_id)
{
    SWSS_LOG_ENTER();

    sai_deserialize_number(s, vlan_id);
}

void sai_deserialize_fdb_entry(
        _In_ const std::string &s,
        _Out_ sai_fdb_entry_t &fdb_entry)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    sai_deserialize_mac(j["mac"], fdb_entry.mac_address);
    sai_deserialize_vlan_id(j["vlan"], fdb_entry.vlan_id);
}

void sai_deserialize_neighbor_entry(
        _In_ const std::string& s,
        _In_ sai_neighbor_entry_t &ne)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    sai_deserialize_object_id(j["rif"], ne.rif_id);
    sai_deserialize_ip_address(j["ip"], ne.ip_address);
}

void sai_deserialize_route_entry(
        _In_ const std::string &s,
        _Out_ sai_unicast_route_entry_t& route_entry)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    sai_deserialize_object_id(j["vr"], route_entry.vr_id);
    sai_deserialize_ip_prefix(j["dest"], route_entry.destination);
}

void sai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ const sai_attr_metadata_t** meta)
{
    SWSS_LOG_ENTER();

    if (meta == NULL)
    {
        SWSS_LOG_ERROR("meta pointer is null");
        throw std::runtime_error("meta pointer is null");
    }

    auto it = AttributesIdMetadata.find(s);

    if (it == AttributesIdMetadata.end())
    {
        if (AttributesIdMetadata.size() == 0)
        {
            SWSS_LOG_ERROR("metadata is not initialized");
        }

        SWSS_LOG_ERROR("invalid attr id: %s", s.c_str());
        throw std::runtime_error("invalid attr id");
    }

    *meta = it->second;
}

void sai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ sai_attr_id_t& attrid)
{
    SWSS_LOG_ENTER();

    const sai_attr_metadata_t *meta = NULL;

    sai_deserialize_attr_id(s, &meta);

    attrid = meta->attrid;
}

// deserialize ntf

void sai_deserialize_json_fdb_event_notification_data(
        _In_ const json& j,
        _Out_ sai_fdb_event_notification_data_t& fdb)
{
    SWSS_LOG_ENTER();

    sai_deserialize_fdb_event(j["fdb_event"], fdb.event_type);
    sai_deserialize_fdb_entry(j["fdb_entry"], fdb.fdb_entry);

    json arr = j["list"];

    fdb.attr_count = (uint32_t)arr.size();
    fdb.attr = new sai_attribute_t[fdb.attr_count];

    for (uint32_t i = 0; i < fdb.attr_count; ++i)
    {
        const json &item = arr[i];

        const sai_attr_metadata_t *meta = NULL;

        sai_deserialize_attr_id(item["id"], &meta);

        fdb.attr[i].id = meta->attrid;

        sai_deserialize_attr_value(item["value"], *meta, fdb.attr[i]);
    }
}

void sai_deserialize_fdb_event_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_fdb_event_notification_data_t** fdb_event)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    count = (uint32_t)j.size();

    auto data = new sai_fdb_event_notification_data_t[count];

    for (uint32_t i = 0; i < count; ++i)
    {
        sai_deserialize_json_fdb_event_notification_data(j[i], data[i]);
    }

    *fdb_event = data;
}

void sai_deserialize_port_oper_status_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_port_oper_status_notification_t** port_oper_status)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    count = (uint32_t)j.size();

    auto data = new sai_port_oper_status_notification_t[count];

    for (uint32_t i = 0; i < count; ++i)
    {
        sai_deserialize_object_id(j[i]["port_id"], data[i].port_id);
        sai_deserialize_port_oper_status(j[i]["port_state"], data[i].port_state);
    }

    *port_oper_status = data;
}

void sai_deserialize_port_event_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_port_event_notification_t** port_event)
{
    SWSS_LOG_ENTER();

    json j = json::parse(s);

    count = (uint32_t)j.size();

    auto data = new sai_port_event_notification_t[count];

    for (uint32_t i = 0; i < count; ++i)
    {
        sai_deserialize_object_id(j[i]["port_id"], data[i].port_id);
        sai_deserialize_port_event(j[i]["port_event"], data[i].port_event);
    }

    *port_event = data;
}

// deserialize free

void sai_deserialize_free_attribute_value(
        _In_ const sai_attr_serialization_type_t type,
        _In_ sai_attribute_t &attr)
{
    SWSS_LOG_ENTER();

    // if we allocated list, then we need to free it

    switch (type)
    {
        case SAI_SERIALIZATION_TYPE_BOOL:
        case SAI_SERIALIZATION_TYPE_CHARDATA:
        case SAI_SERIALIZATION_TYPE_UINT8:
        case SAI_SERIALIZATION_TYPE_INT8:
        case SAI_SERIALIZATION_TYPE_UINT16:
        case SAI_SERIALIZATION_TYPE_INT16:
        case SAI_SERIALIZATION_TYPE_UINT32:
        case SAI_SERIALIZATION_TYPE_INT32:
        case SAI_SERIALIZATION_TYPE_UINT64:
        case SAI_SERIALIZATION_TYPE_INT64:
        case SAI_SERIALIZATION_TYPE_MAC:
        case SAI_SERIALIZATION_TYPE_IP4:
        case SAI_SERIALIZATION_TYPE_IP6:
        case SAI_SERIALIZATION_TYPE_IP_ADDRESS:
        case SAI_SERIALIZATION_TYPE_OBJECT_ID:
            break;

        case SAI_SERIALIZATION_TYPE_OBJECT_LIST:
            sai_free_list(attr.value.objlist);
            break;

        case SAI_SERIALIZATION_TYPE_UINT8_LIST:
            sai_free_list(attr.value.u8list);
            break;

        case SAI_SERIALIZATION_TYPE_INT8_LIST:
            sai_free_list(attr.value.s8list);
            break;

        case SAI_SERIALIZATION_TYPE_UINT16_LIST:
            sai_free_list(attr.value.u16list);
            break;

        case SAI_SERIALIZATION_TYPE_INT16_LIST:
            sai_free_list(attr.value.s16list);
            break;

        case SAI_SERIALIZATION_TYPE_UINT32_LIST:
            sai_free_list(attr.value.u32list);
            break;

        case SAI_SERIALIZATION_TYPE_INT32_LIST:
            sai_free_list(attr.value.s32list);
            break;

        case SAI_SERIALIZATION_TYPE_UINT32_RANGE:
        case SAI_SERIALIZATION_TYPE_INT32_RANGE:
            break;

        case SAI_SERIALIZATION_TYPE_VLAN_LIST:
            sai_free_list(attr.value.vlanlist);
            break;

        case SAI_SERIALIZATION_TYPE_QOS_MAP_LIST:
            sai_free_list(attr.value.qosmap);
            break;

        case SAI_SERIALIZATION_TYPE_TUNNEL_MAP_LIST:
            sai_free_list(attr.value.tunnelmap);
            break;

            // ACL FIELD DATA

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_BOOL:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP4:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_IP6:
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_ID:
            break;
        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
            sai_free_list(attr.value.aclfield.data.objlist);
            break;

        case SAI_SERIALIZATION_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            sai_free_list(attr.value.aclfield.mask.u8list);
            sai_free_list(attr.value.aclfield.data.u8list);
            break;

            // ACL ACTION DATA

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT8:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT16:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_UINT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_INT32:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_MAC:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV4:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_IPV6:
        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_ID:
            break;

        case SAI_SERIALIZATION_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            sai_free_list(attr.value.aclaction.parameter.objlist);
            break;

        default:
            SWSS_LOG_ERROR("unsupported type %d on deserialize free, FIXME", type);
            throw std::runtime_error("unsupported type on deserialize free, FIXME");
    }
}

// deserialize free ntf

void sai_deserialize_free_fdb_event(
        _In_ sai_fdb_event_notification_data_t& fdb_event)
{
    SWSS_LOG_ENTER();

    for (uint32_t i = 0; i < fdb_event.attr_count; ++i)
    {
        auto meta = get_attribute_metadata(SAI_OBJECT_TYPE_FDB, fdb_event.attr[i].id);

        if (meta == NULL)
        {
            SWSS_LOG_ERROR("unable to get metadata for object type %x, attribute %x", SAI_OBJECT_TYPE_FDB, fdb_event.attr[i].id);
            throw std::runtime_error("unable to get metadata");
        }

        sai_deserialize_free_attribute_value(meta->serializationtype, fdb_event.attr[i]);
    }

    delete fdb_event.attr;
}

void sai_deserialize_free_fdb_event_ntf(
        _In_ uint32_t count,
        _In_ sai_fdb_event_notification_data_t* fdb_event)
{
    SWSS_LOG_ENTER();

    for (uint32_t i = 0; i < count; ++i)
    {
        sai_deserialize_free_fdb_event(fdb_event[i]);
    }

    delete fdb_event;
}

void sai_deserialize_free_port_oper_status_ntf(
        _In_ uint32_t count,
        _In_ sai_port_oper_status_notification_t* port_oper_status)
{
    SWSS_LOG_ENTER();

    delete port_oper_status;
}

void sai_deserialize_free_port_event_ntf(
        _In_ uint32_t count,
        _In_ sai_port_event_notification_t* port_event)
{
    SWSS_LOG_ENTER();

    delete port_event;
}

// util

sai_status_t transfer_attributes(
        _In_ sai_object_type_t object_type,
        _In_ uint32_t attr_count,
        _In_ sai_attribute_t *src_attr_list,
        _In_ sai_attribute_t *dst_attr_list,
        _In_ bool countOnly = false);

// serialize

std::string sai_serialize_ip_address(
        _In_ const sai_ip_address_t &ip_address);

std::string sai_serialize_ip_prefix(
        _In_ const sai_ip_prefix_t &ip_prefix);

std::string sai_serialize_neighbor_entry(
        _In_ const sai_neighbor_entry_t &neighbor_entry);

std::string sai_serialize_route_entry(
        _In_ const sai_unicast_route_entry_t &route_entry);

std::string sai_serialize_fdb_entry(
        _In_ const sai_fdb_entry_t &fdb_entry);

std::string sai_serialize_hostif_trap_id(
        _In_ const sai_hostif_trap_id_t hostif_trap_id);

std::string sai_serialize_vlan_id(
        _In_ const sai_vlan_id_t vlan_id);

std::string sai_serialize_object_type(
        _In_ const sai_object_type_t object_type);

std::string sai_serialize_attr_value(
        _In_ const sai_attr_metadata_t& meta,
        _In_ const sai_attribute_t &attr,
        _In_ const bool countOnly = false);

std::string sai_serialize_status(
        _In_ const sai_status_t status);

std::string sai_serialize_port_stat(
        _In_ const sai_port_stat_counter_t counter);

std::string sai_serialize_switch_oper_status(
        _In_ sai_switch_oper_status_t status);

std::string sai_serialize_enum(
        _In_ const int32_t value,
        _In_ const sai_enum_metadata_t* meta);

std::string sai_serialize_number(
        _In_ uint32_t number,
        _In_ bool hex = false);

std::string sai_serialize_attr_id(
        _In_ const sai_attr_metadata_t& meta);

// serialize ntf

std::string sai_serialize_fdb_event_ntf(
        _In_ uint32_t count,
        _In_ const sai_fdb_event_notification_data_t* fdb_event);

std::string sai_serialize_port_oper_status_ntf(
        _In_ uint32_t count,
        _In_ const sai_port_oper_status_notification_t* port_oper_status);

std::string sai_serialize_port_event_ntf(
        _In_ uint32_t count,
        _In_ const sai_port_event_notification_t* port_event);

// deserialize

void sai_deserialize_number(
        _In_ const std::string& s,
        _Out_ uint32_t& number,
        _In_ bool hex = false);

void sai_deserialize_status(
        _In_ const std::string& s,
        _Out_ sai_status_t& status);

void sai_deserialize_switch_oper_status(
        _In_ const std::string& s,
        _Out_ sai_switch_oper_status_t& status);

void sai_deserialize_object_type(
        _In_ const std::string& s,
        _Out_ sai_object_type_t& object_type);

void sai_deserialize_object_id(
        _In_ const std::string& s,
        _Out_ sai_object_id_t& oid);

void sai_deserialize_fdb_entry(
        _In_ const std::string& s,
        _In_ sai_fdb_entry_t &fdb_entry);

void sai_deserialize_neighbor_entry(
        _In_ const std::string& s,
        _In_ sai_neighbor_entry_t &neighbor_entry);

void sai_deserialize_route_entry(
        _In_ const std::string& s,
        _In_ sai_unicast_route_entry_t &route_entry);

void sai_deserialize_vlan_id(
        _In_ const std::string& s,
        _In_ sai_vlan_id_t& vlan_id);

void sai_deserialize_hostif_trap_id(
        _In_ const std::string& s,
        _In_ sai_hostif_trap_id_t& hostif_trap_id);

void sai_deserialize_attr_value(
        _In_ const std::string& s,
        _In_ const sai_attr_metadata_t& meta,
        _Out_ sai_attribute_t &attr,
        _In_ const bool countOnly = false);

void sai_deserialize_attr_id(
        _In_ const std::string& s,
        _Out_ sai_attr_id_t &attrid);

void sai_deserialize_attr_id(
        _In_ const std::string& s,
        _In_ const sai_attr_metadata_t** meta);

// deserialize ntf

void sai_deserialize_fdb_event_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_fdb_event_notification_data_t** fdbdata);

void sai_deserialize_port_oper_status_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_port_oper_status_notification_t** portoperstatus);

void sai_deserialize_port_event_ntf(
        _In_ const std::string& s,
        _Out_ uint32_t &count,
        _Out_ sai_port_event_notification_t** portoperstatus);

// free methods

void sai_deserialize_free_attribute_value(
        _In_ const sai_attr_serialization_type_t type,
        _In_ sai_attribute_t &attr);

// deserialize free ntf

void sai_deserialize_free_fdb_event_ntf(
        _In_ uint32_t count,
        _In_ sai_fdb_event_notification_data_t* fdbdata);

void sai_deserialize_free_port_oper_status_ntf(
        _In_ uint32_t count,
        _In_ sai_port_oper_status_notification_t* portoperstatus);

void sai_deserialize_free_port_event_ntf(
        _In_ uint32_t count,
        _In_ sai_port_event_notification_t* portoperstatus);

#endif // __SAI_SERIALIZE__
*/
