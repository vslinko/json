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

#include <vstd/test.h>
#include <string.h>
#include "json.h"

#define assert_successful_parsing(source_json) \
    parse_result = json_parse(source_json); \
    assert(parse_result != NULL); \
    assert(parse_result->error == 0); \
    assert(parse_result->value != NULL); \
    stringify_result = json_stringify(parse_result->value); \
    assert(strcmp(source_json, stringify_result) == 0); \
    json_value_free(parse_result->value); \
    json_parse_result_free(parse_result); \
    free(stringify_result);

#define assert_failed_parsing(source_json, _error, _error_position) \
    parse_result = json_parse(source_json); \
    assert(parse_result != NULL); \
    assert(parse_result->error > 0); \
    assert(parse_result->value == NULL); \
    assert(parse_result->error == _error); \
    assert(parse_result->error_position == _error_position); \
    json_parse_result_free(parse_result);

static struct json_parse_result *parse_result;
static char *stringify_result;

static void vstd_setup() {}

static void vstd_teardown() {}

vstd_test_unit(json, 100000, {
    assert_successful_parsing("null")
    assert_successful_parsing("true")
    assert_successful_parsing("false")
    assert_successful_parsing("1")
    assert_successful_parsing("1.0")
    assert_successful_parsing("-1.0")
    assert_successful_parsing("-1.0e-1")
    assert_successful_parsing("-1.0e+2")
    assert_successful_parsing("-1.0E3")
    assert_successful_parsing("\"string\"")
    assert_successful_parsing("\"string with \\\" escaped quote\"")
    assert_successful_parsing("\"UTF-8 строка\"")
    assert_successful_parsing("[null,true,false,1,\"string\"]")
    assert_successful_parsing("{\"array\":[null,true,false,1,\"string\"]}")

    assert_failed_parsing("", JSON_ERROR_EMPTY_FILE, 0)
    assert_failed_parsing("string without quotes", JSON_ERROR_UNEXPECTED_TOKEN, 0)
    assert_failed_parsing("[1", JSON_ERROR_UNEXPECTED_TOKEN, 2)
    assert_failed_parsing("{1}", JSON_ERROR_UNEXPECTED_TOKEN, 1)
    assert_failed_parsing("{", JSON_ERROR_UNEXPECTED_TOKEN, 1)
    assert_failed_parsing("{\"a\" 1}", JSON_ERROR_UNEXPECTED_TOKEN, 5)
})

/*
 * 400 000 times per 1 second
 */
vstd_test_benchmark(json_parse_benchmark, 1.0, {
    char *source = "{\"array\":[null,true,false,-1.0e-2,\"string\"]}";

    for (int i = 0; i < 400000; i++) {
        struct json_parse_result *result = json_parse(source);
        json_value_free(result->value);
        json_parse_result_free(result);
    }
})
