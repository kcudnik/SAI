#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sai.h>

#include "saimetadata.h"

#define TEST_ASSERT_TRUE(x,fmt,...)                         \
    if (!(x)){                                              \
        fprintf(stderr,                                     \
                "ASSERT TRUE FAILED(%d): %s: " fmt "\n",    \
                __LINE__, #x, ##__VA_ARGS__);               \
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

int main()
{
    /* list automatically symbols to execute all tests */

    test_serialize_object_id();
    test_serialize_ip_address();
    test_serialize_mac();
    test_serialize_ipv4_mask();

    return 0;
}
