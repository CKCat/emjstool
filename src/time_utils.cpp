#include "time_utils.h"
#include <ctime>
#include <iomanip>
#include <sstream>
#include <algorithm>

namespace TimeTools {

std::string TimestampToDate(const std::string& ts_str) {
    if (ts_str.empty()) return "";
    
    try {
        long long val = std::stoll(ts_str);
        bool is_ms = (val >= 10000000000LL); // 10^10 为界，10位秒 vs 13位毫秒
        
        time_t sec = is_ms ? (time_t)(val / 1000) : (time_t)val;
        int ms = is_ms ? (int)(val % 1000) : 0;
        
        struct tm lt;
        if (localtime_s(&lt, &sec) != 0) return "Invalid Range";
        
        std::stringstream ss;
        ss << std::put_time(&lt, "%Y-%m-%d %H:%M:%S");
        if (is_ms) {
            ss << "." << std::setw(3) << std::setfill('0') << ms;
        }
        return ss.str();
    } catch (...) {
        return "Invalid Number";
    }
}

std::string DateToTimestamp(const std::string& date_str) {
    struct tm lt = {0};
    std::stringstream ss(date_str);
    ss >> std::get_time(&lt, "%Y-%m-%d %H:%M:%S");
    
    if (ss.fail()) return "Parse Error";
    
    // 检查是否包含毫秒
    int ms = 0;
    if (ss.peek() == '.') {
        ss.ignore();
        std::string ms_str;
        while (isdigit(ss.peek())) ms_str += (char)ss.get();
        if (!ms_str.empty()) {
            ms = std::stoi(ms_str.substr(0, 3));
        }
    }
    
    time_t t = mktime(&lt);
    if (t == -1) return "Invalid Date";
    
    if (ms > 0) {
        return std::to_string((long long)t * 1000 + ms);
    }
    return std::to_string((long long)t);
}

std::string SmartConvert(const std::string& input) {
    std::string trimmed = input;
    trimmed.erase(0, trimmed.find_first_not_of(" \t\r\n"));
    trimmed.erase(trimmed.find_last_not_of(" \t\r\n") + 1);
    
    if (trimmed.empty()) return "";
    
    // 如果全是数字，则是时间戳
    if (std::all_of(trimmed.begin(), trimmed.end(), [](char c) { return isdigit(c); })) {
        return TimestampToDate(trimmed);
    }
    
    // 否则尝试解析为日期
    return DateToTimestamp(trimmed);
}

} // namespace TimeTools
