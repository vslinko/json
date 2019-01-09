cmake_minimum_required(VERSION 3.13)

include_directories(src)
include_directories(vendor/vstd/src)

find_library(
  VSTD_LIB
  NAMES vstd libvstd
  HINTS ${CMAKE_SOURCE_DIR}/vendor/vstd
  NO_DEFAULT_PATH
)

add_executable(
  json
  src/main.c
  src/json.c
  src/json_search.c
)
target_link_libraries(
  json
  ${VSTD_LIB}
)
target_compile_options(
  json
  PRIVATE -O2
)
target_compile_options(
  json
  PRIVATE -Wall
  PRIVATE -Wextra
  PRIVATE -std=c89
  PRIVATE -pedantic
  PRIVATE -Wmissing-prototypes
  PRIVATE -Wstrict-prototypes
  PRIVATE -Wold-style-definition
)

add_executable(
  json-test
  src/main_test.c
  src/_json_test.c
  src/json.c
  src/_json_search_test.c
  src/json_search.c
)
target_link_libraries(
  json-test
  ${VSTD_LIB}
)
target_compile_options(
  json-test
  PRIVATE -O2
)
target_compile_options(
  json-test
  PRIVATE -Wall
  PRIVATE -Wextra
  PRIVATE -std=c89
  PRIVATE -pedantic
  PRIVATE -Wmissing-prototypes
  PRIVATE -Wstrict-prototypes
  PRIVATE -Wold-style-definition
)

add_executable(
  json-parser-benchmark
  json_parser_benchmark.c
  vendor/json-parser/json.c
)
target_link_libraries(
  json-parser-benchmark
  ${VSTD_LIB}
)
target_compile_options(
  json-parser-benchmark
  PRIVATE -O2
)
