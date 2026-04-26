#pragma once
#include <windows.h>
#include <string>

namespace MimeTools {

// --- 纯算法函数 (用于单元测试) ---

std::string base64_encode(const std::string& in);
std::string base64_decode(const std::string& in);

std::string url_encode(const std::string& value);
std::string url_decode(const std::string& value);

std::string qp_encode(const std::string& in);
std::string qp_decode(const std::string& in);

std::string hex_encode(const std::string& in);
std::string hex_decode(const std::string& in);

std::string saml_decode(const std::string& in);
std::string html_encode(const std::string& in);
std::string html_decode(const std::string& in);

std::string md5_hash(const std::string& in);
std::string sha1_hash(const std::string& in);
std::string sha256_hash(const std::string& in);
std::string sha512_hash(const std::string& in);

// --- EmEditor 交互函数 (插件调用) ---

void EncodeBase64(HWND hwndView);
void DecodeBase64(HWND hwndView);
void EncodeUrl(HWND hwndView);
void DecodeUrl(HWND hwndView);
void DecodeSaml(HWND hwndView);
void EncodeHex(HWND hwndView);
void DecodeHex(HWND hwndView);
void EncodeQP(HWND hwndView);
void DecodeQP(HWND hwndView);
void EncodeHtml(HWND hwndView);
void DecodeHtml(HWND hwndView);

void HashMD5(HWND hwndView);
void HashSHA1(HWND hwndView);
void HashSHA256(HWND hwndView);
void HashSHA512(HWND hwndView);

void ConvertTimestamp(HWND hwndView);

} // namespace MimeTools
