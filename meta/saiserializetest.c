#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sai.h>

#include "saimetadata.h"

#define TEST_ASSERT_TRUE(x,fmt,...)                         \
    if (!(x)){                                              \
        fprintf(stderr,                                     \
                "ASSERT TRUE FAILED(%s:%d): %s: " fmt "\n", \
                __func__, __LINE__, #x, ##__VA_ARGS__);     \
        exit(1);}

void subtest_serialize_object_id(
        _In_ sai_object_id_t id,
        _In_ const char *exp)
{
    char buf[128];

    int res = sai_serialize_object_id(buf, id);

    printf("%s\n", buf);
    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, exp) == 0, "expected same string");
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
    char buf[128];

    sai_ip_address_t ipaddr;

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    ipaddr.addr.ip4 = htonl(ip);

    int res = sai_serialize_ip_address(buf, &ipaddr);

    printf("%s\n", buf);
    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, exp) == 0, "expected same string");
}

void test_serialize_ip_address()
{
    subtest_serialize_ip_addres_v4(0x0a000015, "10.0.0.21");
    subtest_serialize_ip_addres_v4(0x01020304, "1.2.3.4");
    subtest_serialize_ip_addres_v4(0, "0.0.0.0");
    subtest_serialize_ip_addres_v4((uint32_t)-1, "255.255.255.255");

    int res;
    char buf[128];

    sai_ip_address_t ipaddr;

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV4;
    res = sai_serialize_ip_address(buf, &ipaddr);
    TEST_ASSERT_TRUE(res > 0, "expected positive number");

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;
    res = sai_serialize_ip_address(buf, &ipaddr);
    TEST_ASSERT_TRUE(res > 0, "expected positive number");

    /* invalid address family */

    ipaddr.addr_family = 2;
    res = sai_serialize_ip_address(buf, &ipaddr);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    /* test ip v6 */

    ipaddr.addr_family = SAI_IP_ADDR_FAMILY_IPV6;

    uint16_t ip6[] = { 0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666, 0xaaaa, 0xbbbb };

    memcpy(ipaddr.addr.ip6, ip6, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "1111:2222:3333:4444:5555:6666:aaaa:bbbb") == 0, "expected same string");

    uint16_t ip6a[] = { 0x0100, 0, 0, 0, 0, 0, 0, 0xff00 };

    memcpy(ipaddr.addr.ip6, ip6a, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "1::ff") == 0, "expected same string");

    uint16_t ip6b[] = { 0, 0, 0, 0, 0, 0, 0, 0x100 };

    memcpy(ipaddr.addr.ip6, ip6b, 16);

    res = sai_serialize_ip_address(buf, &ipaddr);

    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "::1") == 0, "expected same string");
}

void test_serialize_mac()
{
    int res;
    char buf[128];

    sai_mac_t mac;

    memcpy(mac, "\x01\x23\x45\x67\x89\xab", 6);

    res = sai_serialize_mac(buf, mac);

    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "01:23:45:67:89:AB") == 0, "expected same string");
}

void test_serialize_ipv4_mask()
{
    int res;
    char buf[128];

    sai_ip4_t mask = 0;

    res = sai_serialize_ipv4_mask(buf, mask);

    printf("%s\n", buf);
    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "0") == 0, "expected same string");

    mask = 0xffffffff;

    res = sai_serialize_ipv4_mask(buf, mask);

    TEST_ASSERT_TRUE(res > 0, "expected positive number");
    TEST_ASSERT_TRUE(strcmp(buf, "32") == 0, "expected same string");

    mask = 0xffffffff;

    int i;

    for (i = 32; i >= 0; i--)
    {
        res = sai_serialize_ipv4_mask(buf, htonl(mask));

        char exp[128];

        sprintf(exp, "%d", i);

        TEST_ASSERT_TRUE(res > 0, "expected positive number");
        TEST_ASSERT_TRUE(strcmp(buf, exp) == 0, "expected same string");

        mask = mask << 1;
    }

    mask = htonl(0xff001); /* holes */

    res = sai_serialize_ipv4_mask(buf, mask);

    TEST_ASSERT_TRUE(res < 0, "expected negative number");
}

void test_serialize_ipv6_mask()
{
    int res;
    char buf[128];

    sai_ip6_t mask = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    mask[1] = 0xff; /* mask with holes */

    res = sai_serialize_ipv6_mask(buf, mask);

    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    uint8_t *m = (uint8_t*)mask;

    int n = 0;
    char bufn[10];
    char ipv6[100];

    for (; n <= 128; n++)
    {
        memset(m, 0, 16);

        int k;
        for (k = 0; k < n; k++)
        {
            m[k/8] |= (uint8_t)(0xff << (7 - k%8));
        }

        sprintf(bufn, "%d", n);

        sai_serialize_ipv6(ipv6, mask);
        res = sai_serialize_ipv6_mask(buf, mask);

        printf("n = %s vs res %s ipv6 %s\n", bufn, buf, ipv6);
        TEST_ASSERT_TRUE(res > 0, "expected positive number");
        TEST_ASSERT_TRUE(strcmp(buf, bufn) == 0, "expected same string");
    }
}

void subtest_deserialize_u8(
        _In_ const char *buffer,
        _In_ uint8_t result,
        _In_ int len)
{
    uint8_t u8;

    int res = sai_deserialize_u8(buffer, &u8);

    printf ("res %d u8: %u\n", res, u8);
    TEST_ASSERT_TRUE(u8 == result, "result to be equal %u vs %u", u8, result);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(res == len, "expected equal lenght number: %d vs %d", res, len);
}

void test_deserialize_uint()
{
    /*
     * possible strings endings : 255,}]"\0
     */

    subtest_deserialize_u8("255",   255, 3);
    subtest_deserialize_u8("255 ",  255, 3);
    subtest_deserialize_u8("255,",  255, 3);
    subtest_deserialize_u8("255]",  255, 3);
    subtest_deserialize_u8("255}",  255, 3);
    subtest_deserialize_u8("255\"", 255, 3);
    subtest_deserialize_u8("99",     99, 2);
    subtest_deserialize_u8("9",      9,  1);
    subtest_deserialize_u8("0",      0,  1);
    subtest_deserialize_u8("0x9",    0,  1);
    subtest_deserialize_u8("077",   77,  3);
    subtest_deserialize_u8("0000000000000000000077",   77,  22);

    int res;
    uint8_t u8;

    res = sai_deserialize_u8("", &u8);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    res = sai_deserialize_u8("300", &u8); /* overflow */

    printf ("res %d u8: %u\n", res, u8);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    res = sai_deserialize_u8("-1", &u8);

    printf ("res %d u8: %u\n", res, u8);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    uint16_t u16;
    res = sai_deserialize_u16("65536", &u16);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    res = sai_deserialize_u16("65535", &u16);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(u16 == (uint16_t)-1, "result to be equal");

    uint32_t u32;
    res = sai_deserialize_u32("4294967296", &u32);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    res = sai_deserialize_u32("4294967295", &u32);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(u32 == (uint32_t)-1, "result to be equal");

    uint64_t u64;
    res = sai_deserialize_u64("18446744073709551616", &u64);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");

    res = sai_deserialize_u64("18446744073709551615", &u64);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(u64 == (uint64_t)-1, "result to be equal");

    res = sai_deserialize_u8("18446744073709551616", &u8);
    TEST_ASSERT_TRUE(res < 0, "expected negative number");
}

void test_deserialize_int()
{
    /* TODO */
}

void test_deserialize_enum()
{
    char buf[1024];
    int res;

    res = sai_serialize_enum(buf, &sai_metadata_enum_sai_status_t, SAI_STATUS_NOT_IMPLEMENTED);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);

    printf ("%s\n", buf);

    int value;

    res = sai_deserialize_enum(buf, &sai_metadata_enum_sai_status_t, &value);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(value == SAI_STATUS_NOT_IMPLEMENTED, "expected same value");

    size_t len = strlen(buf);
    buf[len++] = '"';
    buf[len++] = 0;

    res = sai_deserialize_enum(buf, &sai_metadata_enum_sai_status_t, &value);
    TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
    TEST_ASSERT_TRUE(value == SAI_STATUS_NOT_IMPLEMENTED, "expected same value");

    /* test all enums to be sure! */

    size_t enumindex = 0;

    for (; enumindex < sai_metadata_all_enums_count; ++enumindex)
    {
        const sai_enum_metadata_t* emd = sai_metadata_all_enums[enumindex];

        size_t count = 0;

        printf("checking enum %s %zu\n", emd->name, enumindex);

        for (; count < emd->valuescount; ++count)
        {
            /* make sure there is something after string in the buffer */

            sprintf(buf, "%s\"", emd->valuesnames[count]);

            res = sai_deserialize_enum(buf, emd, &value);
            TEST_ASSERT_TRUE(res > 0, "expected positive number: res = %d", res);
            TEST_ASSERT_TRUE(value == emd->values[count], "expected same value");
        }
    }
}

int main()
{
    /* list automatically symbols to execute all tests */

    test_serialize_object_id();
    test_serialize_ip_address();
    test_serialize_mac();
    test_serialize_ipv4_mask();
    test_serialize_ipv6_mask();

    test_deserialize_uint();
    test_deserialize_int();

    test_deserialize_enum();

    return 0;
}
