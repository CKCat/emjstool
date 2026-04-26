#pragma once
#include <string>

/**
 * @brief 将 JSON5 风格的输入转换为标准 JSON。
 * 支持：行/块注释、单引号、裸键、尾随逗号。
 */
std::string ConvertJson5ToJson(const std::string &input);
