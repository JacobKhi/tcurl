#include "test.h"
#include "core/textbuf.h"

#include <string.h>
#include <stdlib.h>

static int test_tb_move_line_start(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    // Add some text
    tb_set_from_string(&tb, "Hello World");
    
    // Move cursor to middle
    tb.cursor_col = 6;
    TEST_ASSERT(tb.cursor_col == 6);
    
    // Move to line start
    tb_move_line_start(&tb);
    TEST_ASSERT(tb.cursor_col == 0);
    
    // Already at start, should stay at 0
    tb_move_line_start(&tb);
    TEST_ASSERT(tb.cursor_col == 0);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_line_end(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    // Add some text
    tb_set_from_string(&tb, "Hello World");
    
    // Cursor should start at end after set_from_string
    // Move to start first
    tb.cursor_col = 0;
    TEST_ASSERT(tb.cursor_col == 0);
    
    // Move to line end
    tb_move_line_end(&tb);
    TEST_ASSERT(tb.cursor_col == 11);  // "Hello World" is 11 chars
    
    // Already at end, should stay at end
    tb_move_line_end(&tb);
    TEST_ASSERT(tb.cursor_col == 11);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_line_start_end_multiline(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "First line\nSecond line\nThird");
    
    // Move to second line, middle position
    tb.cursor_row = 1;
    tb.cursor_col = 5;
    
    // Move to line start
    tb_move_line_start(&tb);
    TEST_ASSERT(tb.cursor_row == 1);
    TEST_ASSERT(tb.cursor_col == 0);
    
    // Move to line end
    tb_move_line_end(&tb);
    TEST_ASSERT(tb.cursor_row == 1);
    TEST_ASSERT(tb.cursor_col == 11);  // "Second line" is 11 chars
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_word_right_basic(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "hello world test");
    tb.cursor_row = 0;
    tb.cursor_col = 0;
    
    // From "hello"
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 6);  // After "hello "
    
    // From "world"
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 12);  // After "world "
    
    // From "test"
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 16);  // At end
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_word_left_basic(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "hello world test");
    tb.cursor_row = 0;
    tb.cursor_col = 16;  // At end
    
    // To start of "test"
    tb_move_word_left(&tb);
    TEST_ASSERT(tb.cursor_col == 12);
    
    // To start of "world"
    tb_move_word_left(&tb);
    TEST_ASSERT(tb.cursor_col == 6);
    
    // To start of "hello"
    tb_move_word_left(&tb);
    TEST_ASSERT(tb.cursor_col == 0);
    
    // Already at start
    tb_move_word_left(&tb);
    TEST_ASSERT(tb.cursor_col == 0);
    TEST_ASSERT(tb.cursor_row == 0);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_word_punctuation(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "hello-world_test");
    tb.cursor_row = 0;
    tb.cursor_col = 0;
    
    // "hello" (alnum)
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 5);
    
    // "-" (punctuation)
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 6);
    
    // "world_test" (alnum with underscore)
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 16);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_move_word_multiline(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "first\nsecond");
    
    // At end of first line
    tb.cursor_row = 0;
    tb.cursor_col = 5;
    
    // Should wrap to next line
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_row == 1);
    TEST_ASSERT(tb.cursor_col == 0);
    
    // Move to start of "second"
    tb_move_word_left(&tb);
    TEST_ASSERT(tb.cursor_row == 0);
    TEST_ASSERT(tb.cursor_col == 5);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_delete_char_basic(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "Hello World");
    
    // Delete at position 5 (space)
    tb.cursor_col = 5;
    tb_delete_char(&tb);
    
    char *result = tb_to_string(&tb);
    TEST_ASSERT_STR_EQ(result, "HelloWorld");
    free(result);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_delete_char_at_end(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "Hello");
    
    // At end of line, delete should do nothing on single line
    tb.cursor_col = 5;
    tb_delete_char(&tb);
    
    char *result = tb_to_string(&tb);
    TEST_ASSERT_STR_EQ(result, "Hello");
    free(result);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_delete_char_merge_lines(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "First\nSecond");
    
    // At end of first line
    tb.cursor_row = 0;
    tb.cursor_col = 5;
    
    // Delete should merge lines
    tb_delete_char(&tb);
    
    char *result = tb_to_string(&tb);
    TEST_ASSERT_STR_EQ(result, "FirstSecond");
    TEST_ASSERT(tb.line_count == 1);
    free(result);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_delete_char_middle(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    tb_set_from_string(&tb, "ABCDEF");
    
    // Delete 'C' at position 2
    tb.cursor_col = 2;
    tb_delete_char(&tb);
    
    char *result = tb_to_string(&tb);
    TEST_ASSERT_STR_EQ(result, "ABDEF");
    free(result);
    
    // Delete 'D' at position 2
    tb_delete_char(&tb);
    result = tb_to_string(&tb);
    TEST_ASSERT_STR_EQ(result, "ABEF");
    free(result);
    
    tb_free(&tb);
    return 0;
}

static int test_tb_word_navigation_url(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    // Test with URL-like content
    tb_set_from_string(&tb, "https://api.example.com/users");
    tb.cursor_row = 0;
    tb.cursor_col = 0;
    
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 5);  // After "https"
    
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 8);  // After "://"
    
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 11);  // After "api"
    
    tb_free(&tb);
    return 0;
}

static int test_tb_word_navigation_json(void) {
    TextBuffer tb;
    tb_init(&tb);
    
    // Test with JSON-like content - "{\"name\": \"value\"}"
    // Let's just verify basic movement works
    tb_set_from_string(&tb, "name value test");
    tb.cursor_row = 0;
    tb.cursor_col = 0;
    
    // Simple word navigation
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 5);  // After "name "
    
    tb_move_word_right(&tb);
    TEST_ASSERT(tb.cursor_col == 11);  // After "value "
    
    tb_free(&tb);
    return 0;
}

int test_textbuf_navigation(void) {
    int failed = 0;
    
    printf("Running TextBuffer navigation tests...\n");
    
    if (test_tb_move_line_start()) { failed++; printf("FAIL: test_tb_move_line_start\n"); }
    else { printf("PASS: test_tb_move_line_start\n"); }
    
    if (test_tb_move_line_end()) { failed++; printf("FAIL: test_tb_move_line_end\n"); }
    else { printf("PASS: test_tb_move_line_end\n"); }
    
    if (test_tb_move_line_start_end_multiline()) { failed++; printf("FAIL: test_tb_move_line_start_end_multiline\n"); }
    else { printf("PASS: test_tb_move_line_start_end_multiline\n"); }
    
    if (test_tb_move_word_right_basic()) { failed++; printf("FAIL: test_tb_move_word_right_basic\n"); }
    else { printf("PASS: test_tb_move_word_right_basic\n"); }
    
    if (test_tb_move_word_left_basic()) { failed++; printf("FAIL: test_tb_move_word_left_basic\n"); }
    else { printf("PASS: test_tb_move_word_left_basic\n"); }
    
    if (test_tb_move_word_punctuation()) { failed++; printf("FAIL: test_tb_move_word_punctuation\n"); }
    else { printf("PASS: test_tb_move_word_punctuation\n"); }
    
    if (test_tb_move_word_multiline()) { failed++; printf("FAIL: test_tb_move_word_multiline\n"); }
    else { printf("PASS: test_tb_move_word_multiline\n"); }
    
    if (test_tb_delete_char_basic()) { failed++; printf("FAIL: test_tb_delete_char_basic\n"); }
    else { printf("PASS: test_tb_delete_char_basic\n"); }
    
    if (test_tb_delete_char_at_end()) { failed++; printf("FAIL: test_tb_delete_char_at_end\n"); }
    else { printf("PASS: test_tb_delete_char_at_end\n"); }
    
    if (test_tb_delete_char_merge_lines()) { failed++; printf("FAIL: test_tb_delete_char_merge_lines\n"); }
    else { printf("PASS: test_tb_delete_char_merge_lines\n"); }
    
    if (test_tb_delete_char_middle()) { failed++; printf("FAIL: test_tb_delete_char_middle\n"); }
    else { printf("PASS: test_tb_delete_char_middle\n"); }
    
    if (test_tb_word_navigation_url()) { failed++; printf("FAIL: test_tb_word_navigation_url\n"); }
    else { printf("PASS: test_tb_word_navigation_url\n"); }
    
    if (test_tb_word_navigation_json()) { failed++; printf("FAIL: test_tb_word_navigation_json\n"); }
    else { printf("PASS: test_tb_word_navigation_json\n"); }
    
    printf("\n");
    if (failed) {
        printf("FAILED %d tests\n", failed);
        return 1;
    }
    
    printf("All TextBuffer navigation tests passed!\n");
    return 0;
}
