#include "./vendor/json-parser/json.h"

#include <vstd/test.h>


#define PARSE_BENCHMARK_TRIES 1000000
static char sources[PARSE_BENCHMARK_TRIES][100];
static json_value** links;

vstd_test_benchmark(json_parser_parse_benchmark, 1.0, {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        sprintf(sources[i], "{\"array\":[null,true,false,-1.0e-2,\"string\",%f]}", ((double) rand()) / RAND_MAX);
    }

    links = malloc(sizeof(json_value*) * PARSE_BENCHMARK_TRIES);
}, {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value* result = json_parse(sources[i], strlen(sources[i]));
        links[i] = result;
    }
}, {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value_free(links[i]);
    }
    free(links);
})

int main(int argc, char** argv) {
    vstd_test_runner(argc, argv);
    return 0;
}
