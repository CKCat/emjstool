#pragma once
#include <string>
#include <cstdint>

namespace TimeTools {

/**
 * @brief 将时间戳转换为日期字符串 (自动识别秒/毫秒)
 * @param ts 时间戳字符串
 * @return 格式化后的日期时间字符串
 */
std::string TimestampToDate(const std::string& ts_str);

/**
 * @brief 将日期字符串转换为时间戳 (秒)
 * @param date_str 日期字符串 (YYYY-MM-DD HH:MM:SS)
 * @return 时间戳字符串
 */
std::string DateToTimestamp(const std::string& date_str);

/**
 * @brief 智能转换：根据内容自动决定转换方向
 */
std::string SmartConvert(const std::string& input);

} // namespace TimeTools
