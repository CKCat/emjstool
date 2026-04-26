#include "hash_utils.h"
#include <gtest/gtest.h>
#include <string>

using namespace HashTools;

TEST(HashTest, MD5_RFC1321) {
    // RFC 1321 Section A.5
    EXPECT_EQ(md5(""), "d41d8cd98f00b204e9800998ecf8427e");
    EXPECT_EQ(md5("a"), "0cc175b9c0f1b6a831c399e269772661");
    EXPECT_EQ(md5("abc"), "900150983cd24fb0d6963f7d28e17f72");
    EXPECT_EQ(md5("message digest"), "f96b697d7cb7938d525a2f31aaf161d0");
    EXPECT_EQ(md5("abcdefghijklmnopqrstuvwxyz"), "c3fcd3d76192e4007dfb496cca67e13b");
}

TEST(HashTest, SHA1_RFC3174) {
    // RFC 3174 Section 7
    EXPECT_EQ(sha1("abc"), "a9993e364706816aba3e25717850c26c9cd0d89d");
    EXPECT_EQ(sha1("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"), 
              "84983e441c3bd26ebaae4aa1f95129e5e54670f1");
}

TEST(HashTest, SHA256_NIST) {
    // NIST Test Vectors
    EXPECT_EQ(sha256("abc"), "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

TEST(HashTest, SHA512_NIST) {
    EXPECT_EQ(sha512("abc"), 
              "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a"
              "2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f");
}

TEST(HashTest, UTF8_Hashing) {
    // "你好" in UTF-8 is E4 BD A0 E5 A5 BD
    // md5sum: 7eca689f0d3389d9dea66ae112e5cfd7
    EXPECT_EQ(md5("\xE4\xBD\xA0\xE5\xA5\xBD"), "7eca689f0d3389d9dea66ae112e5cfd7");
}
