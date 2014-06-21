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

#include <stdlib.h>
#include <string.h>
#include "json_template.h"

struct json_value *json_combine(size_t arguments_length, ...) {
    va_list arguments;
    va_start(arguments, arguments_length);

    struct json_value *array = json_array_value();
    struct json_value *object = json_object_value();
    json_object_push(object, "srcs", array);

    for (int i = 0; i < arguments_length; i++) {
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

    for (int i = 0; i < size; i++) {
        json_array_push(array, values[i]);
    }

    return object;
}
