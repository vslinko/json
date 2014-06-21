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
#include <stdbool.h> /* true */
#include <string.h> /* memcpy, memcmp, strcpy, strlen */
#include <stdlib.h> /* malloc, realloc, free */
#include "json.h"

/*
 * TABLE OF CONTENTS
 */

#define json_malloc(pointer, size) \
    pointer = malloc(size); \
    assert(pointer);

#define json_realloc(pointer, size) \
    pointer = realloc(pointer, size); \
    assert(pointer);

#define JSON_ARRAY_CHUNK_SIZE 8
#define JSON_OBJECT_CHUNK_SIZE 8

struct json_value *json_null_value();
struct json_value *json_boolean_value(bool value);
struct json_value *json_string_value(const char *value);
struct json_value *json_array_value();
struct json_value *json_object_value();
void json_array_push(struct json_value *array_value, struct json_value *value);
void json_object_push(struct json_value *object_value, const char *name, struct json_value *value);

static int json_get_next_token();
static struct json_object_member *json_parse_object_member();
static struct json_object *json_parse_object();
static struct json_array *json_parse_array();
static struct json_value *json_parse_value();
struct json_parse_result *json_parse(const char *json);

static void json_stringify_init_string();
static void json_stringify_append_character(char character);
static void json_stringify_append_string(const char *string);
static void json_stringify_array(const struct json_array *array);
static void json_stringify_object(const struct json_object *object);
static void json_stringify_value(const struct json_value *value);
char *json_stringify(const struct json_value *value);

static void json_array_free(struct json_array *array);
static void json_object_free(struct json_object *object);
void json_value_free(struct json_value *value);
void json_parse_result_free(struct json_parse_result *parse_result);

/*
 * JSON CONSTRUCTOR
 */

struct json_value *json_null_value() {
    struct json_value *null_value;
    json_malloc(null_value, sizeof(struct json_value));
    null_value->type = JSON_NULL_VALUE;
    return null_value;
}

struct json_value *json_boolean_value(bool value) {
    struct json_value *boolean_value;
    json_malloc(boolean_value, sizeof(struct json_value));
    boolean_value->type = JSON_BOOLEAN_VALUE;
    if (value) {
        boolean_value->boolean_value = "true";
    } else {
        boolean_value->boolean_value = "false";
    }
    return boolean_value;
}

struct json_value *json_string_value(const char *value) {
    struct json_value *string_value;
    json_malloc(string_value, sizeof(struct json_value));
    string_value->type = JSON_STRING_VALUE;
    string_value->string_value = malloc(sizeof(char) * (strlen(value) + 1));
    strcpy(string_value->string_value, value);
    return string_value;
}

struct json_value *json_array_value() {
    struct json_array *array;
    json_malloc(array, sizeof(struct json_array));
    array->size = 0;
    array->values = NULL;

    struct json_value *array_value;
    json_malloc(array_value, sizeof(struct json_value));
    array_value->type = JSON_ARRAY_VALUE;
    array_value->array_value = array;

    return array_value;
}

struct json_value *json_object_value() {
    struct json_object *object;
    json_malloc(object, sizeof(struct json_object));
    object->size = 0;
    object->members = NULL;

    struct json_value *object_value;
    json_malloc(object_value, sizeof(struct json_value));
    object_value->type = JSON_OBJECT_VALUE;
    object_value->object_value = object;

    return object_value;
}

void json_array_push(struct json_value *array_value, struct json_value *value) {
    assert(array_value->type == JSON_ARRAY_VALUE);
    struct json_array *array = array_value->array_value;

    if (array->size == 0) {
        json_malloc(array->values, sizeof(struct json_value *) * JSON_ARRAY_CHUNK_SIZE);
    } else if (array->size % JSON_ARRAY_CHUNK_SIZE == 0) {
        json_realloc(array->values, sizeof(struct json_value *) * (array->size + JSON_ARRAY_CHUNK_SIZE));
    }

    array->values[array->size++] = value;
}

void json_object_push(struct json_value *object_value, const char *name, struct json_value *value) {
    assert(object_value->type == JSON_OBJECT_VALUE);
    struct json_object *object = object_value->object_value;

    struct json_object_member *member;
    json_malloc(member, sizeof(struct json_object_member));
    json_malloc(member->name, sizeof(char) * (strlen(name) + 1));
    strcpy(member->name, name);
    member->value = value;

    if (object->size == 0) {
        json_malloc(object->members, sizeof(struct json_object_member *) * JSON_OBJECT_CHUNK_SIZE);
    } else if (object->size % JSON_OBJECT_CHUNK_SIZE == 0) {
        json_realloc(object->members, sizeof(struct json_object_member *) * (object->size + JSON_OBJECT_CHUNK_SIZE));
    }

    object->members[object->size++] = member;
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

static const char *source;
static size_t source_length;
static unsigned int last_error;
static unsigned int last_error_position;
static unsigned int current_position;
static unsigned int next_token;
static char *token_value = NULL;
static unsigned int token_value_length;

static int json_get_next_token() {
    token_value = NULL;
    token_value_length = 0;

    if (current_position >= source_length) {
        return JSON_TOKEN_EOF;
    }

    char current_character = source[current_position];

    while (json_is_whitespace(current_character)) {
        current_character = source[++current_position];
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
                if (current_character == '"' && source[current_position - 1] != '\\') {
                    json_malloc(token_value, sizeof(char) * (token_value_length + 1));
                    memcpy(token_value, source + current_position - token_value_length, token_value_length);
                    token_value[token_value_length] = '\0';
                    current_position++;
                    return JSON_TOKEN_STRING;
                }
                current_character = source[++current_position];
                token_value_length++;
            }
            break;

        case JSON_TOKEN_FALSE:
            if (source_length - current_position >= 5 && memcmp(source + current_position, "false", 5) == 0) {
                current_position += 5;
                token_value = "false";
                return JSON_TOKEN_FALSE;
            }
            break;

        case JSON_TOKEN_TRUE:
            if (source_length - current_position >= 4 && memcmp(source + current_position, "true", 4) == 0) {
                current_position += 4;
                token_value = "true";
                return JSON_TOKEN_TRUE;
            }
            break;

        case JSON_TOKEN_NULL:
            if (source_length - current_position >= 4 && memcmp(source + current_position, "null", 4) == 0) {
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
            goto invalid_number;
        }

        if (current_character == '.') {
            current_character = source[++current_position];
            token_value_length++;

            if (!json_is_digit(current_character)) {
                goto invalid_number;
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
                goto invalid_number;
            }

            while (json_is_digit(current_character)) {
                current_character = source[++current_position];
                token_value_length++;
            }
        }

        json_malloc(token_value, sizeof(char) * (token_value_length + 1));
        memcpy(token_value, source + current_position - token_value_length, token_value_length);
        token_value[token_value_length] = '\0';
        return JSON_TOKEN_NUMBER;
    }
invalid_number:

    return JSON_TOKEN_UNKNOWN;
}

static struct json_object_member *json_parse_object_member() {
    if (next_token != JSON_TOKEN_STRING) {
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position - token_value_length;
        return NULL;
    }

    char *name = token_value;
    token_value = NULL;
    next_token = json_get_next_token();

    if (next_token != JSON_TOKEN_NAME_SEPARATOR) {
        last_error = JSON_ERROR_UNEXPECTED_TOKEN;
        last_error_position = current_position - token_value_length;
        free(name);
        return NULL;
    }

    next_token = json_get_next_token();
    struct json_value *value = json_parse_value();

    if (value == NULL) {
        free(name);
        return NULL;
    }

    struct json_object_member *member;
    json_malloc(member, sizeof(struct json_object_member));
    member->name = name;
    member->value = value;
    return member;
}

static struct json_object *json_parse_object() {
    struct json_object *object;

    next_token = json_get_next_token();

    json_malloc(object, sizeof(struct json_object));
    object->size = 0;

    if (next_token == JSON_TOKEN_END_OBJECT) {
        next_token = json_get_next_token();
        object->members = NULL;
        return object;
    }

    while (true) {
        struct json_object_member *member = json_parse_object_member();

        if (member == NULL) {
            json_object_free(object);
            return NULL;
        }

        if (object->size == 0) {
            json_malloc(object->members, sizeof(struct json_object_member *) * JSON_OBJECT_CHUNK_SIZE);
        } else if (object->size % JSON_OBJECT_CHUNK_SIZE == 0) {
            json_realloc(object->members, sizeof(struct json_object_member *) * (object->size + JSON_OBJECT_CHUNK_SIZE));
        }

        object->members[object->size++] = member;

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
    next_token = json_get_next_token();

    struct json_array *array;
    json_malloc(array, sizeof(struct json_array));
    array->size = 0;

    if (next_token == JSON_TOKEN_END_ARRAY) {
        next_token = json_get_next_token();
        array->values = NULL;
        return array;
    }

    while (true) {
        struct json_value *value = json_parse_value();

        if (value == NULL) {
            json_array_free(array);
            return NULL;
        }

        if (array->size == 0) {
            json_malloc(array->values, sizeof(struct json_value *) * JSON_ARRAY_CHUNK_SIZE);
        } else if (array->size % JSON_ARRAY_CHUNK_SIZE == 0) {
            json_realloc(array->values, sizeof(struct json_value *) * (array->size + JSON_ARRAY_CHUNK_SIZE));
        }

        array->values[array->size++] = value;

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
        json_malloc(value, sizeof(struct json_value)); \
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
        json_parse_value_case_value(JSON_TOKEN_STRING, JSON_STRING_VALUE, string_value)
        json_parse_value_case_value(JSON_TOKEN_NUMBER, JSON_NUMBER_VALUE, number_value)
        json_parse_value_case_value(JSON_TOKEN_FALSE, JSON_BOOLEAN_VALUE, boolean_value)
        json_parse_value_case_value(JSON_TOKEN_TRUE, JSON_BOOLEAN_VALUE, boolean_value)

        case JSON_TOKEN_NULL:
            json_malloc(value, sizeof(struct json_value));
            value->type = JSON_NULL_VALUE;
            token_value = NULL;
            next_token = json_get_next_token();
            return value;

        case JSON_TOKEN_BEGIN_ARRAY:
            array = json_parse_array();
            if (array == NULL) {
                return NULL;
            }
            json_malloc(value, sizeof(struct json_value));
            value->type = JSON_ARRAY_VALUE;
            value->array_value = array;
            return value;

        case JSON_TOKEN_BEGIN_OBJECT:
            object = json_parse_object();
            if (object == NULL) {
                return NULL;
            }
            json_malloc(value, sizeof(struct json_value));
            value->type = JSON_OBJECT_VALUE;
            value->object_value = object;
            return value;

        default:
            last_error = JSON_ERROR_UNEXPECTED_TOKEN;
            last_error_position = current_position;
            return NULL;
    }
}

struct json_parse_result *json_parse(const char *json) {
    struct json_parse_result *result;
    json_malloc(result, sizeof(struct json_parse_result));

    source = json;
    source_length = strlen(json);

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

    result->value = json_parse_value();

    if (last_error > 0) {
        result->error = last_error;
        result->error_position = last_error_position;

        if (token_value != NULL) {
            free(token_value);
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

static char *stringify_result = NULL;
static size_t stringify_result_length;
static size_t result_allocated_chunks;

static void json_stringify_init_string() {
    json_malloc(stringify_result, sizeof(char) * (JSON_STRINGIFY_CHUNK_SIZE + 1));
    stringify_result[0] = '\0';
    stringify_result_length = 0;
    result_allocated_chunks = 1;
}

static void json_stringify_append_character(char character) {
    if (stringify_result_length > 0 && stringify_result_length % JSON_STRINGIFY_CHUNK_SIZE == 0) {
        json_realloc(stringify_result, sizeof(char) * (stringify_result_length + JSON_STRINGIFY_CHUNK_SIZE + 1));
        result_allocated_chunks++;
    }

    stringify_result[stringify_result_length++] = character;
    stringify_result[stringify_result_length] = '\0';
}

static void json_stringify_append_string(const char *string) {
    size_t string_length = strlen(string);
    size_t allocated_length = result_allocated_chunks * JSON_STRINGIFY_CHUNK_SIZE;
    size_t new_string_length = stringify_result_length + string_length;

    if (new_string_length > allocated_length) {
        size_t new_string_chunks = new_string_length / JSON_STRINGIFY_CHUNK_SIZE + 1;
        json_realloc(stringify_result, sizeof(char) * (JSON_STRINGIFY_CHUNK_SIZE * new_string_chunks));
        result_allocated_chunks = new_string_chunks;
    }

    memcpy(stringify_result + stringify_result_length, string, string_length);
    stringify_result_length = new_string_length;
    stringify_result[new_string_length] = '\0';
}

static void json_stringify_array(const struct json_array *array) {
    json_stringify_append_character('[');

    for (int i = 0; i < array->size; i++) {
        if (i != 0) {
            json_stringify_append_character(',');
        }

        json_stringify_value(array->values[i]);
    }

    json_stringify_append_character(']');
}

static void json_stringify_object(const struct json_object *object) {
    json_stringify_append_character('{');

    for (int i = 0; i < object->size; i++) {
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
            json_stringify_append_string(value->boolean_value);
            break;

        case JSON_NUMBER_VALUE:
            json_stringify_append_string(value->number_value);
            break;

        case JSON_STRING_VALUE:
            json_stringify_append_character('"');
            json_stringify_append_string(value->string_value);
            json_stringify_append_character('"');
            break;

        case JSON_ARRAY_VALUE:
            json_stringify_array(value->array_value);
            break;

        case JSON_OBJECT_VALUE:
            json_stringify_object(value->object_value);
            break;
    }
}

char *json_stringify(const struct json_value *value) {
    json_stringify_init_string();

    json_stringify_value(value);

    char *result = stringify_result;
    stringify_result = NULL;

    return result;
}

/*
 * FREE FUNCTIONS
 */

static void json_array_free(struct json_array *array) {
    if (array->size > 0) {
        for (int i = 0; i < array->size; i++) {
            json_value_free(array->values[i]);
        }

        free(array->values);
    }

    free(array);
}

static void json_object_free(struct json_object *object) {
    if (object->size > 0) {
        for (int i = 0; i < object->size; i++) {
            free(object->members[i]->name);
            json_value_free(object->members[i]->value);
            free(object->members[i]);
        }

        free(object->members);
    }

    free(object);
}

void json_value_free(struct json_value *value) {
    switch (value->type) {
        case JSON_NULL_VALUE:
        case JSON_BOOLEAN_VALUE:
            break;

        case JSON_NUMBER_VALUE:
            free(value->number_value);
            break;

        case JSON_STRING_VALUE:
            free(value->string_value);
            break;

        case JSON_ARRAY_VALUE:
            json_array_free(value->array_value);
            break;

        case JSON_OBJECT_VALUE:
            json_object_free(value->object_value);
            break;
    }

    free(value);
}

void json_parse_result_free(struct json_parse_result *parse_result) {
    free(parse_result);
}
