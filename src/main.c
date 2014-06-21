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

#define SOURCE_CHUNK_SIZE 512

const char *error_messages[] = {
    "JSON_ERROR_EMPTY_FILE",
    "JSON_ERROR_UNEXPECTED_TOKEN"
};

char *read_file(const char *path) {
    FILE *handle = fopen(path, "r");

    if (handle == NULL) {
        return NULL;
    }

    char *source = malloc(sizeof(char) * SOURCE_CHUNK_SIZE + 1);
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

void parse_source(const char *source) {
    struct json_parse_result *result = json_parse(source);

    if (result->error > 0) {
        fprintf(stderr, "ERROR #%d: %s at position %d\n", result->error, error_messages[result->error - 1], result->error_position);
        exit(3);
    }

    char *processed_source = json_stringify(result->value);

    puts(processed_source);

    if (result->error == 0) {
        json_value_free(result->value);
    }
    json_parse_result_free(result);
    free(processed_source);
}

void parse_file(const char *path) {
    char *source = read_file(path);

    if (source == NULL) {
        fprintf(stderr, "Unable to read file \"%s\"\n", path);
        exit(2);
    }

    parse_source(source);

    free(source);
}

int main(int argc, const char ** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s JSON_FILE\n", argv[0]);
        exit(1);
    }

    parse_file(argv[1]);

    return 0;
}
