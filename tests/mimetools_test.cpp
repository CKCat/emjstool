#include "mimetools.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>

using namespace MimeTools;

// --- RFC 4648: Base64 Tests ---

TEST(RFC4648Test, Base64StandardVectors) {
  // Official test vectors from RFC 4648 Section 10
  EXPECT_EQ(base64_encode(""), "");
  EXPECT_EQ(base64_encode("f"), "Zg==");
  EXPECT_EQ(base64_encode("fo"), "Zm8=");
  EXPECT_EQ(base64_encode("foo"), "Zm9v");
  EXPECT_EQ(base64_encode("foob"), "Zm9vYg==");
  EXPECT_EQ(base64_encode("fooba"), "Zm9vYmE=");
  EXPECT_EQ(base64_encode("foobar"), "Zm9vYmFy");

  EXPECT_EQ(base64_decode(""), "");
  EXPECT_EQ(base64_decode("Zg=="), "f");
  EXPECT_EQ(base64_decode("Zm8="), "fo");
  EXPECT_EQ(base64_decode("Zm9v"), "foo");
  EXPECT_EQ(base64_decode("Zm9vYg=="), "foob");
  EXPECT_EQ(base64_decode("Zm9vYmE="), "fooba");
  EXPECT_EQ(base64_decode("Zm9vYmFy"), "foobar");
}

TEST(RFC4648Test, Base64BinaryData) {
  // Test with binary data (all bytes 0-255)
  std::string binary;
  for (int i = 0; i < 256; ++i)
    binary += (char)i;

  std::string encoded = base64_encode(binary);
  std::string decoded = base64_decode(encoded);

  EXPECT_EQ(binary.length(), 256);
  EXPECT_EQ(decoded, binary);
}

// --- RFC 4648: Base16 (Hex) Tests ---

TEST(RFC4648Test, HexStandardVectors) {
  // Our implementation adds spaces for readability in EmEditor
  EXPECT_EQ(hex_encode("f"), "66 ");
  EXPECT_EQ(hex_encode("fo"), "66 6F ");
  EXPECT_EQ(hex_encode("foobar"), "66 6F 6F 62 61 72 ");

  EXPECT_EQ(hex_decode("66"), "f");
  EXPECT_EQ(hex_decode("66 6f"), "fo");
  EXPECT_EQ(hex_decode("666F6F626172"),
            "foobar"); // Should handle no spaces too
}

// --- RFC 3986: URI Encoding Tests ---

TEST(RFC3986Test, UnreservedCharacters) {
  // ALPHA / DIGIT / "-" / "." / "_" / "~" should NOT be encoded
  std::string unreserved =
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-._~";
  EXPECT_EQ(url_encode(unreserved), unreserved);
}

TEST(RFC3986Test, ReservedCharacters) {
  // General delimiters and sub-delimiters should be encoded
  // Note: !*'();:@&=+$,/?#[]
  EXPECT_EQ(url_encode("!"), "%21");
  EXPECT_EQ(url_encode("*"), "%2A");
  EXPECT_EQ(url_encode("'"), "%27");
  EXPECT_EQ(url_encode("("), "%28");
  EXPECT_EQ(url_encode(")"), "%29");
  EXPECT_EQ(url_encode(";"), "%3B");
  EXPECT_EQ(url_encode(":"), "%3A");
  EXPECT_EQ(url_encode("@"), "%40");
  EXPECT_EQ(url_encode("&"), "%26");
  EXPECT_EQ(url_encode("="), "%3D");
  EXPECT_EQ(url_encode("+"), "%2B");
  EXPECT_EQ(url_encode("$"), "%24");
  EXPECT_EQ(url_encode(","), "%2C");
  EXPECT_EQ(url_encode("/"), "%2F");
  EXPECT_EQ(url_encode("?"), "%3F");
  EXPECT_EQ(url_encode("#"), "%23");
  EXPECT_EQ(url_encode("["), "%5B");
  EXPECT_EQ(url_encode("]"), "%5D");
}

TEST(RFC3986Test, UTF8Support) {
  // Test Unicode (UTF-8) encoding
  // "你好" in UTF-8 is E4 BD A0 E5 A5 BD
  EXPECT_EQ(url_encode("\xE4\xBD\xA0\xE5\xA5\xBD"), "%E4%BD%A0%E5%A5%BD");
}

TEST(RFC3986Test, DecodingFlexibility) {
  EXPECT_EQ(url_decode("a%20b"), "a b");
  EXPECT_EQ(url_decode("a+b"), "a b"); // Standard web convention: + is space
  EXPECT_EQ(url_decode("%E4%BD%A0%E5%A5%BD"), "\xE4\xBD\xA0\xE5\xA5\xBD");
}

// --- RFC 2045: Quoted-Printable Tests ---

TEST(RFC2045Test, BasicCompliance) {
  // 1. Literal characters
  EXPECT_EQ(qp_encode("ABC"), "ABC");

  // 2. Special character = must be encoded as =3D
  EXPECT_EQ(qp_encode("a=b"), "a=3Db");
  EXPECT_EQ(qp_decode("a=3Db"), "a=b");

  // 3. Line length limit (76 chars) and Soft Line Breaks
  std::string longLine(100, 'A');
  std::string encoded = qp_encode(longLine);
  EXPECT_TRUE(encoded.find("=\r\n") != std::string::npos);
  EXPECT_EQ(qp_decode(encoded), longLine);
}

TEST(RFC2045Test, TerminalWhitespace) {
  // Rule #3: Tabs and spaces at the end of an encoded line must be encoded
  // Our implementation currently hexes many things, let's verify roundtrip
  std::string input = "text with space ";
  std::string encoded = qp_encode(input);
  EXPECT_EQ(qp_decode(encoded), input);
}

TEST(RFC2045Test, BinaryAndUnicode) {
  std::string input = "\xE4\xBD\xA0\xE5\xA5\xBD"; // 你好
  std::string encoded = qp_encode(input);
  EXPECT_EQ(qp_decode(encoded), input);
}

// --- SAML Decode Tests ---

TEST(SAMLTest, StandardAuthnRequest) {
  // Standard SAML 2.0 AuthnRequest (Redirect Binding: URL -> B64 -> Inflate)
  std::string input =
      "nZJNT8JAEIb%2FSrN3WoqoyYZiEGIk8aOB6sHbuh1kk%2F1yZ6r137stYDgoB0%"
      "2BbzLyZ5513doLCaM9nDW3tCt4bQEpaoy3yvlGwJljuBCrkVhhATpKvZ%2Fd3fJQOuQ%"
      "2BOnHSaJctFwfymzUdnLHmGgMrZgkVJ7CA2sLRIwlIsDfPxYHg5yC%2Bq0Rk%2FH%2FHx%"
      "2BQtLyv2ca2VrZd9OQ193IuS3VVUOysd1xZIZIgSK0Lmz2BgIawgfSsLT6q5gWyLPswx9Cq0"
      "wXkMqnclqMC7P4ixoU7%2F1V0Iim066pXlvORzFcNqQOMDZ9BTKAIlakOhok%2BwItKN6%"
      "2FhAnLxel00p%2BJTcuGEF%2Fg%"
      "2FM07yuqHmx6KQcjlJ7VdQDEmIjW7nMeQBAUjEIDLDtw9meGuj96TIygpWTujBdBYXe4aF7S"
      "IY1j1VzHZVew%2BU82J2WSy250LJfx%2BXSh7v4EyOiyCsKid4H2of3mZ7rr%2FbHbT%"
      "2Ff4l0%2B%2FAQ%3D%3D";

  std::string expected =
      "<samlp:AuthnRequest "
      "xmlns:samlp=\"urn:oasis:names:tc:SAML:2.0:protocol\" ID=\"pfx123\" "
      "Version=\"2.0\" IssueInstant=\"2014-07-16T23:52:45Z\" "
      "ProtocolBinding=\"urn:oasis:names:tc:SAML:2.0:bindings:HTTP-POST\" "
      "AssertionConsumerServiceURL=\"http://sp.example.com/demo1/"
      "index.php?acs\"><saml:Issuer "
      "xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\">http://"
      "sp.example.com/demo1/metadata.php</saml:Issuer><samlp:NameIDPolicy "
      "Format=\"urn:oasis:names:tc:SAML:1.1:nameid-format:emailAddress\" "
      "AllowCreate=\"true\"/><samlp:RequestedAuthnContext "
      "Comparison=\"exact\"><saml:AuthnContextClassRef "
      "xmlns:saml=\"urn:oasis:names:tc:SAML:2.0:assertion\">urn:oasis:names:tc:"
      "SAML:2.0:ac:classes:PasswordProtectedTransport</"
      "saml:AuthnContextClassRef></samlp:RequestedAuthnContext></"
      "samlp:AuthnRequest>";

  EXPECT_EQ(saml_decode(input), expected);
}

// --- Additional Edge Case Tests (Multi-byte, Emoji, Stability) ---

TEST(Base64EdgeTest, MultibyteAndEmoji) {
  // Emoji: 🚀 (\xF0\x9F\x9A\x80) -> 8J+agA==
  EXPECT_EQ(base64_encode("\xF0\x9F\x9A\x80"), "8J+agA==");
  EXPECT_EQ(base64_decode("8J+agA=="), "\xF0\x9F\x9A\x80");

  // Mixed UTF-8: Hello 你好 🚀 !
  std::string mixed = "Hello \xE4\xBD\xA0\xE5\xA5\xBD \xF0\x9F\x9A\x80 !";
  std::string encoded = base64_encode(mixed);
  EXPECT_EQ(base64_decode(encoded), mixed);
}

TEST(Base64EdgeTest, BinaryNullHandling) {
  // Binary data with null bytes mid-string
  std::string binary = std::string("Start\0End", 9);
  std::string encoded = base64_encode(binary);
  EXPECT_EQ(base64_decode(encoded), binary);
}

TEST(URLEdgeTest, EmojiAndMalformed) {
  // Emoji: 🚀 -> %F0%9F%9A%80
  EXPECT_EQ(url_encode("\xF0\x9F\x9A\x80"), "%F0%9F%9A%80");
  EXPECT_EQ(url_decode("%F0%9F%9A%80"), "\xF0\x9F\x9A\x80");

  // Robustness: incomplete percent at end
  EXPECT_EQ(url_decode("abc%"), "abc%");
  // Robustness: non-hex sequence
  EXPECT_EQ(url_decode("abc%ZZ"), "abc%ZZ");
}

TEST(QPEdgeTest, MultibyteAndSoftBreaks) {
  // Ensure multi-byte characters are encoded as triplets and round-trip safe
  std::string emoji = "\xF0\x9F\x9A\x80";
  EXPECT_EQ(qp_decode(qp_encode(emoji)), emoji);

  // Soft line breaks should not break a byte triplet (=XX)
  std::string longInput(200, '\xE4');
  std::string encoded = qp_encode(longInput);
  EXPECT_TRUE(encoded.find("=\r\n") != std::string::npos);
  EXPECT_EQ(qp_decode(encoded), longInput);
}

TEST(SAMLEdgeTest, RobustnessOnInvalidInput) {
  // Ensure the decoder doesn't crash on garbage input
  std::string garbage = "!!!ThisIsNotValidBase64OrDeflate!!!";
  EXPECT_NO_THROW({ saml_decode(garbage); });
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
