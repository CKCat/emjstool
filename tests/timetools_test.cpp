#include "time_utils.h"
#include <gtest/gtest.h>
#include <string>

using namespace TimeTools;

TEST(TimeTest, TimestampToDate) {
    // Test seconds detection
    std::string s10 = "1714131234";
    std::string d10 = TimestampToDate(s10);
    // Round trip
    EXPECT_EQ(DateToTimestamp(d10), s10);
    
    // Test milliseconds detection
    std::string s13 = "1714131234567";
    std::string d13 = TimestampToDate(s13);
    EXPECT_TRUE(d13.find(".567") != std::string::npos);
    // Round trip
    EXPECT_EQ(DateToTimestamp(d13), s13);
}

TEST(TimeTest, DateToTimestamp) {
    // Standard format
    std::string d = "2024-04-26 12:00:00";
    std::string ts = DateToTimestamp(d);
    // Round trip
    EXPECT_EQ(TimestampToDate(ts), d);
    
    // Milliseconds format
    std::string dm = "2024-04-26 12:00:00.123";
    std::string tsm = DateToTimestamp(dm);
    EXPECT_EQ(TimestampToDate(tsm), dm);
}

TEST(TimeTest, SmartConvert) {
    // Number -> Date -> Number
    std::string input = "1714131234";
    std::string date = SmartConvert(input);
    EXPECT_EQ(SmartConvert(date), input);
}

TEST(TimeTest, EdgeCases) {
    EXPECT_EQ(TimestampToDate("abc"), "Invalid Number");
    EXPECT_EQ(DateToTimestamp("not a date"), "Parse Error");
    EXPECT_EQ(SmartConvert(""), "");
}
