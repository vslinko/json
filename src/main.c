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

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "json.h"
#include "json_template.h"

#define SOURCE_CHUNK_SIZE 512

const char* error_messages[] = {
    "JSON_ERROR_EMPTY_FILE",
    "JSON_ERROR_UNEXPECTED_TOKEN"
};

char* read_file(const char* path);
char* read_file(const char* path) {
    FILE* handle = fopen(path, "r");

    if (handle == NULL) {
        return NULL;
    }

    char* source = malloc(sizeof(char) * SOURCE_CHUNK_SIZE + 1);
    assert(source);

    int length = 0;

    source[length] = '\0';

    while (true) {
        char character = getc(handle);

        if (character == EOF) {
            break;
        }

        if (length > 0 && length % SOURCE_CHUNK_SIZE == 0) {
            source = realloc(source, sizeof(char) * (length + SOURCE_CHUNK_SIZE + 1));
            assert(source);
        }

        source[length++] = character;
        source[length] = '\0';
    }

    fclose(handle);

    return source;
}

struct json_value* parse_file(const char* path);
struct json_value* parse_file(const char* path) {
    char* source = read_file(path);

    if (source == NULL) {
        fprintf(stderr, "Unable to read file \"%s\"\n", path);
        exit(2);
    }

    struct json_parse_result* result = json_parse(source);

    if (result->error > 0) {
        fprintf(
            stderr,
            "ERROR #%d: %s at position %d at file %s\n",
            result->error,
            error_messages[result->error - 1],
            result->error_position,
            path
        );
        exit(3);
    }

    struct json_value* value = result->value;
    json_parse_result_free(result);

    return value;
}

int main(int argc, const char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: json <template_file> <json_file>...\n");
        return 1;
    }

    struct json_value* json_template = parse_file(argv[1]);
    struct json_value** values = NULL;
    size_t size = 0;

    for (int i = 2; i < argc; i++) {
        struct json_value* value = parse_file(argv[i]);

        if (size == 0) {
            values = malloc(sizeof(struct json_value*));
        } else {
            values = realloc(values, sizeof(struct json_value*) * (size + 1));
        }
        assert(values);

        values[size++] = value;
    }

    struct json_value* combined = json_combine_array(values, size);
    struct json_value* compiled = json_compile(json_template, combined);

    if (compiled == NULL) {
        fprintf(stderr, "Unable to compile template\n");
        exit(4);
    }

    puts(json_stringify(compiled));

    return 0;
}
