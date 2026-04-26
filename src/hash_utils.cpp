#include "hash_utils.h"
#include <vector>
#include <iomanip>
#include <sstream>

namespace HashTools {

std::string ComputeHash(LPCWSTR algo, const std::string& input) {
    BCRYPT_ALG_HANDLE hAlg = NULL;
    BCRYPT_HASH_HANDLE hHash = NULL;
    DWORD cbHashObject = 0;
    DWORD cbHash = 0;
    DWORD cbData = 0;
    std::string result;

    // 1. 打开算法提供程序
    if (BCryptOpenAlgorithmProvider(&hAlg, algo, NULL, 0) != 0) return "";

    // 2. 获取哈希对象大小和哈希值大小
    if (BCryptGetProperty(hAlg, BCRYPT_OBJECT_LENGTH, (PBYTE)&cbHashObject, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }
    if (BCryptGetProperty(hAlg, BCRYPT_HASH_LENGTH, (PBYTE)&cbHash, sizeof(DWORD), &cbData, 0) != 0) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return "";
    }

    std::vector<BYTE> hashObject(cbHashObject);
    std::vector<BYTE> hash(cbHash);

    // 3. 创建哈希对象
    if (BCryptCreateHash(hAlg, &hHash, hashObject.data(), cbHashObject, NULL, 0, 0) == 0) {
        // 4. 哈希数据
        if (BCryptHashData(hHash, (PBYTE)input.c_str(), (ULONG)input.length(), 0) == 0) {
            // 5. 完成哈希
            if (BCryptFinishHash(hHash, hash.data(), cbHash, 0) == 0) {
                std::stringstream ss;
                ss << std::hex << std::setfill('0');
                for (BYTE b : hash) {
                    ss << std::setw(2) << (int)b;
                }
                result = ss.str();
            }
        }
        BCryptDestroyHash(hHash);
    }

    BCryptCloseAlgorithmProvider(hAlg, 0);
    return result;
}

std::string md5(const std::string& input) {
    return ComputeHash(BCRYPT_MD5_ALGORITHM, input);
}

std::string sha1(const std::string& input) {
    return ComputeHash(BCRYPT_SHA1_ALGORITHM, input);
}

std::string sha256(const std::string& input) {
    return ComputeHash(BCRYPT_SHA256_ALGORITHM, input);
}

std::string sha512(const std::string& input) {
    return ComputeHash(BCRYPT_SHA512_ALGORITHM, input);
}

} // namespace HashTools
