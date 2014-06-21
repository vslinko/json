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

#include <string.h> /* size_t, strlen */
#include <stdlib.h> /* atoi, abort */
#include <stdio.h> /* fprintf */
#include "json_search.h"

/*
 * TABLE OF CONTENTS
 */

static char json_search_get_next_token();
struct json_value *json_search(struct json_value *root, const char *path);

/*
 * JSON SEARCH
 */

#define JSON_SEARCH_TOKEN_EOF 0
#define JSON_SEARCH_TOKEN_PROPERTY 1
#define JSON_SEARCH_TOKEN_ARRAY 2
#define JSON_SEARCH_TOKEN_UNKNOWN 3

#define json_search_is_digit(character) \
    (character >= '0' && character <= '9')

static const char *search_path;
static size_t search_path_length;
static unsigned int current_position;
static char* property_name;
static unsigned int array_index;

static char json_search_get_next_token() {
    if (current_position >= search_path_length) {
        return JSON_SEARCH_TOKEN_EOF;
    }

    if (search_path[current_position] == '[') {
        current_position++;

        int array_index_length = 0;

        while (current_position < search_path_length) {
            if (json_search_is_digit(search_path[current_position])) {
                current_position++;
                array_index_length++;
            } else {
                break;
            }
        }

        if (array_index_length == 0 || search_path[current_position] != ']') {
            return JSON_SEARCH_TOKEN_UNKNOWN;
        }

        current_position++;

        char *array_index_string = malloc(sizeof(char) * (array_index_length + 1));
        memcpy(array_index_string, search_path + current_position - array_index_length - 1, array_index_length);
        array_index = atoi(array_index_string);
        free(array_index_string);
        return JSON_SEARCH_TOKEN_ARRAY;
    } else if (current_position == 0 || search_path[current_position] == '.') {
        if (search_path[current_position] == '.') {
            if (current_position == 0) {
                return JSON_SEARCH_TOKEN_UNKNOWN;
            }
            current_position++;
        }

        int property_name_length = 0;

        while (current_position < search_path_length) {
            if (search_path[current_position] != '.' && search_path[current_position] != '[') {
                current_position++;
                property_name_length++;
            } else {
                break;
            }
        }

        if (property_name_length == 0) {
            return JSON_SEARCH_TOKEN_UNKNOWN;
        }

        property_name = malloc(sizeof(char) * (property_name_length + 1));
        memcpy(property_name, search_path + current_position - property_name_length, property_name_length);
        return JSON_SEARCH_TOKEN_PROPERTY;
    }

    return JSON_SEARCH_TOKEN_UNKNOWN;
}

struct json_value *json_search(struct json_value *root, const char *path) {
    search_path = path;
    search_path_length = strlen(search_path);
    current_position = 0;

    while (true) {
        char token = json_search_get_next_token();

        switch (token) {
            case JSON_SEARCH_TOKEN_EOF:
                return root;

            case JSON_SEARCH_TOKEN_PROPERTY:
                if (root->type != JSON_OBJECT_VALUE) {
                    free(property_name);
                    return NULL;
                }

                struct json_value *new_root = NULL;

                for (int i = 0; i < root->object_value->size; i++) {
                    if (strcmp(root->object_value->members[i]->name, property_name) == 0) {
                        new_root = root->object_value->members[i]->value;
                    }
                }

                if (new_root == NULL) {
                    free(property_name);
                    return NULL;
                }

                root = new_root;
                free(property_name);
                break;

            case JSON_SEARCH_TOKEN_ARRAY:
                if (root->type != JSON_ARRAY_VALUE || root->array_value->size <= array_index) {
                    return NULL;
                }

                root = root->array_value->values[array_index];
                break;

            case JSON_SEARCH_TOKEN_UNKNOWN:
                fprintf(stderr, "Invalid JSON search path: %s\n", path);
                abort();
        }
    }

    return NULL;
}
