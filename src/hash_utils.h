#pragma once
#include <string>
#include <windows.h>
#include <bcrypt.h>

#pragma comment(lib, "bcrypt.lib")

namespace HashTools {

/**
 * @brief 使用 Windows CNG API 计算哈希值
 * @param algo 算法标识符 (如 BCRYPT_MD5_ALGORITHM)
 * @param input 输入字符串 (UTF-8)
 * @return 十六进制哈希字符串
 */
std::string ComputeHash(LPCWSTR algo, const std::string& input);

std::string md5(const std::string& input);
std::string sha1(const std::string& input);
std::string sha256(const std::string& input);
std::string sha512(const std::string& input);

} // namespace HashTools
