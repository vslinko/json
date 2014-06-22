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

#ifndef JSON_JSON_H
#define JSON_JSON_H

#include <stdbool.h>

#define JSON_VERSION "json 1.0.0"

#define JSON_ERROR_EMPTY_FILE 1
#define JSON_ERROR_UNEXPECTED_TOKEN 2

enum json_value_type {
    JSON_NULL_VALUE,
    JSON_BOOLEAN_VALUE,
    JSON_NUMBER_VALUE,
    JSON_STRING_VALUE,
    JSON_ARRAY_VALUE,
    JSON_OBJECT_VALUE
};

struct json_array {
    unsigned int size;
    struct json_value **values;
};

struct json_object_member {
    char *name;
    struct json_value *value;
};

struct json_object {
    unsigned int size;
    struct json_object_member **members;
};

struct json_value {
    enum json_value_type type;

    union {
        char *boolean_value;
        char *number_value;
        char *string_value;
        struct json_array *array_value;
        struct json_object *object_value;
    };
};

struct json_parse_result {
    unsigned int error;
    unsigned int error_position;
    struct json_value *value;
};

char *json_escape_string(const char *string);

struct json_value *json_null_value();
struct json_value *json_boolean_value(bool value);
struct json_value *json_string_value(const char *value);
struct json_value *json_number_value(const char *value);
struct json_value *json_array_value();
struct json_value *json_object_value();
void json_array_push(struct json_value *array_value, struct json_value *value);
void json_object_push(struct json_value *object_value, const char *name, struct json_value *value);
bool json_object_has(struct json_value *object_value, const char *name);
struct json_value *json_object_get(struct json_value *object_value, const char *name);

struct json_value *json_clone(const struct json_value *source);

struct json_parse_result *json_parse(const char *json);

char *json_stringify(const struct json_value *value);

void json_value_free(struct json_value *value);
void json_parse_result_free(struct json_parse_result *parse_result);

#endif
