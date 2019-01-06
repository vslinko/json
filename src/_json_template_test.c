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

#include "json_template.h"

#define assert_successful(template, data, espected) \
    parse_result = json_parse(template); \
    assert(parse_result); \
    template_value = parse_result->value; \
    free(parse_result); \
    parse_result = json_parse(data); \
    assert(parse_result); \
    data_value = parse_result->value; \
    free(parse_result); \
    result_value = json_compile(template_value, data_value); \
    result = json_stringify(result_value); \
    assert(strcmp(result, espected) == 0); \
    free(result); \
    json_value_free(result_value); \
    json_value_free(data_value); \
    json_value_free(template_value);

static struct json_parse_result* parse_result;
static struct json_value* template_value;
static struct json_value* data_value;
static struct json_value* result_value;
static char* result;

static void vstd_setup() {}

static void vstd_teardown() {}

vstd_test_unit(json_template, 10000, {
    assert_successful("{\"properties\":{\"b\":{\"path\":\"a\"}}}", "{\"a\":1}", "{\"b\":1}");
})
