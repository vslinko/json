cmake_minimum_required(VERSION 3.13)
include_directories(src)
include_directories(vendor/vstd/src)
add_executable(
  json
  src/main.c
  src/json.c
  src/json_search.c
  src/json_template.c
)
target_compile_options(
  json
  PRIVATE -O2
)
target_compile_options(
  json
  PRIVATE -Wall
  PRIVATE -Wextra
  PRIVATE -std=c11
  PRIVATE -pedantic
  PRIVATE -Wmissing-prototypes
  PRIVATE -Wstrict-prototypes
  PRIVATE -Wold-style-definition
)
add_executable(
  json-test
  vendor/vstd/src/vstd/test_runner.c
  vendor/vstd/src/vstd/test.c
  src/_json_test.c
  src/json.c
  src/_json_search_test.c
  src/json_search.c
  src/_json_template_test.c
  src/json_template.c
)
target_compile_options(
  json-test
  PRIVATE -g
  PRIVATE -O0
)
target_compile_options(
  json-test
  PRIVATE -Wall
  PRIVATE -Wextra
  PRIVATE -std=c11
  PRIVATE -pedantic
  PRIVATE -Wmissing-prototypes
  PRIVATE -Wstrict-prototypes
  PRIVATE -Wold-style-definition
)
