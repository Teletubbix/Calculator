cmake_minimum_required(VERSION 3.10)
project(Calculator VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# ---------- 关键：设置输出目录（必须在创建目标之前） ----------
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(include)

file(GLOB MATH_SRCS "src/math_*.c")
add_library(math SHARED ${MATH_SRCS})

add_executable(calculator src/main.c src/print_error.c src/print_output.c)
target_link_libraries(calculator math)

set_target_properties(calculator PROPERTIES OUTPUT_NAME "Calculator")