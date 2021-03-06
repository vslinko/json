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

#include "./json.h"
#include <assert.h> /* assert */
#include <stdbool.h> /* true */
#include <stdlib.h> /* malloc, realloc, free */
#include <string.h> /* memcpy, memcmp, strcpy, strlen */
#include <vstd/object_pool.h>

#define JSON_MALLOC(pointer, size) \
    pointer = malloc(size); \
    assert(pointer);

#define JSON_REALLOC(pointer, size) \
    pointer = realloc(pointer, size); \
    assert(pointer);

#define JSON_FREE(pointer) \
    /*printf("free %s:%d\n", __FILE__, __LINE__);*/ \
    free(pointer);

#define JSON_ARRAY_CHUNK_SIZE 8
#define JSON_OBJECT_CHUNK_SIZE 8
#define JSON_ESCAPE_STRING_CHUNK_SIZE 64

static struct json_value *json_clone_value(const struct json_value *source);
static struct json_value *json_parse_value(void);
static void json_stringify_value(const struct json_value *value);
static void json_array_free(struct json_array *array);
static void json_object_free(struct json_object *object);

/*
 * JSON CONSTRUCTOR
 */

static char *escape_result;
static int escape_result_length;

static char true_value[5] = "true";
static char false_value[6] = "false";

#define POOL_ALLOC(type) \
    static struct vstd_object_pool *type##_pool; \
    static struct type *type##_alloc() { \
        struct type *value; \
        if (!type##_pool) { \
            type##_pool = vstd_object_pool_alloc(16, sizeof(struct type)); \
            assert(type##_pool); \
        } \
        value = vstd_object_pool_get(type##_pool); \
        assert(value); \
        return value; \
    }

POOL_ALLOC(json_value)
POOL_ALLOC(json_array)
POOL_ALLOC(json_object)
POOL_ALLOC(json_object_member)
POOL_ALLOC(json_parse_result)

static void json_escape_string_append_character(char character) {
    if (escape_result_length == 0) {
        JSON_MALLOC(escape_result, sizeof(char) * (JSON_ESCAPE_STRING_CHUNK_SIZE + 1))
    } else if (escape_result_length % JSON_ESCAPE_STRING_CHUNK_SIZE == 0) {
        /* cppcheck-suppress memleakOnRealloc */
        JSON_REALLOC(escape_result,
                     sizeof(char) * (escape_result_length + JSON_ESCAPE_STRING_CHUNK_SIZE + 1))
    }

    escape_result[escape_result_length++] = character;
    escape_result[escape_result_length] = '\0';
}

char *json_escape_string(const char *string) {
    size_t string_length, position;
    char character;

    string_length = strlen(string);
    escape_result_length = 0;

    for (position = 0; position < string_length; position++) {
        character = string[position];

        if (character == '"') {
            json_escape_string_append_character('\\');
        }

        json_escape_string_append_character(character);
    }

    return escape_result;
}

struct json_value *json_null_value() {
    struct json_value *null_value;

    null_value = json_value_alloc();
    null_value->type = JSON_NULL_VALUE;

    return null_value;
}

struct json_value *json_boolean_value(bool value) {
    struct json_value *boolean_value;

    boolean_value = json_value_alloc();
    boolean_value->type = JSON_BOOLEAN_VALUE;
    if (value) {
        boolean_value->value.boolean = true_value;
    } else {
        boolean_value->value.boolean = false_value;
    }

    return boolean_value;
}

struct json_value *json_string_value(const char *value) {
    struct json_value *string_value;

    string_value = json_value_alloc();
    string_value->type = JSON_STRING_VALUE;
    JSON_MALLOC(string_value->value.string, sizeof(char) * (strlen(value) + 1))
    strcpy(string_value->value.string, value);

    return string_value;
}

struct json_value *json_number_value(const char *value) {
    struct json_value *number_value;

    number_value = json_value_alloc();
    number_value->type = JSON_NUMBER_VALUE;
    JSON_MALLOC(number_value->value.number, sizeof(char) * (strlen(value) + 1))
    strcpy(number_value->value.number, value);

    return number_value;
}

struct json_value *json_array_value() {
    struct json_array *array;
    struct json_value *array_value;

    array = json_array_alloc();
    array->size = 0;
    array->values = NULL;

    array_value = json_value_alloc();
    array_value->type = JSON_ARRAY_VALUE;
    array_value->value.array = array;

    return array_value;
}

struct json_value *json_object_value() {
    struct json_object *object;
    struct json_value *object_value;

    object = json_object_alloc();
    object->size = 0;
    object->members = NULL;

    object_value = json_value_alloc();
    object_value->type = JSON_OBJECT_VALUE;
    object_value->value.object = object;

    return object_value;
}

static void _json_array_push(struct json_array *array, struct json_value *value) {
    if (array->size == 0) {
        JSON_MALLOC(array->values, sizeof(struct json_value *) * JSON_ARRAY_CHUNK_SIZE)
    } else if (array->size % JSON_ARRAY_CHUNK_SIZE == 0) {
        /* cppcheck-suppress memleakOnRealloc */
        JSON_REALLOC(array->values,
                     sizeof(struct json_value *) * (array->size + JSON_ARRAY_CHUNK_SIZE))
    }

    array->values[array->size++] = value;
}

void json_array_push(struct json_value *array_value, struct json_value *value) {
    assert(array_value->type == JSON_ARRAY_VALUE);
    _json_array_push(array_value->value.array, value);
}

static void _json_object_push(struct json_object *object, struct json_object_member *member) {
    if (object->size == 0) {
        JSON_MALLOC(object->members, sizeof(struct json_object_member *) * JSON_OBJECT_CHUNK_SIZE)
    } else if (object->size % JSON_OBJECT_CHUNK_SIZE == 0) {
        /* cppcheck-suppress memleakOnRealloc */
        JSON_REALLOC(object->members,
                     sizeof(struct json_object_member *) * (object->size + JSON_OBJECT_CHUNK_SIZE))
    }

    object->members[object->size++] = member;
}

void json_object_push(struct json_value *object_value, const char *name, struct json_value *value) {
    struct json_object_member *member;

    assert(object_value->type == JSON_OBJECT_VALUE);

    member = json_object_member_alloc();
    JSON_MALLOC(member->name, sizeof(char) * (strlen(name) + 1))
    strcpy(member->name, name);
    member->value = value;

    _json_object_push(object_value->value.object, member);
}

bool json_object_has(struct json_value *object_value, const char *name) {
    struct json_object *object;
    unsigned int i;

    assert(object_value->type == JSON_OBJECT_VALUE);

    object = object_value->value.object;

    for (i = 0; i < object->size; i++) {
        if (strcmp(object->members[i]->name, name) == 0) {
            return true;
        }
    }

    return false;
}

struct json_value *json_object_get(struct json_value *object_value, const char *name) {
    struct json_object *object;
    unsigned int i;

    assert(object_value->type == JSON_OBJECT_VALUE);
    object = object_value->value.object;

    for (i = 0; i < object->size; i++) {
        if (strcmp(object->members[i]->name, name) == 0) {
            return object->members[i]->value;
        }
    }

    return NULL;
}

/*
 * JSON CLONE
 */

static struct json_value *json_clone_array(const struct json_value *source) {
    struct json_value *array_value;
    unsigned int i;

    array_value = json_array_value();

    for (i = 0; i < source->value.array->size; i++) {
        json_array_push(array_value, json_clone_value(source->value.array->values[i]));
    }

    return array_value;
}

static struct json_value *json_clone_object(const struct json_value *source) {
    struct json_value *object_value;
    unsigned int i;

    object_value = json_object_value();

    for (i = 0; i < source->value.object->size; i++) {
        json_object_push(object_value,
                         source->value.object->members[i]->name,
                         json_clone_value(source->value.object->members[i]->value));
    }

    return object_value;
}

static struct json_value *json_clone_value(const struct json_value *source) {
    switch (source->type) {
        case JSON_NULL_VALUE:
            return json_null_value();

        case JSON_BOOLEAN_VALUE:
            return json_boolean_value(strcmp(source->value.boolean, "true") == 0);

        case JSON_STRING_VALUE:
            return json_string_value(source->value.string);

        case JSON_NUMBER_VALUE:
            return json_number_value(source->value.number);

        case JSON_ARRAY_VALUE:
            return json_clone_array(source);

        case JSON_OBJECT_VALUE:
            return json_clone_object(source);
    }
}

struct json_value *json_clone(const struct json_value *source) {
    return json_clone_value(source);
}

/*
 * JSON PARSER
 */

#define JSON_TOKEN_EOF 0x00
#define JSON_TOKEN_STRING 0x22
#define JSON_TOKEN_NUMBER 0x30
#define JSON_TOKEN_BEGIN_ARRAY 0x5B
#define JSON_TOKEN_BEGIN_OBJECT 0x7B
#define JSON_TOKEN_END_ARRAY 0x5D
#define JSON_TOKEN_END_OBJECT 0x7D
#define JSON_TOKEN_NAME_SEPARATOR 0x3A
#define JSON_TOKEN_VALUE_SEPARATOR 0x2C
#define JSON_TOKEN_FALSE 0x66
#define JSON_TOKEN_NULL 0x6E
#define JSON_TOKEN_TRUE 0x74
#define JSON_TOKEN_UNKNOWN 0xFF

#define json_is_whitespace(character) \
    (character == 0x20 || character == 0x09 || character == 0x0A || character == 0x0D)

#define json_is_digit(character) \
    (character >= '0' && character <= '9')

#define json_is_utf_escape(character) \
    ((character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') \
     || (character >= 'A' && character <= 'F'))

static const char *source;
static size_t source_length;
static unsigned int last_error;
static unsigned int last_error_position;
static unsigned int current_position;
static unsigned int next_token;
static char *token_value = NULL;
static unsigned int token_value_length;

static int json_get_next_token() {
    char current_character;

    token_value = NULL;
    token_value_length = 0;

    if (current_position >= source_length) {
        return JSON_TOKEN_EOF;
    }

    current_character = source[current_position];

    while (json_is_whitespace(current_character)) {
        current_character = source[++current_position];
    }

    if (current_position >= source_length) {
        return JSON_TOKEN_EOF;
    }

    switch (current_character) {
        case JSON_TOKEN_BEGIN_ARRAY:
        case JSON_TOKEN_BEGIN_OBJECT:
        case JSON_TOKEN_END_ARRAY:
        case JSON_TOKEN_END_OBJECT:
        case JSON_TOKEN_NAME_SEPARATOR:
        case JSON_TOKEN_VALUE_SEPARATOR:
            current_position++;
            return current_character;

        case JSON_TOKEN_STRING:
            current_character = source[++current_position];

            while (current_position < source_length) {
                if (current_character == '\n' || current_character == '\t' || current_character == '\0') {
                    return JSON_TOKEN_UNKNOWN;
                }

                if (current_character == '\\') {
                    if (source_length - current_position < 2) {
                        return JSON_TOKEN_UNKNOWN;
                    }

                    current_character = source[++current_position];

                    switch (current_character) {
                        case '"':
                        case '/':
                        case '\\':
                        case 'b':
                        case 'f':
                        case 'n':
                        case 'r':
                        case 't':
                            current_character = source[++current_position];
                            token_value_length += 2;
                            continue;

                        case 'u':
                            if (source_length - current_position < 5) {
                                return JSON_TOKEN_UNKNOWN;
                            }
                            current_character = source[++current_position];
                            if (!json_is_utf_escape(current_character)) {
                                return JSON_TOKEN_UNKNOWN;
                            }
                            current_character = source[++current_position];
                            if (!json_is_utf_escape(current_character)) {
                                return JSON_TOKEN_UNKNOWN;
                            }
                            current_character = source[++current_position];
                            if (!json_is_utf_escape(current_character)) {
                                return JSON_TOKEN_UNKNOWN;
                            }
                            current_character = source[++current_position];
                            if (!json_is_utf_escape(current_character)) {
                                return JSON_TOKEN_UNKNOWN;
                            }
                            current_character = source[++current_position];
                            token_value_length += 6;
                            continue;

                        default:
                            return JSON_TOKEN_UNKNOWN;
                    }
                }

                if (current_character == '"') {
                    JSON_MALLOC(token_value, sizeof(char) * (token_value_length + 1))
                    memcpy(token_value,
                           source + current_position - token_value_length,
                           token_value_length);
                    token_value[token_value_length] = '\0';
                    current_position++;
                    return JSON_TOKEN_STRING;
                }

                current_character = source[++current_position];
                token_value_length++;
            }
            break;

        case JSON_TOKEN_FALSE:
            if (source_length - current_position >= 5
                && memcmp(source + current_position, "false", 5) == 0) {
                current_position += 5;
                token_value = false_value;
                return JSON_TOKEN_FALSE;
            }
            break;

        case JSON_TOKEN_TRUE:
            if (source_length - current_position >= 4
                && memcmp(source + current_position, "true", 4) == 0) {
                current_position += 4;
                token_value = true_value;
                return JSON_TOKEN_TRUE;
            }
            break;

        case JSON_TOKEN_NULL:
            if (source_length - current_position >= 4
                && memcmp(source + current_position, "null", 4) == 0) {
                current_position += 4;
                return JSON_TOKEN_NULL;
            }
            break;
    }

    if (current_character == '-' || json_is_digit(current_character)) {
        if (current_character == '-') {
            current_character = source[++current_position];
            token_value_length++;
        }

        if (current_character == '0') {
            current_character = source[++current_position];
            token_value_length++;
        } else if (json_is_digit(current_character)) {
            while (json_is_digit(current_character)) {
                current_character = source[++current_position];
                token_value_length++;
            }
        } else {
            return JSON_TOKEN_UNKNOWN;
        }

        if (current_character == '.') {
            current_character = source[++current_position];
            token_value_length++;

            if (!json_is_digit(current_character)) {
                return JSON_TOKEN_UNKNOWN;
            }

            while (json_is_digit(current_character)) {
                current_character = source[++current_position];
                token_value_length++;
            }
        }

        if (current_character == 'e' || current_character == 'E') {
            current_character = source[++current_position];
            token_value_length++;

            if (current_character == '-' || current_character == '+') {
                current_character = source[++current_position];
                token_value_length++;
            }

            if (!json_is_digit(current_character)) {
                return JSON_TOKEN_UNKNOWN;
            }

            while (json_is_digit(current_character)) {
                current_character = source[++current_position];
                token_value_length++;
            }
        }

        JSON_MALLOC(token_value, sizeof(char) * (token_value_length + 1))
        memcpy(token_value, source + current_position - token_value_length, token_value_length);
        token_value[token_value_length] = '\0';
        return JSON_TOKEN_NUMBER;
    }

    return JSON_TOKEN_UNKNOWN;
}

static struct json_object_member *json_parse_object_member() {
    char *name;
    struct json_value *value;
    struct json_object_member *member;

    if (next_token != JSON_TOKEN_STRING) {
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position - token_value_length;
        return NULL;
    }

    name = token_value;
    token_value = NULL;
    next_token = json_get_next_token();

    if (next_token != JSON_TOKEN_NAME_SEPARATOR) {
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position - token_value_length;
        JSON_FREE(name)
        return NULL;
    }

    next_token = json_get_next_token();
    value = json_parse_value();

    if (!value) {
        JSON_FREE(name)
        return NULL;
    }

    member = json_object_member_alloc();
    member->name = name;
    member->value = value;
    return member;
}

static struct json_object *json_parse_object() {
    struct json_object *object;
    struct json_object_member *member;

    next_token = json_get_next_token();

    object = json_object_alloc();
    object->size = 0;

    if (next_token == JSON_TOKEN_END_OBJECT) {
        next_token = json_get_next_token();
        object->members = NULL;
        return object;
    }

    while (true) {
        member = json_parse_object_member();

        if (!member) {
            json_object_free(object);
            return NULL;
        }

        _json_object_push(object, member);

        if (next_token != JSON_TOKEN_VALUE_SEPARATOR) {
            break;
        }

        next_token = json_get_next_token();
    }

    if (next_token != JSON_TOKEN_END_OBJECT) {
        json_object_free(object);
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position;
        return NULL;
    }

    next_token = json_get_next_token();

    return object;
}

static struct json_array *json_parse_array() {
    struct json_array *array;
    struct json_value *value;

    next_token = json_get_next_token();

    array = json_array_alloc();
    array->size = 0;

    if (next_token == JSON_TOKEN_END_ARRAY) {
        next_token = json_get_next_token();
        array->values = NULL;
        return array;
    }

    while (true) {
        value = json_parse_value();

        if (!value) {
            json_array_free(array);
            return NULL;
        }

        _json_array_push(array, value);

        if (next_token != JSON_TOKEN_VALUE_SEPARATOR) {
            break;
        }

        next_token = json_get_next_token();
    }

    if (next_token != JSON_TOKEN_END_ARRAY) {
        json_array_free(array);
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position;
        return NULL;
    }

    next_token = json_get_next_token();

    return array;
}

#define json_parse_value_case_value(token_type, value_type, value_property) \
    case token_type: \
        value = json_value_alloc(); \
        value->type = value_type; \
        value->value_property = token_value; \
        token_value = NULL; \
        next_token = json_get_next_token(); \
        return value;

static struct json_value *json_parse_value() {
    struct json_value *value;
    struct json_object *object;
    struct json_array *array;

    switch (next_token) {
        json_parse_value_case_value(JSON_TOKEN_STRING, JSON_STRING_VALUE, value.string);
        json_parse_value_case_value(JSON_TOKEN_NUMBER, JSON_NUMBER_VALUE, value.number);
        json_parse_value_case_value(JSON_TOKEN_FALSE, JSON_BOOLEAN_VALUE, value.boolean);
        json_parse_value_case_value(JSON_TOKEN_TRUE, JSON_BOOLEAN_VALUE, value.boolean);

        case JSON_TOKEN_NULL:
            value = json_value_alloc();
            value->type = JSON_NULL_VALUE;
            token_value = NULL;
            next_token = json_get_next_token();
            return value;

        case JSON_TOKEN_BEGIN_ARRAY:
            array = json_parse_array();
            if (!array) {
                return NULL;
            }
            value = json_value_alloc();
            value->type = JSON_ARRAY_VALUE;
            value->value.array = array;
            return value;

        case JSON_TOKEN_BEGIN_OBJECT:
            object = json_parse_object();
            if (!object) {
                return NULL;
            }
            value = json_value_alloc();
            value->type = JSON_OBJECT_VALUE;
            value->value.object = object;
            return value;

        default:
            last_error = JSON_ERROR_UNEXPECTED_TOKEN;
            last_error_position = current_position;
            return NULL;
    }
}

static struct json_value *json_parse_file() {
    struct json_value *value;

    value = json_parse_value();

    if (!value) {
        return NULL;
    }

    if (next_token != JSON_TOKEN_EOF) {
        json_value_free(value);
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position;
        return NULL;
    }

    return value;
}

struct json_parse_result *json_parse(const char *json, unsigned int length) {
    struct json_parse_result *result;

    result = json_parse_result_alloc();

    source = json;
    source_length = length;

    if (source_length == 0) {
        result->error = JSON_ERROR_EMPTY_FILE;
        result->error_position = 0;
        result->value = NULL;
        return result;
    }

    last_error = 0;
    last_error_position = 0;
    current_position = 0;
    next_token = json_get_next_token();

    result->value = json_parse_file();

    if (last_error > 0) {
        result->error = last_error;
        result->error_position = last_error_position;

        if (token_value && token_value != false_value && token_value != true_value) {
            JSON_FREE(token_value)
        }
    } else {
        result->error = result->error_position = 0;
    }

    return result;
}

/*
 * JSON STRINGIFIER
 */

#define JSON_STRINGIFY_CHUNK_SIZE 64

static char *stringify_result;
static size_t stringify_result_length;
static size_t result_allocated_chunks;

static void json_stringify_init_string() {
    JSON_MALLOC(stringify_result, sizeof(char) * (JSON_STRINGIFY_CHUNK_SIZE + 1))
    stringify_result[0] = '\0';
    stringify_result_length = 0;
    result_allocated_chunks = 1;
}

static void json_stringify_append_character(char character) {
    if (stringify_result_length > 0 && stringify_result_length % JSON_STRINGIFY_CHUNK_SIZE == 0) {
        /* cppcheck-suppress memleakOnRealloc */
        JSON_REALLOC(stringify_result,
                     sizeof(char) * (stringify_result_length + JSON_STRINGIFY_CHUNK_SIZE + 1))
        result_allocated_chunks++;
    }

    stringify_result[stringify_result_length++] = character;
    stringify_result[stringify_result_length] = '\0';
}

static void json_stringify_append_string(const char *string) {
    size_t string_length, allocated_length, new_string_length, new_string_chunks;

    string_length = strlen(string);
    allocated_length = result_allocated_chunks * JSON_STRINGIFY_CHUNK_SIZE;
    new_string_length = stringify_result_length + string_length;

    if (new_string_length > allocated_length) {
        new_string_chunks = new_string_length / JSON_STRINGIFY_CHUNK_SIZE + 1;
        /* cppcheck-suppress memleakOnRealloc */
        JSON_REALLOC(stringify_result,
                     sizeof(char) * (JSON_STRINGIFY_CHUNK_SIZE * new_string_chunks))
        result_allocated_chunks = new_string_chunks;
    }

    memcpy(stringify_result + stringify_result_length, string, string_length);
    stringify_result_length = new_string_length;
    stringify_result[new_string_length] = '\0';
}

static void json_stringify_array(const struct json_array *array) {
    unsigned int i;

    json_stringify_append_character('[');

    for (i = 0; i < array->size; i++) {
        if (i != 0) {
            json_stringify_append_character(',');
        }

        json_stringify_value(array->values[i]);
    }

    json_stringify_append_character(']');
}

static void json_stringify_object(const struct json_object *object) {
    unsigned int i;

    json_stringify_append_character('{');

    for (i = 0; i < object->size; i++) {
        if (i != 0) {
            json_stringify_append_character(',');
        }

        json_stringify_append_character('"');
        json_stringify_append_string(object->members[i]->name);
        json_stringify_append_character('"');
        json_stringify_append_character(':');
        json_stringify_value(object->members[i]->value);
    }

    json_stringify_append_character('}');
}

static void json_stringify_value(const struct json_value *value) {
    switch (value->type) {
        case JSON_NULL_VALUE:
            json_stringify_append_string("null");
            break;

        case JSON_BOOLEAN_VALUE:
            json_stringify_append_string(value->value.boolean);
            break;

        case JSON_NUMBER_VALUE:
            json_stringify_append_string(value->value.number);
            break;

        case JSON_STRING_VALUE:
            json_stringify_append_character('"');
            json_stringify_append_string(value->value.string);
            json_stringify_append_character('"');
            break;

        case JSON_ARRAY_VALUE:
            json_stringify_array(value->value.array);
            break;

        case JSON_OBJECT_VALUE:
            json_stringify_object(value->value.object);
            break;
    }
}

char *json_stringify(const struct json_value *value) {
    char *result;

    json_stringify_init_string();

    json_stringify_value(value);

    result = stringify_result;
    stringify_result = NULL;

    return result;
}

/*
 * FREE FUNCTIONS
 */

static void json_array_free(struct json_array *array) {
    unsigned int i;

    if (array->size > 0) {
        for (i = 0; i < array->size; i++) {
            json_value_free(array->values[i]);
        }

        JSON_FREE(array->values)
    }

    vstd_object_pool_return(json_array_pool, array);
}

static void json_object_free(struct json_object *object) {
    unsigned int i;

    if (object->size > 0) {
        for (i = 0; i < object->size; i++) {
            JSON_FREE(object->members[i]->name)
            json_value_free(object->members[i]->value);
            vstd_object_pool_return(json_object_member_pool, object->members[i]);
        }

        JSON_FREE(object->members)
    }

    vstd_object_pool_return(json_object_pool, object);
}

void json_value_free(struct json_value *value) {
    switch (value->type) {
        case JSON_NULL_VALUE:
        case JSON_BOOLEAN_VALUE:
            break;

        case JSON_NUMBER_VALUE:
            JSON_FREE(value->value.number)
            break;

        case JSON_STRING_VALUE:
            JSON_FREE(value->value.string)
            break;

        case JSON_ARRAY_VALUE:
            json_array_free(value->value.array);
            break;

        case JSON_OBJECT_VALUE:
            json_object_free(value->value.object);
            break;
    }

    vstd_object_pool_return(json_value_pool, value);
}

void json_parse_result_free(struct json_parse_result *parse_result) {
    vstd_object_pool_return(json_parse_result_pool, parse_result);
}
