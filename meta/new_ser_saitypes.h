
typedef enum _sai_ip_addr_family_t
{
    SAI_IP_ADDR_FAMILY_IPV4,

    SAI_IP_ADDR_FAMILY_IPV6

} sai_ip_addr_family_t;

/**
 * @serialize skip
 */
typedef struct _sai_ip_address_t
{
    sai_ip_addr_family_t addr_family;
    union _ip_addr {
        sai_ip4_t ip4;
        sai_ip6_t ip6;
    } addr;
} sai_ip_address_t;

/**
 * @serialize skip <- will mean that user provided serialize for this
 *
 * pozniej mozna wygenerowac funkcje validacji:
 *
 * sai_is_valid_ip_prefix or sai_is_ip_prefix_valid(..);
 *
 */
typedef struct _sai_ip_prefix_t
{
    sai_ip_addr_family_t addr_family;
    union _ip_prefix_addr {
        sai_ip4_t ip4;
        sai_ip6_t ip6;
    } addr;
    union _ip_prefix_mask {
        /* @validonly addr_family == SAI_IP_ADDR_FAMILY_IPV4 */
        sai_ip4_t ip4;

        /* @validonly addr_family == SAI_IP_ADDR_FAMILY_IPV6 */
        sai_ip6_t ip6;
    } mask;
} sai_ip_prefix_t;

/**
 * @brief Defines a single ACL filter
 *
 * @param const sai_attr_metadata_t *meta
 *
 * @param sai_attr_value_type_t foo <- will add extra param to struct serialize
 * for compare and validonly this param compare or switch()
 * albo automatycznie dodac ten param jestli uzyty jest w validonly
 *
 * albo "extraparam" dodac tag, oczywiscie mozna by przeginac tutaj
 * i petle tworzyc 
 * 
 * also if contains lists <- then add count only ?
 * or generate separate methods with _count_only
 *
 * @note IPv4 and IPv6 Address expected in Network Byte Order
 */
typedef struct _sai_acl_field_data_t
{
    /**
     * @brief Match enable/disable
     */
    bool enable;

    /**
     * @brief Field match mask
     *
     * @validonly enable == true
     *
     * @condition/validonly meta->attrvaluetype (for distinct action)? ale w sai_attribute
     * moze byc kilka akcji dla acl_field/actiondata :/
     */
    union _mask {

        /* inside valionly must be distinct/uniq since only 1 valu can be used */

        /** this will be added as param to serialize: */
        /** @type sai_attr_value_type_t == SAI_ATTR_VALUT_TYPE_ACL_FIELD_OBJECT_ID  */
        /** @validonly foo == SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT8 */
        /** @validonly meta->attrvaluetype == SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT8 */
        /** @type SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT8 */
        sai_uint8_t u8;

        /** @type SAI_ATTR_VALUT_TYPE_ACL_FIELD_INT8 */
        sai_int8_t s8;

        /** @type SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT16 */
        /** @validonly meta->attrvaluetype == SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT16 */
        sai_uint16_t u16;
        sai_int16_t s16;
        sai_uint32_t u32;

        /* @ not enum !!! since this is mask */
        sai_int32_t s32; /* can be enum? */
        sai_mac_t mac;
        sai_ip4_t ip4;
        sai_ip6_t ip6;
        /** @type SAI_ATTR_VALUT_TYPE_ACL_FIELD_UINT16 */
        sai_u8_list_t u8list;
    } mask;

    /**
     * @brief Expected AND result using match mask above with packet field
     * value where applicable.
     *
     * @validonly enable == true
     */
    union _data {
        bool booldata;
        sai_uint8_t u8;
        sai_int8_t s8;
        sai_uint16_t u16;
        sai_int16_t s16;
        sai_uint32_t u32;

        /* @enum meta->enummetadata */
        sai_int32_t s32; /* enum */
        sai_mac_t mac;
        sai_ip4_t ip4;
        sai_ip6_t ip6;

        /** 
         * @type sai_attr_value_type_t == SAI_ATTR_VALUT_TYPE_ACL_FIELD_OBJECT_ID 
         * 
         * @objects meta->objecttypes ?
         */
        sai_object_id_t oid;

        /** @type SAI_ATTR_VALUT_TYPE_ACL_FIELD_OBJ_LIST */
        sai_object_list_t objlist;
        sai_u8_list_t u8list;
    } data;
} sai_acl_field_data_t;

/**
 * @brief Defines a single ACL action
 *
 * @param const sai_attr_metadata_t *meta
 * @note IPv4 and IPv6 Address expected in Network Byte Order
 */
typedef struct _sai_acl_action_data_t
{
    /**
     * @brief Action enable/disable
     */
    bool enable;

    /**
     * @brief Action parameter
     *
     * @validonly enable == true
     */
    union _parameter {
        sai_uint8_t u8;
        sai_int8_t s8;
        sai_uint16_t u16;
        sai_int16_t s16;
        sai_uint32_t u32;
        sai_int32_t s32; /* enum */
        sai_mac_t mac;
        sai_ip4_t ip4;
        sai_ip6_t ip6;
        sai_object_id_t oid;
        sai_object_list_t objlist;
    } parameter;

} sai_acl_action_data_t;

/**
 * @brief Structure for ACL attributes supported at each stage.
 * action_list alone is added now. Qualifier list can also be added
 * when needed.
 */
typedef struct _sai_acl_capability_t
{
    /**
     * @brief Output from get function.
     *
     * Flag indicating whether action list is mandatory for table creation.
     */
    bool is_action_list_mandatory;

    /**
     * @brief Output from get function.
     *
     * List of actions supported per stage from the sai_acl_table_action_list_t.
     * Max action list can be obtained using the #SAI_SWITCH_ATTR_MAX_ACL_ACTION_COUNT.
     *
     * @type sai_action_type_t <- for serialize/deserialize enum
     * @validonly is_action_list_mandatory == true
     */
    sai_s32_list_t action_list;
} sai_acl_capability_t;

/**
 * @brief Segment Routing Tag Length Value entry
 */
typedef struct _sai_tlv_t
{
    sai_tlv_type_t tlv_type;

    /* and since this is union, each validonly must be distinct
     * since only 1 value can be used on serialization 
     *
     * and on unions validonly will be required
     * ale w si_atribute is nto that simple
     * @validonly tlv_type
     *
     * deserialize sie latwo domysli bo typ jest i zostal zdeserializowany
     * juz wiec switch/case will work ale w przypadku acl acdion/field to niewiem cholera
     */
    union _entry {

        /** @validonly SAI_TLV_TYPE_INGRESS_NODE */
        sai_ip6_t ingress_node;

        /** @validonly SAI_TLV_TYPE_EGRESS_NODE */
        sai_ip6_t egress_node;

        /* ten jest bardzje rozszezalny bomozemy podac rozne parametery 
         * bo param bedzei sie nazywal tlv, wiec bezposrednio mozna to wkleic jako kod
         * vlidacja sai_\w+_t->\w+ == (SAI_|bool)
         * */
        /** @validonly tlv->tlv_type == SAI_TLV_TYPE_EGRESS_NODE */
        sai_uint32_t opaque_container[4];

        /** @validonly tlv->tlv_type == SAI_TLV_TYPE_EGRESS_NODE */
        sai_hmac_t hmac;
    } entry;
} sai_tlv_t;


/**
 * @brief Data Type
 *
 * To use enum values as attribute value is sai_int32_t s32
 *
 * @param sai_attr_metadata_t meta
 *
 * @passparam <- deserialize tez powinno miec metadata attribute pobrany z ID
 */
typedef union _sai_attribute_value_t
{
    bool booldata;
    char chardata[32];
    sai_uint8_t u8;
    sai_int8_t s8;
    sai_uint16_t u16;
    sai_int16_t s16;
    sai_uint32_t u32;

    /* meta->enummetadata */
    sai_int32_t s32;  /* int 32 and int32list* should always be considered enums */
    sai_uint64_t u64;
    sai_int64_t s64;
    sai_pointer_t ptr;
    sai_mac_t mac;
    sai_ip4_t ip4;
    sai_ip6_t ip6;
    sai_ip_address_t ipaddr;
    sai_ip_prefix_t ipprefix;
    sai_object_id_t oid;
    sai_object_list_t objlist;
    sai_u8_list_t u8list;
    sai_s8_list_t s8list;
    sai_u16_list_t u16list;
    sai_s16_list_t s16list;
    sai_u32_list_t u32list;
    sai_s32_list_t s32list; /* enum */
    sai_u32_range_t u32range;
    sai_s32_range_t s32range;
    sai_map_list_t maplist;
    sai_vlan_list_t vlanlist;
    sai_qos_map_list_t qosmap;
    /* @validonly sai_attr_value_type == SAI_ATTR_VALUE_TYPE_TUNNEL_MAP_LIST, type can be deduced auto from SAI_... */
    /* @validonly meta->attrvaluetype == SAI_ATTR_VALUE_TYPE_TUNNEL_MAP_LIST, type can be deduced auto from SAI_... */
    sai_tunnel_map_list_t tunnelmap;

    /* @validonly - a co tutaj ? tu musimy przekazac  */
    /* @param meta 
     *
     * ten jest valid kiedy jest multiple actions 
     *
     * could be auto translated to code -> if (..validonly )
     * @validonly meta->isaclfield == true
     *
     * swietnie - tylko jak deserialize ma sie tego domyslic ? cholera typ musi byc przekazany
     *
     * @passparam meta
     */
    sai_acl_field_data_t aclfield;      
    
    /* will contain enm, we would need recursivli check structs
    and read struct should return struct from memory if already read, since we don't need to read the same file
    multiple times, maybe even ReadXml should return that*/

    /*
     *
     * @validonly meta->isaclaction == true */
    sai_acl_action_data_t aclaction; /* also enums, since e */


    sai_acl_capability_t aclcapability; /* also enums, we must past meta ? to get enum  */

    /**
     * @brief Attribute valus is TLV list.
     *
     * @validonly meta->attrvaluetype == SAI_ATTR_VALUE_TYPE_TLV_LIST
     */
    sai_tlv_list_t tlvlist;

    /* @validonly meta->attrvaluetype == SAI_ATTR_VALUE_TYPE_SEGMENT_LIST */
    sai_segment_list_t segmentlist;

} sai_attribute_value_t;

/*
 * @param sai_attr_metadata_t meta
 *
 * przy desrialize metadata param mozna pobrac metadane z desilizacji id
 * poniewaz bedzei cala nazwa
 *
 * @deserialize skip
 *
 * int sai_deserialize_attribute(const char* buffer, sai_attribute_t *attribute);
 */
typedef struct _sai_attribute_t
{
    sai_attr_id_t id; /* za pomoca tego da sie pozyskac metadate + object type
                         prz deserializacji bedzei string wiec odrazu da sie metadata pozyskac */

    /* @param meta */
    sai_attribute_value_t value;
} sai_attribute_t;
