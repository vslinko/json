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

#include <assert.h> /* assert */
#include <stdarg.h> /* va_list, va_start, va_end */
#include <stdlib.h> /* free */
#include <string.h> /* strcmp */
#include "json_search.h"
#include "json_template.h"

/*
 * TABLE OF CONTENTS
 */

struct json_value *json_combine(size_t arguments_length, ...);
struct json_value *json_combine_array(struct json_value **values, size_t size);

static struct json_value *json_compile_object(struct json_value *instruction);
static struct json_value *json_compile_path(struct json_value *instruction);
static struct json_value *json_compile_instruction(struct json_value *instruction);
struct json_value *json_compile(struct json_value *json_template, struct json_value *data);

/*
 * JSON COMBINE
 */

struct json_value *json_combine(size_t arguments_length, ...) {
    va_list arguments;
    va_start(arguments, arguments_length);

    struct json_value *array = json_array_value();
    struct json_value *object = json_object_value();
    json_object_push(object, "srcs", array);

    for (size_t i = 0; i < arguments_length; i++) {
        struct json_value *value = va_arg(arguments, struct json_value *);
        json_array_push(array, value);
    }

    va_end(arguments);

    return object;
}

struct json_value *json_combine_array(struct json_value **values, size_t size) {
    struct json_value *array = json_array_value();
    struct json_value *object = json_object_value();
    json_object_push(object, "srcs", array);

    for (size_t i = 0; i < size; i++) {
        json_array_push(array, values[i]);
    }

    return object;
}

/*
 * JSON COMPILE
 */

static struct json_value *context;

static struct json_value *json_compile_object(struct json_value *instruction) {
    struct json_value *properties_value = json_object_get(instruction, "properties");
    assert(properties_value);
    assert(properties_value->type == JSON_OBJECT_VALUE);

    struct json_object *properties = properties_value->object_value;

    struct json_value *result = json_object_value();

    for (unsigned int i = 0; i < properties->size; i++) {
        struct json_value *property = json_compile_instruction(properties->members[i]->value);

        if (property != NULL) {
            json_object_push(result, properties->members[i]->name, property);
        }
    }

    return result;
}

static struct json_value *json_compile_path(struct json_value *instruction) {
    struct json_value *path_value = json_object_get(instruction, "path");
    assert(path_value);
    assert(path_value->type == JSON_STRING_VALUE);

    struct json_value *value = json_search(context, path_value->string_value);

    if (value == NULL) {
        return NULL;
    }

    value = json_clone(value);

    if (json_object_has(instruction, "stringify")) {
        struct json_value *stringify_value = json_object_get(instruction, "stringify");
        assert(stringify_value);
        assert(stringify_value->type == JSON_BOOLEAN_VALUE);

        if (strcmp(stringify_value->boolean_value, "true") == 0) {
            char *stringify = json_stringify(value);
            char *escaped_stringify = json_escape_string(stringify);
            value = json_string_value(escaped_stringify);
            free(escaped_stringify);
            free(stringify);
        }
    }

    return value;
}

static struct json_value *json_compile_instruction(struct json_value *instruction) {
    assert(instruction->type == JSON_OBJECT_VALUE);

    if (json_object_has(instruction, "properties")) {
        return json_compile_object(instruction);
    } else if (json_object_has(instruction, "path")) {
        return json_compile_path(instruction);
    }

    return NULL;
}

struct json_value *json_compile(struct json_value *json_template, struct json_value *data) {
    context = data;
    struct json_value *result = json_compile_instruction(json_template);
    context = NULL;

    return result;
}
