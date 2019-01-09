/* Copyright (c) 2014 Vyacheslav Slinko
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>
#include <vstd/test.h>

#include "json_search.h"

#define assert_successfull_search(source, path, result) \
    parse_result = json_parse(source, strlen(source)); \
    tassert(parse_result); \
    tassert(parse_result->error == 0); \
    found = json_search(parse_result->value, path); \
    tassert(found); \
    stringified = json_stringify(found); \
    tassert(strcmp(stringified, result) == 0); \
    free(stringified); \
    json_value_free(parse_result->value); \
    json_parse_result_free(parse_result);

#define assert_empty_search(source, path) \
    parse_result = json_parse(source, strlen(source)); \
    tassert(parse_result); \
    tassert(parse_result->error == 0); \
    found = json_search(parse_result->value, path); \
    tassert(found == NULL); \
    json_value_free(parse_result->value); \
    json_parse_result_free(parse_result);

#define assert_failed_search(source, path) \
    parse_result = json_parse(source, strlen(source)); \
    tassert(parse_result); \
    tassert(parse_result->error == 0); \
    json_search(parse_result->value, path);

static char* source = "{\"a\":1,\"b\":[2,{\"c\":[1,2,3]}]}";
static struct json_parse_result* parse_result;
static struct json_value* found;
static char* stringified;

static void test_json_search_successfull() {
    assert_successfull_search(source, "a", "1");
    assert_successfull_search(source, "b", "[2,{\"c\":[1,2,3]}]");
    assert_successfull_search(source, "b[0]", "2");
    assert_successfull_search(source, "b[1]", "{\"c\":[1,2,3]}");
    assert_successfull_search(source, "b[1].c", "[1,2,3]");
    assert_successfull_search(source, "b[1].c[0]", "1");
    assert_successfull_search(source, "b[1].c[1]", "2");
    assert_successfull_search(source, "b[1].c[2]", "3");
}
VSTD_TEST_REGISTER_UNIT(test_json_search_successfull, 10000, NULL, NULL)

static void test_json_search_empty() {
    assert_empty_search(source, "c");
    assert_empty_search(source, "b[2]");
    assert_empty_search(source, "b[1].c[3]");
}
VSTD_TEST_REGISTER_UNIT(test_json_search_empty, 10000, NULL, NULL)

static void test_json_search_abort_unclosed_array() {
    assert_failed_search(source, "[");
}
VSTD_TEST_REGISTER_ABORT(test_json_search_abort_unclosed_array, NULL, NULL)

static void test_json_search_abort_empty_array_index() {
    assert_failed_search(source, "[]");
}
VSTD_TEST_REGISTER_ABORT(test_json_search_abort_empty_array_index, NULL, NULL)

static void test_json_search_abort_empty_property_name() {
    assert_failed_search(source, "a.");
}
VSTD_TEST_REGISTER_ABORT(test_json_search_abort_empty_property_name, NULL, NULL)

static void test_json_search_abort_starts_with_dot() {
    assert_failed_search(source, ".a");
}
VSTD_TEST_REGISTER_ABORT(test_json_search_abort_starts_with_dot, NULL, NULL)
