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
#include "./json.h"

#define ASSERT_SUCCESSFUL_PARSING(source_json) \
    parse_result = json_parse(source_json, strlen(source_json)); \
    tassert(parse_result); \
    tassert(parse_result->error == 0); \
    tassert(parse_result->value); \
    stringify_result = json_stringify(parse_result->value); \
    tassert(strcmp(source_json, stringify_result) == 0); \
    json_value_free(parse_result->value); \
    json_parse_result_free(parse_result); \
    free(stringify_result);

#define ASSERT_FAILED_PARSING(source_json, _error, _error_position) \
    parse_result = json_parse(source_json, strlen(source_json)); \
    tassert(parse_result); \
    tassert(parse_result->error > 0); \
    tassert(!parse_result->value); \
    tassert(parse_result->error == _error); \
    tassert(parse_result->error_position == _error_position); \
    json_parse_result_free(parse_result);

static struct json_parse_result *parse_result;
static char *stringify_result;

static void test_json() {
    ASSERT_SUCCESSFUL_PARSING("null")
    ASSERT_SUCCESSFUL_PARSING("true")
    ASSERT_SUCCESSFUL_PARSING("false")
    ASSERT_SUCCESSFUL_PARSING("1")
    ASSERT_SUCCESSFUL_PARSING("1.0")
    ASSERT_SUCCESSFUL_PARSING("-1.0")
    ASSERT_SUCCESSFUL_PARSING("-1.0e-1")
    ASSERT_SUCCESSFUL_PARSING("-1.0e+2")
    ASSERT_SUCCESSFUL_PARSING("-1.0E3")
    ASSERT_SUCCESSFUL_PARSING("\"string\"")
    ASSERT_SUCCESSFUL_PARSING("\"string with \\\" escaped quote\"")
    ASSERT_SUCCESSFUL_PARSING("\"UTF-8 строка\"")
    ASSERT_SUCCESSFUL_PARSING("[null,true,false,1,\"string\"]")
    ASSERT_SUCCESSFUL_PARSING("{\"array\":[null,true,false,1,\"string\"]}")
    ASSERT_SUCCESSFUL_PARSING("[[],[]]")
    ASSERT_SUCCESSFUL_PARSING("{\"a\":1,\"b\":2}")
    ASSERT_SUCCESSFUL_PARSING("{\"a\":{},\"b\":{}}")

    ASSERT_FAILED_PARSING("", JSON_ERROR_EMPTY_FILE, 0)
    ASSERT_FAILED_PARSING("string without quotes", JSON_ERROR_UNEXPECTED_TOKEN, 0)
    ASSERT_FAILED_PARSING("[1", JSON_ERROR_UNEXPECTED_TOKEN, 2)
    ASSERT_FAILED_PARSING("{1}", JSON_ERROR_UNEXPECTED_TOKEN, 1)
    ASSERT_FAILED_PARSING("{", JSON_ERROR_UNEXPECTED_TOKEN, 1)
    ASSERT_FAILED_PARSING("{\"a\": 1", JSON_ERROR_UNEXPECTED_TOKEN, 7)
    ASSERT_FAILED_PARSING("{\"a\" 1}", JSON_ERROR_UNEXPECTED_TOKEN, 5)
}
VSTD_TEST_REGISTER_UNIT(test_json, 10000, NULL, NULL)

/*
 * 1 000 000 times per 1 second
 */
#define PARSE_BENCHMARK_TRIES 1000000
static char sources[PARSE_BENCHMARK_TRIES][100];
static struct json_parse_result **links;

static void benchmark_json_parse_setup() {
    int i;

    for (i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        sprintf(sources[i], "{\"array\":[null,true,false,-1.0e-2,\"string\",%f]}", ((double) rand()) / RAND_MAX);
    }

    links = malloc(sizeof(struct json_parse_result *) * PARSE_BENCHMARK_TRIES);
    tassert(links);
}

static void benchmark_json_parse_teardown() {
    int i;

    for (i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value_free(links[i]->value);
        json_parse_result_free(links[i]);
    }
    free(links);
}

static void benchmark_json_parse() {
    struct json_parse_result *result;
    int i;

    for (i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        result = json_parse(sources[i], strlen(sources[i]));
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
    char *source;
    struct json_parse_result *result;
    char *stringified;
    int i;

    source = "{\"array\":[null,true,false,-1.0e-2,\"string\"]}";

    result = json_parse(source, strlen(source));

    for (i = 0; i < 7500000; i++) {
        stringified = json_stringify(result->value);
        free(stringified);
    }

    json_value_free(result->value);
    json_parse_result_free(result);
}
VSTD_TEST_REGISTER_BENCHMARK(benchmark_json_stringify, 1.0, NULL, NULL)
