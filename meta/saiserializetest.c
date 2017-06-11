#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sai.h>

#include "saimetadata.h"

#define ASSERT_TRUE(x,fmt,...)                              \
    if (!(x)){                                              \
        fprintf(stderr,                                     \
                "ASSERT TRUE FAILED(%s:%d): %s: " fmt "\n", \
                __func__, __LINE__, #x, ##__VA_ARGS__);     \
        exit(1);}

#define ASSERT_STR_EQ(a,b,r)                                                \
    if (strcmp(a,b) != 0){                                                  \
        fprintf(stderr,                                                     \
                "ASSERT STR_EQ FAILED(%s:%d): is:\n%s\nexpected:\n%s\n",    \
                __func__, __LINE__, a, b);                                  \
        exit(1);}                                                           \
    if ((int)strlen(a) != r){                                               \
        fprintf(stderr,                                                     \
                "ASSERT STR_EQ FAILED(%s:%d): returned length is wrong"     \
                " res (%d) != strlen (%zu)\n",                              \
                __func__, __LINE__, r, strlen(a));                          \
        exit(1);}

#define PRIMITIVE_BUFFER_SIZE 128

void subtest_serialize_object_id(
        _In_ sai_object_id_t id,
        _In_ const char *exp)
{
    char buf[PRIMITIVE_BUFFER_SIZE];

    int res = sai_serialize_object_id(buf, id);

    ASSERT_STR_EQ(buf, exp, res);
}

void test_serialize_chardata()
{
    sai_attribute_value_t val;

    memset(&val, 0, sizeof(val));

    char buf[PRIMITIVE_BUFFER_SIZE];

    int res;

    res = sai_serialize_chardata(buf, val.chardata);
    ASSERT_STR_EQ(buf, "", res);

    strcpy(val.chardata, "foo bar 123");

    res = sai_serialize_chardata(buf, val.chardata);
    ASSERT_STR_EQ(buf, "foo bar 123", res);

    strcpy(val.chardata, "foo \\ bar 123");

    res = sai_serialize_chardata(buf, val.chardata);
    ASSERT_TRUE(res < 0, "expected negative number");

    strcpy(val.chardata, "foo \" bar 123");

    res = sai_serialize_chardata(buf, val.chardata);
    ASSERT_TRUE(res < 0, "expected negative number");

    strcpy(val.chardata, "0123456789012345678912");

    res = sai_serialize_chardata(buf, val.chardata);
    ASSERT_STR_EQ(buf, "0123456789012345678912", res);
}

void test_serialize_object_id()
{
    subtest_serialize_object_id(0, "oid:0x0");
    subtest_serialize_object_id(0x123456789abcdef0, "oid:0x123456789abcdef0");
    subtest_serialize_object_id(0x123459abcdef0, "oid:0x123459abcdef0");
    subtest_serialize_object_id(0xFFFFFFFFFFFFFFFF, "oid:0xffffffffffffffff");
}

void subtest_serialize_ip_addres_v4(
        _In_ uint32_t ip,
        _In_ const char *exp)
{
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_ip_address_t ipaddr;

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    ipaddr.addr.ip4 = htonl(ip);

    int res = sai_serialize_ip_address(buf, &ipaddr);

    ASSERT_STR_EQ(buf, exp, res);
}

void test_serialize_ip_address()
{
    subtest_serialize_ip_addres_v4(0x0a000015, "10.0.0.21");
    subtest_serialize_ip_addres_v4(0x01020304, "1.2.3.4");
    subtest_serialize_ip_addres_v4(0, "0.0.0.0");
    subtest_serialize_ip_addres_v4((uint32_t)-1, "255.255.255.255");

    int res;
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_ip_address_t ipaddr;

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    res = sai_serialize_ip_address(buf, &ipaddr);
    ASSERT_TRUE(res > 0, "expected positive number");

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    res = sai_serialize_ip_address(buf, &ipaddr);
    ASSERT_TRUE(res > 0, "expected positive number");

    /* invalid address family */

    ipaddr.addr_family = 2;
    res = sai_serialize_ip_address(buf, &ipaddr);
    ASSERT_TRUE(res < 0, "expected negative number");

    /* test ip v6 */

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

    uint16_t ip6[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xaaaa, 0xbbbb };

    memcpy(ipaddr.addr.ip6, ip6, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    ASSERT_STR_EQ(buf, "1111:2222:3333:4444:5555:6666:aaaa:bbbb", res);

    uint16_t ip6a[] = { 0x0100, 0, 0, 0, 0, 0, 0, 0xff00 };

    memcpy(ipaddr.addr.ip6, ip6a, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    ASSERT_STR_EQ(buf, "1::ff", res);

    uint16_t ip6b[] = { 0, 0, 0, 0, 0, 0, 0, 0x100 };

    memcpy(ipaddr.addr.ip6, ip6b, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    ASSERT_STR_EQ(buf, "::1", res);
}

void test_serialize_mac()
{
    int res;
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_mac_t mac;

    memcpy(mac, "\x01\x23\x45\x67\x89\xab", 6);

    res = sai_serialize_mac(buf, mac);

    ASSERT_STR_EQ(buf, "01:23:45:67:89:AB", res);
}

void test_serialize_enum()
{
    int res;
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_object_type_t ot = SAI_OBJECT_TYPE_PORT;

    res = sai_serialize_enum(buf, &sai_metadata_enum_sai_object_type_t, ot);

    ASSERT_STR_EQ(buf, "SAI_OBJECT_TYPE_PORT", res);

    res = sai_serialize_enum(buf, &sai_metadata_enum_sai_object_type_t, -1);

    ASSERT_STR_EQ(buf, "-1", res);

    res = sai_serialize_enum(buf, &sai_metadata_enum_sai_object_type_t, 100);

    ASSERT_STR_EQ(buf, "100", res);

    /* test all enums */

    size_t i = 0;

    for (; i < sai_metadata_all_enums_count; ++i)
    {
        const sai_enum_metadata_t* emd = sai_metadata_all_enums[i];

        size_t j = 0;

        for (; j < emd->valuescount; ++j)
        {
            int value = emd->values[j];

            res = sai_serialize_enum(buf, emd, value);

            ASSERT_STR_EQ(buf, emd->valuesnames[j], res);
        }
    }
}

void test_serialize_ip4_mask()
{
    int res;
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_ip4_t mask = 0;

    res = sai_serialize_ip4_mask(buf, mask);

    ASSERT_STR_EQ(buf, "0", res);

    mask = 0xffffffff;

    res = sai_serialize_ip4_mask(buf, mask);

    ASSERT_STR_EQ(buf, "32", res);

    mask = 0xffffffff;

    int i;
    char exp[PRIMITIVE_BUFFER_SIZE];

    for (i = 32; i >= 0; i--)
    {
        res = sai_serialize_ip4_mask(buf, htonl(mask));

        sprintf(exp, "%d", i);

        ASSERT_STR_EQ(buf, exp, res);

        mask = mask << 1;
    }

    mask = htonl(0xff001); /* holes */

    res = sai_serialize_ip4_mask(buf, mask);

    ASSERT_TRUE(res < 0, "expected negative number");
}

void test_serialize_ip6_mask()
{
    int res;
    char buf[PRIMITIVE_BUFFER_SIZE];

    sai_ip6_t mask = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    mask[1] = 0xff; /* mask with holes */

    res = sai_serialize_ip6_mask(buf, mask);

    ASSERT_TRUE(res < 0, "expected negative number");

    uint8_t *m = (uint8_t*)mask;

    int n = 0;
    char bufn[PRIMITIVE_BUFFER_SIZE];
    char ipv6[PRIMITIVE_BUFFER_SIZE];

    for (; n <= 128; n++)
    {
        memset(m, 0, 16);

        int k;
        for (k = 0; k < n; k++)
        {
            m[k/8] |= (uint8_t)(0xff << (7 - k%8));
        }

        sprintf(bufn, "%d", n);

        sai_serialize_ip6(ipv6, mask);
        res = sai_serialize_ip6_mask(buf, mask);

        ASSERT_STR_EQ(buf, bufn, res);
    }
}


void test_serialize_route_entry()
{
    sai_route_entry_t re;

    char buf[PRIMITIVE_BUFFER_SIZE];
    int res;

    re.switch_id = 0x123;
    re.vr_id = 0xfab;
    re.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV4;

    re.destination.addr.ip4 = htonl(0x01020304);
    re.destination.mask.ip4 = htonl(0xffffffff);

    res = sai_serialize_route_entry(buf, &re);

    ASSERT_STR_EQ(buf, "{\"switch_id\":\"oid:0x123\",\"vr_id\":\"oid:0xfab\",\"destination\":\"1.2.3.4/32\"}", res);

    re.destination.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

    uint16_t ip6[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xaaaa, 0xbbbb };

    memcpy(re.destination.addr.ip6, ip6, 16);
    memset(re.destination.mask.ip6, 0xff, 16);

    res = sai_serialize_route_entry(buf, &re);

    ASSERT_STR_EQ(buf, "{\"switch_id\":\"oid:0x123\",\"vr_id\":\"oid:0xfab\",\"destination\":\"1111:2222:3333:4444:5555:6666:aaaa:bbbb/128\"}", res);

    re.destination.addr_family = 2;

    res = sai_serialize_route_entry(buf, &re);

    ASSERT_TRUE(res < 0, "expected negative number");
}

void test_serialize_neighbor_entry()
{
    char buf[PRIMITIVE_BUFFER_SIZE];
    int res;

    sai_neighbor_entry_t ne;

    ne.switch_id = 0x123;
    ne.rif_id = 0xfab;

    ne.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    ne.ip_address.addr.ip4 = htonl(0x01020304);

    res = sai_serialize_neighbor_entry(buf, &ne);

    ASSERT_STR_EQ(buf, "{\"switch_id\":\"oid:0x123\",\"rif_id\":\"oid:0xfab\",\"ip_address\":\"1.2.3.4\"}", res);

    uint16_t ip6[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xaaaa, 0xbbbb };

    memcpy(ne.ip_address.addr.ip6, ip6, 16);

    ne.ip_address.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

    res = sai_serialize_neighbor_entry(buf, &ne);

    ASSERT_STR_EQ(buf, "{\"switch_id\":\"oid:0x123\",\"rif_id\":\"oid:0xfab\",\"ip_address\":\"1111:2222:3333:4444:5555:6666:aaaa:bbbb\"}", res);

    ne.ip_address.addr_family = 2;

    res = sai_serialize_neighbor_entry(buf, &ne);

    ASSERT_TRUE(res < 0, "expected negative number");
}

void test_serialize_fdb_entry()
{
    char buf[PRIMITIVE_BUFFER_SIZE];
    int res;

    sai_fdb_entry_t fe;

    fe.switch_id = 0x123;
    fe.bv_id = 0xfab;

    memcpy(fe.mac_address, "\x01\x23\x45\x67\x89\xab", 6);

    res = sai_serialize_fdb_entry(buf, &fe);

    ASSERT_STR_EQ(buf, "{\"switch_id\":\"oid:0x123\",\"mac_address\":\"01:23:45:67:89:AB\",\"bv_id\":\"oid:0xfab\"}", res);
}

void test_serialize_notifications()
{
    char buf[0x100 * PRIMITIVE_BUFFER_SIZE];
    int res;
    const char* ret;

    sai_object_id_t switch_id = 0x123abc;

    sai_fdb_event_notification_data_t data;
    memset(&data, 0, sizeof(data));

    res = sai_serialize_fdb_event_notification(buf, 1, &data);
    ret = "{\"count\":1,\"data\":[{\"event_type\":\"SAI_FDB_EVENT_LEARNED\",\"fdb_entry\":{\"switch_id\":\"oid:0x0\",\"mac_address\":\"00:00:00:00:00:00\",\"bv_id\":\"oid:0x0\"},\"attr_count\":0,\"attr\":null}]}";

    ASSERT_STR_EQ(buf, ret, res);

    char buffer[7] = { 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77 };

    sai_attribute_t attrs[1];

    /*
     * TODO we set to zero, since attr serialize is not supported yet, after
     * support we need to fix this.
     */

    res = sai_serialize_packet_event_notification(buf, switch_id, buffer, 7, 0, attrs);
    ret = "{\"switch_id\":\"oid:0x123abc\",\"buffer\":[17,34,51,68,85,102,119],\"buffer_size\":7,\"attr_count\":0,\"attr_list\":null}";
    ASSERT_STR_EQ(buf, ret, res);

    sai_port_oper_status_notification_t data1;
    memset(&data1, 0, sizeof(data1));

    res = sai_serialize_port_state_change_notification(buf, 1, &data1);
    ret = "{\"count\":1,\"data\":[{\"port_id\":\"oid:0x0\",\"port_state\":\"SAI_PORT_OPER_STATUS_UNKNOWN\"}]}";
    ASSERT_STR_EQ(buf, ret , res);

    sai_queue_deadlock_notification_data_t data2;
    memset(&data2, 0, sizeof(data2));

    res = sai_serialize_queue_pfc_deadlock_notification(buf, 1, &data2);
    ret = "{\"count\":1,\"data\":[{\"queue_id\":\"oid:0x0\",\"event\":\"SAI_QUEUE_PFC_DEADLOCK_EVENT_TYPE_DETECTED\"}]}";
    ASSERT_STR_EQ(buf, ret, res);

    res = sai_serialize_switch_shutdown_request_notification(buf, switch_id);
    ret = "{\"switch_id\":\"oid:0x123abc\"}";
    ASSERT_STR_EQ(buf, ret, res);

    res = sai_serialize_switch_state_change_notification(buf, switch_id, SAI_SWITCH_OPER_STATUS_UP);
    ret = "{\"switch_id\":\"oid:0x123abc\",\"switch_oper_status\":\"SAI_SWITCH_OPER_STATUS_UP\"}";
    ASSERT_STR_EQ(buf, ret, res);

    sai_tam_threshold_breach_event_t data3;
    memset(&data3, 0, sizeof(data3));

    res = sai_serialize_tam_event_notification(buf, 1, &data3);
    ret = "{\"count\":1,\"data\":[{\"threshold_id\":\"oid:0x0\",\"is_snapshot_valid\":false,\"tam_snapshot_id\":\"oid:0x0\",\"value\":0}]}";
    ASSERT_STR_EQ(buf, ret, res);
}

void test_deserialize_bool()
{
    int res;
    bool b;

    const char *valid_true[] = { "true", "true,", "true\"", "true]", "true}"};
    const char *invalid_true[] = { "truee", "tru1", "true)", "true="};

    const char *valid_false[] = { "false", "false,", "false\"", "false]", "false}"};
    const char *invalid_false[] = { "falsee", "tru1", "false)", "false="};

    size_t n;

    for (n = 0; n < sizeof(valid_true)/sizeof(const char*); n++)
    {
        b = false;
        res = sai_deserialize_bool(valid_true[n], &b);
        ASSERT_TRUE(b, "expected true");
        ASSERT_TRUE(res == 4, "expected true");
    }

    for (n = 0; n < sizeof(invalid_true)/sizeof(const char*); n++)
    {
        b = false;
        res = sai_deserialize_bool(invalid_true[n], &b);
        ASSERT_TRUE(res < 0, "expected negative");
    }

    for (n = 0; n < sizeof(valid_false)/sizeof(const char*); n++)
    {
        b = false;
        res = sai_deserialize_bool(valid_false[n], &b);
        ASSERT_TRUE(!b, "expected false");
        ASSERT_TRUE(res == 5, "expected false");
    }

    for (n = 0; n < sizeof(invalid_false)/sizeof(const char*); n++)
    {
        b = false;
        res = sai_deserialize_bool(invalid_false[n], &b);
        ASSERT_TRUE(res < 0, "expected negative");
    }
}

void test_deserialize_mac()
{
    int res;
    sai_mac_t mac;

    res = sai_deserialize_mac("11:22:33:44:55:66", mac);
    ASSERT_TRUE(res == 17, "expected 17 length");
    ASSERT_TRUE(memcmp(mac, "\x11\x22\x33\x44\x55\x66", 6) == 0, "expected equal");

    res = sai_deserialize_mac("ff:22:33:44:55:66", mac);
    ASSERT_TRUE(res == 17, "expected 17 length");
    ASSERT_TRUE(memcmp(mac, "\xff\x22\x33\x44\x55\x66", 6) == 0, "expected equal");

    res = sai_deserialize_mac("FF:22:33:44:55:66", mac);
    ASSERT_TRUE(res == 17, "expected 17 length");
    ASSERT_TRUE(memcmp(mac, "\xff\x22\x33\x44\x55\x66", 6) == 0, "expected equal");

    res = sai_deserialize_mac("11:22:33:44:55:66,", mac);
    ASSERT_TRUE(res == 17, "expected 17 length");
    ASSERT_TRUE(memcmp(mac, "\x11\x22\x33\x44\x55\x66", 6) == 0, "expected equal");

    res = sai_deserialize_mac("1:2:3:4:5:f", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("011:022:33:44:55:66,", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("11:22:33:44:55:z6,", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("11:22:33:44:55:66j", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("111:22:33:44:55:66", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("11:22:33:44:55:]6", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("11:g2:33:44:55:66", mac);
    ASSERT_TRUE(res < 0, "expected negative");

    res = sai_deserialize_mac("11:22::33:44:55:66", mac);
    ASSERT_TRUE(res < 0, "expected negative");
}

void test_deserialize_object_id()
{
    int res;
    size_t n;
    sai_object_id_t oid;

    const char *valid_oids[] = {
        "oid:0x0",
        "oid:0x1",
        "oid:0x0123456789abcdef",
        "oid:0x0123456789abcdef,",
        "oid:0x0123456789abcdef\"",
        "oid:0x0123456789abcdef}",
        "oid:0x0123456789abcdef]",
        };

    const char *invalid_oids[] = {
        "oid:0x",
        "aa",
        "45",
        "oid:0x0123456789abcdefv",
        "oid:0x0123456789abcdef0",
        "oid:0x00123456789abcdef",
        };

    for (n = 0; n < sizeof(valid_oids)/sizeof(const char*); n++)
    {
        res = sai_deserialize_object_id(valid_oids[n], &oid);
        ASSERT_TRUE(res > 0 , "expected true");

        sai_object_id_t ref;
        int len;
        sscanf(valid_oids[n], "oid:0x%lx%n", &ref, &len);

        ASSERT_TRUE(res == len, "expected true");
        ASSERT_TRUE(oid == ref, "expected true");
    }

    for (n = 0; n < sizeof(invalid_oids)/sizeof(const char*); n++)
    {
        res = sai_deserialize_object_id(invalid_oids[n], &oid);
        ASSERT_TRUE(res < 0, "expected negative");
    }
}

int main()
{
    test_serialize_chardata();
    test_serialize_object_id();
    test_serialize_ip_address();
    test_serialize_mac();
    test_serialize_enum();
    test_serialize_ip4_mask();
    test_serialize_ip6_mask();

    test_serialize_route_entry();
    test_serialize_neighbor_entry();
    test_serialize_fdb_entry();

    test_serialize_notifications();

    test_deserialize_bool();
    test_deserialize_mac();
    test_deserialize_object_id();

    return 0;
}
