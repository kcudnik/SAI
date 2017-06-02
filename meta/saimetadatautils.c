#include <stdio.h>
#include <string.h>
#include <sai.h>
#include "saimetadatautils.h"
#include "saimetadata.h"

bool sai_metadata_is_allowed_object_type(
        _In_ const sai_attr_metadata_t* metadata,
        _In_ sai_object_type_t object_type)
{
    if (metadata == NULL || metadata->allowedobjecttypes == NULL)
    {
        return false;
    }

    size_t i = 0;

    for (; i < metadata->allowedobjecttypeslength; ++i)
    {
        if (metadata->allowedobjecttypes[i] == object_type)
        {
            return true;
        }
    }

    return false;
}

bool sai_metadata_is_allowed_enum_value(
        _In_ const sai_attr_metadata_t* metadata,
        _In_ int value)
{
    if (metadata == NULL || metadata->enummetadata == NULL)
    {
        return false;
    }

    size_t i = 0;

    const sai_enum_metadata_t *emd = metadata->enummetadata;

    for (; i < emd->valuescount; ++i)
    {
        if (emd->values[i] == value)
        {
            return true;
        }
    }

    return false;
}

const sai_attr_metadata_t* sai_metadata_get_attr_metadata(
        _In_ sai_object_type_t objecttype,
        _In_ sai_attr_id_t attrid)
{
    if (sai_metadata_is_object_type_valid(objecttype))
    {
        const sai_attr_metadata_t* const* const md = sai_metadata_attr_by_object_type[objecttype];

        size_t index = 0;

        for (; md[index] != NULL; index++)
        {
            if (md[index]->attrid == attrid)
            {
                return md[index];
            }
        }
    }

    return NULL;
}

const sai_attr_metadata_t* sai_metadata_get_attr_metadata_by_attr_id_name(
        _In_ const char *attr_id_name)
{
    if (attr_id_name == NULL)
    {
        return NULL;
    }

    /* use binary search */

    ssize_t first = 0;
    ssize_t last = (ssize_t)(sai_metadata_attr_sorted_by_id_name_count - 1);

    while (first <= last)
    {
        ssize_t middle = (first + last) / 2;

        int res = strcmp(attr_id_name, sai_metadata_attr_sorted_by_id_name[middle]->attridname);

        if (res > 0)
        {
            first = middle + 1;
        }
        else if (res < 0)
        {
            last = middle - 1;
        }
        else
        {
            /* found */

            return sai_metadata_attr_sorted_by_id_name[middle];
        }
    }

    /* not found */

    return NULL;
}

const char* sai_metadata_get_enum_value_name(
        _In_ const sai_enum_metadata_t* metadata,
        _In_ int value)
{
    if (metadata == NULL)
    {
        return NULL;
    }

    size_t i = 0;

    for (; i < metadata->valuescount; ++i)
    {
        if (metadata->values[i] == value)
        {
            return metadata->valuesnames[i];
        }
    }

    return NULL;
}

const sai_attribute_t* sai_metadata_get_attr_by_id(
        _In_ sai_attr_id_t id,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    if (attr_list == NULL)
    {
        return NULL;
    }

    uint32_t i = 0;

    for (; i < attr_count; ++i)
    {
        if (attr_list[i].id == id)
        {
            return &attr_list[i];
        }
    }

    return NULL;
}

const sai_object_type_info_t* sai_metadata_get_object_type_info(
        _In_ sai_object_type_t object_type)
{
    if (sai_metadata_is_object_type_valid(object_type))
    {
        return sai_metadata_all_object_type_infos[object_type];
    }

    return NULL;
}

bool sai_metadata_is_object_type_valid(
        _In_ sai_object_type_t object_type)
{
    return object_type > SAI_OBJECT_TYPE_NULL && object_type < SAI_OBJECT_TYPE_MAX;
}

bool sai_metadata_is_condition_in_force(
        _In_ const sai_attr_metadata_t *metadata,
        _In_ uint32_t attr_count,
        _In_ const sai_attribute_t *attr_list)
{
    if (metadata == NULL || !metadata->isconditional || attr_list == NULL)
    {
        return false;
    }

    size_t idx = 0;

    bool met = (metadata->conditiontype == SAI_ATTR_CONDITION_TYPE_AND);

    for (; idx < metadata->conditionslength; ++idx)
    {
        const sai_attr_condition_t *condition = metadata->conditions[idx];

        /*
         * Conditons may only be on the same object type.
         *
         * Default value may not exists if conditional object is marked as
         * MANDATORY_ON_CREATE.
         */

        const sai_attr_metadata_t *cmd = sai_metadata_get_attr_metadata(metadata->objecttype, condition->attrid);

        const sai_attribute_t *cattr = sai_metadata_get_attr_by_id(condition->attrid, attr_count, attr_list);

        const sai_attribute_value_t* cvalue = NULL;

        if (cattr == NULL)
        {
            /*
             * User didn't passed conditional attribute, so check if there is
             * default value.
             */

            cvalue = cmd->defaultvalue;
        }
        else
        {
            cvalue = &cattr->value;
        }

        if (cvalue == NULL)
        {
            /*
             * There is no default value and user didn't passed attribute.
             */

            if (metadata->conditiontype == SAI_ATTR_CONDITION_TYPE_AND)
            {
                return false;
            }

            continue;
        }

        bool current = false;

        switch (cmd->attrvaluetype)
        {
            case SAI_ATTR_VALUE_TYPE_BOOL:
                current = (condition->condition.booldata == cvalue->booldata);
                break;
            case SAI_ATTR_VALUE_TYPE_INT8:
                current = (condition->condition.s8 == cvalue->s8);
                break;
            case SAI_ATTR_VALUE_TYPE_INT16:
                current = (condition->condition.s16 == cvalue->s16);
                break;
            case SAI_ATTR_VALUE_TYPE_INT32:
                current = (condition->condition.s32 == cvalue->s32);
                break;
            case SAI_ATTR_VALUE_TYPE_INT64:
                current = (condition->condition.s64 == cvalue->s64);
                break;
            case SAI_ATTR_VALUE_TYPE_UINT8:
                current = (condition->condition.u8 == cvalue->u8);
                break;
            case SAI_ATTR_VALUE_TYPE_UINT16:
                current = (condition->condition.u16 == cvalue->u16);
                break;
            case SAI_ATTR_VALUE_TYPE_UINT32:
                current = (condition->condition.u32 == cvalue->u32);
                break;
            case SAI_ATTR_VALUE_TYPE_UINT64:
                current = (condition->condition.u64 == cvalue->u64);
                break;

            default:

                /*
                 * We should never get here since sanity check tests all
                 * attributes and all conditions.
                 */

                SAI_META_LOG_ERROR("condition value type %d is not supported, FIXME", cmd->attrvaluetype);

                return false;
        }

        if (metadata->conditiontype == SAI_ATTR_CONDITION_TYPE_AND)
        {
            met &= current;
        }
        else /* OR */
        {
            met |= current;
        }
    }

    return met;
}

#define CASE_PRIMITIVE(x,y)                 \
    case x:                                 \
    dst_attr.value. #y = src_attr.value #y; \
    return SAI_STATUS_SUCCESS;

sai_status_t sai_metadata_transfer_attribute_value(
        _In_ const sai_attr_metadata_t *metadata,
        _In_ const sai_attribute_t *src_attr,
        _Inout_ sai_attribute_t *dst_attr,
        _In_ bool count_only)
{

    /* 
     * We need serialize/deserialize for testing,
     * complex types could be generated automaticly since we are using plane structs
     */
    
    /* is primitive we could just transfer entrie stricture memcpy, ~ 40 bytes */
    /* lists are generic, so only w list will handle all
     * complex data need to be handles separetly - so far only acl capability
     * */

    switch (serialization_type)
    {
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_BOOL, booldata);

        case SAI_ATTR_VALUE_TYPE_CHARDATA:
            memcpy(dst_attr.value.chardata, src_attr.value.chardata, sizeof(src_attr.value.chardata));
            return SAI_STATUS_SUCCESS;

        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_UINT8,  u8);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_INT8,   s8);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_UINT16, u16);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_INT16,  s16);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_UINT32, u32);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_INT32,  s32);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_UINT64, u64);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_INT64,  s64);

        case SAI_ATTR_VALUE_TYPE_MAC:
            memcpy(dst_attr.value.mac, src_attr.value.mac, sizeof(src_attr.value.mac));
            break;

        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_IPV4,  ip4);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_IPV6,  ip6);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_IP_ADDRESS,  ipaddr);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_OBJECT_ID,   oid);

        case SAI_ATTR_VALUE_TYPE_OBJECT_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.objlist, dst_attr.value.objlist, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_UINT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u8list, dst_attr.value.u8list, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_INT8_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s8list, dst_attr.value.s8list, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_UINT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u16list, dst_attr.value.u16list, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_INT16_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s16list, dst_attr.value.s16list, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_UINT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.u32list, dst_attr.value.u32list, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_INT32_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.s32list, dst_attr.value.s32list, countOnly));
            break;

        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_UINT32_RANGE, u32range);
        CASE_PRIMITIVE(SAI_ATTR_VALUE_TYPE_INT32_RANGE,  s32range);

        case SAI_ATTR_VALUE_TYPE_VLAN_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.vlanlist, dst_attr.value.vlanlist, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_QOS_MAP_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.qosmap, dst_attr.value.qosmap, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_TUNNEL_MAP_LIST:
            RETURN_ON_ERROR(transfer_list(src_attr.value.tunnelmap, dst_attr.value.tunnelmap, countOnly));
            break;

            /* we need to check enable since if disabled, list may not exists */
            /* ACL FIELD DATA */

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_BOOL:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.data.booldata, dst_attr.value.aclfield.data.booldata);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT8:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u8, dst_attr.value.aclfield.mask.u8);
            transfer_primitive(src_attr.value.aclfield.data.u8, dst_attr.value.aclfield.data.u8);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT8:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s8, dst_attr.value.aclfield.mask.s8);
            transfer_primitive(src_attr.value.aclfield.data.s8, dst_attr.value.aclfield.data.s8);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT16:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u16, dst_attr.value.aclfield.mask.u16);
            transfer_primitive(src_attr.value.aclfield.data.u16, dst_attr.value.aclfield.data.u16);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT16:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s16, dst_attr.value.aclfield.mask.s16);
            transfer_primitive(src_attr.value.aclfield.data.s16, dst_attr.value.aclfield.data.s16);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT32:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.u16, dst_attr.value.aclfield.mask.u16);
            transfer_primitive(src_attr.value.aclfield.data.u16, dst_attr.value.aclfield.data.u16);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_INT32:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.s32, dst_attr.value.aclfield.mask.s32);
            transfer_primitive(src_attr.value.aclfield.data.s32, dst_attr.value.aclfield.data.s32);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_MAC:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.mac, dst_attr.value.aclfield.mask.mac);
            transfer_primitive(src_attr.value.aclfield.data.mac, dst_attr.value.aclfield.data.mac);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV4:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.ip4, dst_attr.value.aclfield.mask.ip4);
            transfer_primitive(src_attr.value.aclfield.data.ip4, dst_attr.value.aclfield.data.ip4);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_IPV6:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.mask.ip6, dst_attr.value.aclfield.mask.ip6);
            transfer_primitive(src_attr.value.aclfield.data.ip6, dst_attr.value.aclfield.data.ip6);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_OBJECT_ID:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            transfer_primitive(src_attr.value.aclfield.data.oid, dst_attr.value.aclfield.data.oid);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_OBJECT_LIST:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclfield.data.objlist, dst_attr.value.aclfield.data.objlist, countOnly));
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_FIELD_DATA_UINT8_LIST:
            transfer_primitive(src_attr.value.aclfield.enable, dst_attr.value.aclfield.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclfield.mask.u8list, dst_attr.value.aclfield.mask.u8list, countOnly));
            transfer_list(src_attr.value.aclfield.data.u8list, dst_attr.value.aclfield.data.u8list, countOnly);
            break;

            /* ACL ACTION DATA */

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT8:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u8, dst_attr.value.aclaction.parameter.u8);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT8:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s8, dst_attr.value.aclaction.parameter.s8);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT16:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u16, dst_attr.value.aclaction.parameter.u16);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT16:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s16, dst_attr.value.aclaction.parameter.s16);
            break;


        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_UINT32:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.u32, dst_attr.value.aclaction.parameter.u32);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_INT32:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.s32, dst_attr.value.aclaction.parameter.s32);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_MAC:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.mac, dst_attr.value.aclaction.parameter.mac);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_IPV4:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.ip4, dst_attr.value.aclaction.parameter.ip4);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_IPV6:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.ip6, dst_attr.value.aclaction.parameter.ip6);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_OBJECT_ID:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            transfer_primitive(src_attr.value.aclaction.parameter.oid, dst_attr.value.aclaction.parameter.oid);
            break;

        case SAI_ATTR_VALUE_TYPE_ACL_ACTION_DATA_OBJECT_LIST:
            transfer_primitive(src_attr.value.aclaction.enable, dst_attr.value.aclaction.enable);
            RETURN_ON_ERROR(transfer_list(src_attr.value.aclaction.parameter.objlist, dst_attr.value.aclaction.parameter.objlist, countOnly));
            break;

        default:
            return SAI_STATUS_NOT_IMPLEMENTED;
    }
}
