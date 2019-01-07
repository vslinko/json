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
#include "json_search.h"

#define SOURCE_CHUNK_SIZE 512

const char* error_messages[] = {
    "JSON_ERROR_EMPTY_FILE",
    "JSON_ERROR_UNEXPECTED_TOKEN"
};

unsigned int read_file(const char* path, char** _source);
unsigned int read_file(const char* path, char** _source) {
    FILE* handle = fopen(path, "r");

    if (handle == NULL) {
        return -1;
    }
    
    char* source = malloc(sizeof(char) * SOURCE_CHUNK_SIZE + 1);
    assert(source);

    unsigned int length = 0;

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

    *_source = source;

    return length;
}

struct json_value* parse_file(const char* path);
struct json_value* parse_file(const char* path) {
    char* source = NULL;
    unsigned int length = read_file(path, &source);

    if (length < 0) {
        fprintf(stderr, "Unable to read file \"%s\"\n", path);
        exit(2);
    }

    struct json_parse_result* result = json_parse(source, length);

    if (result->error > 0) {
        fprintf(
            stderr,
            "ERROR #%d: %s at position %d at file %s\n",
            result->error,
            error_messages[result->error - 1],
            result->error_position,
            path
        );
        exit(1);
    }

    struct json_value* value = result->value;
    json_parse_result_free(result);

    return value;
}

int main(int argc, const char** argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: json <json_file> [search_path]\n");
        return 2;
    }

    struct json_value* json_value = parse_file(argv[1]);

    if (argc >= 3) {
        json_value = json_search(json_value, argv[2]);

        if (!json_value) {
            fprintf(stderr, "Unable to find %s\n", argv[2]);
            return 2;
        }
    }

    puts(json_stringify(json_value));

    return 0;
}
