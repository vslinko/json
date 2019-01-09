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

#include "json.h"

#define assert_successful_parsing(source_json) \
    parse_result = json_parse(source_json, strlen(source_json)); \
    tassert(parse_result != NULL); \
    tassert(parse_result->error == 0); \
    tassert(parse_result->value != NULL); \
    stringify_result = json_stringify(parse_result->value); \
    tassert(strcmp(source_json, stringify_result) == 0); \
    json_value_free(parse_result->value); \
    json_parse_result_free(parse_result); \
    free(stringify_result);

#define assert_failed_parsing(source_json, _error, _error_position) \
    parse_result = json_parse(source_json, strlen(source_json)); \
    tassert(parse_result != NULL); \
    tassert(parse_result->error > 0); \
    tassert(parse_result->value == NULL); \
    tassert(parse_result->error == _error); \
    tassert(parse_result->error_position == _error_position); \
    json_parse_result_free(parse_result);

static struct json_parse_result* parse_result;
static char* stringify_result;

static void test_json() {
    assert_successful_parsing("null");
    assert_successful_parsing("true");
    assert_successful_parsing("false");
    assert_successful_parsing("1");
    assert_successful_parsing("1.0");
    assert_successful_parsing("-1.0");
    assert_successful_parsing("-1.0e-1");
    assert_successful_parsing("-1.0e+2");
    assert_successful_parsing("-1.0E3");
    assert_successful_parsing("\"string\"");
    assert_successful_parsing("\"string with \\\" escaped quote\"");
    assert_successful_parsing("\"UTF-8 строка\"");
    assert_successful_parsing("[null,true,false,1,\"string\"]");
    assert_successful_parsing("{\"array\":[null,true,false,1,\"string\"]}");
    assert_successful_parsing("[[],[]]");
    assert_successful_parsing("{\"a\":1,\"b\":2}");
    assert_successful_parsing("{\"a\":{},\"b\":{}}");

    assert_failed_parsing("", JSON_ERROR_EMPTY_FILE, 0);
    assert_failed_parsing("string without quotes", JSON_ERROR_UNEXPECTED_TOKEN, 0);
    assert_failed_parsing("[1", JSON_ERROR_UNEXPECTED_TOKEN, 2);
    assert_failed_parsing("{1}", JSON_ERROR_UNEXPECTED_TOKEN, 1);
    assert_failed_parsing("{", JSON_ERROR_UNEXPECTED_TOKEN, 1);
    assert_failed_parsing("{\"a\": 1", JSON_ERROR_UNEXPECTED_TOKEN, 7);
    assert_failed_parsing("{\"a\" 1}", JSON_ERROR_UNEXPECTED_TOKEN, 5);
}
VSTD_TEST_REGISTER_UNIT(test_json, 10000, NULL, NULL)

/*
 * 1 000 000 times per 1 second
 */
#define PARSE_BENCHMARK_TRIES 1000000
static char sources[PARSE_BENCHMARK_TRIES][100];
static struct json_parse_result** links;

static void benchmark_json_parse_setup() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        sprintf(sources[i], "{\"array\":[null,true,false,-1.0e-2,\"string\",%f]}", ((double) rand()) / RAND_MAX);
    }

    links = malloc(sizeof(struct json_parse_result*) * PARSE_BENCHMARK_TRIES);
}

static void benchmark_json_parse_teardown() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value_free(links[i]->value);
        json_parse_result_free(links[i]);
    }
    free(links);
}

static void benchmark_json_parse() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        struct json_parse_result* result = json_parse(sources[i], strlen(sources[i]));
        links[i] = result;
    }
}
VSTD_TEST_REGISTER_BENCHMARK(benchmark_json_parse,
                             1.0,
                             benchmark_json_parse_setup,
                             benchmark_json_parse_teardown)

/*
 * 7 500 000 times per 1 second
 */
static void benchmark_json_stringify() {
    char* source = "{\"array\":[null,true,false,-1.0e-2,\"string\"]}";
    struct json_parse_result* result = json_parse(source, strlen(source));

    for (int i = 0; i < 7500000; i++) {
        char* stringified = json_stringify(result->value);
        free(stringified);
    }

    json_value_free(result->value);
    json_parse_result_free(result);
}
VSTD_TEST_REGISTER_BENCHMARK(benchmark_json_stringify, 1.0, NULL, NULL)
