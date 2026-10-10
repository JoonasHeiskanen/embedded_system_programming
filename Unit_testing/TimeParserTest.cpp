#include <gtest/gtest.h>
#include "../TimeParser.h"

// Test suite: TimeParserTest
TEST(TimeParserTest, TestCaseCorrectTime) {

    // Test with correct time string
    char time[] = "000005";
    EXPECT_EQ(time_parse(time), 5);

    char time2[] = "000105";
    EXPECT_EQ(time_parse(time2), 65);

    char random_time[] = "132512";
    EXPECT_EQ(time_parse(random_time), 48312);

    char random_time2[] = "054509";
    EXPECT_EQ(time_parse(random_time2), 20709);
}

TEST(TimeParserTest, BoundaryValuesCorrect) {

    // Test with boundaryvalues is correct
    char min_time[] = "000000";
    EXPECT_EQ(time_parse(min_time), 0);

    char max_time[] = "235959";
    EXPECT_EQ(time_parse(max_time), 86399);
}

TEST(TimeParserTest, BoundaryValuesIncorrect) {

    // Test with boundrayvalues is incorrect
    char over_hours[] = "240001";
    EXPECT_EQ(time_parse(over_hours), TIME_VALUE_ERROR);

    char over_minutes[] = "006100";
    EXPECT_EQ(time_parse(over_minutes), TIME_VALUE_ERROR);

    char over_seconds[] = "000060";
    EXPECT_EQ(time_parse(over_seconds), TIME_VALUE_ERROR);
}
TEST(TimeParserTest, TestStringLen) {

    // Test with null time and wrong string length
    char null_time[] = "";
    EXPECT_EQ(time_parse(null_time), TIME_LEN_ERROR);

    char wrong_len[] = "324";
    EXPECT_EQ(time_parse(wrong_len), TIME_LEN_ERROR);
}

TEST(TimeParserTest, TestCorrectSeq) {
    
    char seq[] = "RYGRYG";
    EXPECT_EQ(seq_parse(seq), SEQUENCE_OK);
}

TEST(TimeParserTest, TestInCorrectSeq) {
    
    char seq[] = "RYHG";
    EXPECT_EQ(seq_parse(seq), SEQUENCE_ERROR);
}

TEST(TimeParserTest, TestNullString) {
    
    char seq[] = "";
    EXPECT_EQ(seq_parse(seq), NULL_VALUE_ERROR);
}

// https://google.github.io/googletest/reference/testing.html
// https://google.github.io/googletest/reference/assertions.html
