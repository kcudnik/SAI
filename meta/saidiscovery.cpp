#include <stdlib.h>
#include <string.h>
#include <stdio.h>

extern "C" {
#include <sai.h>
#include "saimetadata.h"
}

#include <iostream>
#include <map>
#include <vector>
#include <set>

/*
 * We want to find all object present on the switch
 *
 * Depends on the attribute flags if not read only
 * we can build asic view, and mark some attributes
 * as removable/nonremovable
 */

#define MAX_ELEMENTS 0x10000

void discover(
        _In_ sai_object_id_t id)
{
    SAI_META_LOG_ENTER();

    static std::vector<std::set<sai_attr_id_t> > notWorthQuery(SAI_OBJECT_TYPE_MAX);

    static std::set<sai_object_id_t> processed;

    /*
     * NOTE: This method is only good after switch init
     * since we are making assumptions that tere are no
     * ACL after initialization.
     */

    if (id == SAI_NULL_OBJECT_ID)
    {
        return;
    }

    if (processed.find(id) != processed.end())
    {
        return;
    }

    processed.insert(id);

    sai_object_type_t ot = sai_object_type_query(id);

    if (ot == SAI_OBJECT_TYPE_NULL)
    {
        SAI_META_LOG_ERROR("id 0x%lx returned NULL object type", id);
        return; //  throw ?
    }

    // TODO this is new object ! worth saving somewhere

    const sai_object_type_info_t *info =
        sai_metadata_get_object_type_info(ot);

    /*
     * Since we know that we will be quering only oid object types
     * then we dont need meta key, but we need to add to metadata
     * pointers to only generic functions.
     */

    sai_object_meta_key_t mk = { .objecttype = ot, .objectkey = { .key = { .object_id = id } } };

    static std::vector<sai_object_id_t> vec(MAX_ELEMENTS);

    // for each attribute
    for (int idx = 0; info->attrmetadata[idx] != NULL; ++idx)
    {
        const sai_attr_metadata_t *md = info->attrmetadata[idx];

        if (notWorthQuery[ot].find(md->attrid) != notWorthQuery[ot].end())
        {
            continue;
        }

        /*
         * Note that we don't care about ACL object id's since
         * we assume that there are no ACLs on switch after init.
         */

        sai_attribute_t attr;

        attr.id = md->attrid;

        if (md->attrvaluetype == SAI_ATTR_VALUE_TYPE_OBJECT_ID)
        {
            if (md->defaultvaluetype == SAI_DEFAULT_VALUE_TYPE_CONST)
            {
                /*
                 * This means that default value for this object is
                 * SAI_NULL_OBJECT_ID, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                continue;
            }

            sai_status_t status = info->get(&mk, 1, &attr);

            if (status != SAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SAI_META_LOG_WARN("failed to get atribute %s: %d",
                        md->attridname,
                        status);

                if (!md->isconditional)
                {
                    if (md->objecttype == SAI_OBJECT_TYPE_SWITCH &&
                            md->attrid == SAI_SWITCH_ATTR_CPU_PORT)
                    {
                        /*
                         * Since cpu port differs from other ports, just skip
                         * not worth query, let other port define those.
                         */

                        continue;
                    }

                    notWorthQuery[ot].insert(md->attrid);
                }

                continue;
            }

            discover(attr.value.oid); // recursion
        }
        else if (md->attrvaluetype == SAI_ATTR_VALUE_TYPE_OBJECT_LIST)
        {
            if (md->defaultvaluetype == SAI_DEFAULT_VALUE_TYPE_EMPTY_LIST)
            {
                /*
                 * This means that default value for this object is
                 * empty list, since this is discovery after
                 * create, we don't need to query this attribute.
                 */

                continue;
            }

            attr.value.objlist.count = MAX_ELEMENTS;
            attr.value.objlist.list = vec.data();

            sai_status_t status = info->get(&mk, 1, &attr);

            if (status != SAI_STATUS_SUCCESS)
            {
                /*
                 * We failed to get value, maybe it's not supported ?
                 */

                SAI_META_LOG_WARN("failed to get %s: %d",
                        md->attridname,
                        status);

                if (!md->isconditional)
                {
                    notWorthQuery[ot].insert(md->attrid);
                }

                continue;
            }

            for (uint32_t i = 0; i < attr.value.objlist.count; ++i)
            {
                discover(attr.value.objlist.list[i]); // recursion
            }
        }
    }
}

int main()
{
    sai_object_id_t id = SAI_NULL_OBJECT_ID;

    // find all objects on the switch after init!
    discover(id);
}
