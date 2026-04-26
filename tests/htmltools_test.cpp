#include "mimetools.h"
#include <gtest/gtest.h>
#include <string>

using namespace MimeTools;

TEST(HTMLTest, BasicEntities) {
    EXPECT_EQ(html_encode("<tag> & \"quote\" 'apos'"), "&lt;tag&gt; &amp; &quot;quote&quot; &apos;apos&apos;");
    EXPECT_EQ(html_decode("&lt;tag&gt; &amp; &quot;quote&quot; &apos;apos&apos;"), "<tag> & \"quote\" 'apos'");
}

TEST(HTMLTest, NumericEntities) {
    // Decimal
    EXPECT_EQ(html_decode("&#65;&#66;&#67;"), "ABC");
    // Hexadecimal
    EXPECT_EQ(html_decode("&#x41;&#x42;&#x43;"), "ABC");
    EXPECT_EQ(html_decode("&#X41;&#X42;&#X43;"), "ABC");
}

TEST(HTMLTest, UTF8Support) {
    // Chinese "你好" -> &#x4F60;&#x597D;
    EXPECT_EQ(html_encode("\xE4\xBD\xA0\xE5\xA5\xBD"), "&#x4F60;&#x597D;");
    EXPECT_EQ(html_decode("&#x4F60;&#x597D;"), "\xE4\xBD\xA0\xE5\xA5\xBD");
    
    // 2-byte: ¢ (U+00A2)
    EXPECT_EQ(html_encode("\xC2\xA2"), "&#xA2;");
    // 4-byte: 🚀 (U+1F680)
    EXPECT_EQ(html_encode("\xF0\x9F\x9A\x80"), "&#x1F680;");
    EXPECT_EQ(html_decode("&#x1F680;"), "\xF0\x9F\x9A\x80");
}

TEST(HTMLTest, EncodeEdgeCases) {
    EXPECT_EQ(html_encode(""), "");
    EXPECT_EQ(html_encode("plain"), "plain");
    EXPECT_EQ(html_encode("&&"), "&amp;&amp;");
    
    // Malformed UTF-8: Incomplete 3-byte sequence
    std::string malformed = "\xE4\xBD"; 
    EXPECT_EQ(html_encode(malformed), malformed);
    
    // Malformed UTF-8: Unexpected continuation byte
    std::string lone_cont = "\x80";
    EXPECT_EQ(html_encode(lone_cont), lone_cont);
}

TEST(HTMLTest, DecodeEdgeCases) {
    // Empty/Simple
    EXPECT_EQ(html_decode(""), "");
    EXPECT_EQ(html_decode("plain"), "plain");

    // Unterminated (no semicolon) - should remain as is
    EXPECT_EQ(html_decode("&amp"), "&amp");
    EXPECT_EQ(html_decode("&#65"), "&#65");
    EXPECT_EQ(html_decode("&#x41"), "&#x41");

    // Invalid formats
    EXPECT_EQ(html_decode("&;"), "&;");
    EXPECT_EQ(html_decode("&#;"), "&#;");
    EXPECT_EQ(html_decode("&#x;"), "&#x;");
    EXPECT_EQ(html_decode("&#abc;"), "&#abc;");
    EXPECT_EQ(html_decode("&#xxyz;"), "&#xxyz;");

    // Out of range Unicode (U+110000)
    EXPECT_EQ(html_decode("&#1114112;"), "&#1114112;");

    // Length limit (code limit is 10)
    EXPECT_EQ(html_decode("&toolongentityname;"), "&toolongentityname;");

    // Case sensitivity for named entities (HTML5 standard is specific)
    EXPECT_EQ(html_decode("&LT;"), "&LT;"); // Our impl uses lowercase check

    // Hex case flexibility
    EXPECT_EQ(html_decode("&#X41;"), "A");
    
    // Null byte
    std::string null_ent = "&#0;";
    std::string expected_null(1, '\0');
    // Note: nlohmann/json and some string impls might struggle with \0, 
    // but std::string handles it.
    EXPECT_EQ(html_decode(null_ent), expected_null);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
