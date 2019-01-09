#include <assert.h>
#include <string.h>
#include <vstd/test.h>
#include "./vendor/json-parser/json.h"

#define PARSE_BENCHMARK_TRIES 1000000
static char sources[PARSE_BENCHMARK_TRIES][100];
static json_value **links;

static void benchmark_json_parse_setup() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        sprintf(sources[i], "{\"array\":[null,true,false,-1.0e-2,\"string\",%f]}", ((double) rand()) / RAND_MAX);
    }

    links = malloc(sizeof(json_value *) * PARSE_BENCHMARK_TRIES);
    assert(links);
}

static void benchmark_json_parse_teardown() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value_free(links[i]);
    }
    free(links);
}

static void benchmark_json_parse() {
    for (int i = 0; i < PARSE_BENCHMARK_TRIES; i++) {
        json_value *result = json_parse(sources[i], strlen(sources[i]));
        links[i] = result;
    }
}
VSTD_TEST_REGISTER_BENCHMARK(benchmark_json_parse,
                             1.0,
                             benchmark_json_parse_setup,
                             benchmark_json_parse_teardown)

int main(int argc, char **argv) {
    vstd_test_runner(argc, argv);
    return 0;
}
