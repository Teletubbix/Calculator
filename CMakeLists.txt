cmake_minimum_required(VERSION 3.10)
project(Calculator VERSION 1.6.0)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# 重要：GUI 程序设置 （避免弹出控制台黑框）
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -mwindows")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

include_directories(include)

file(GLOB MATH_SRCS "src/math_*.c")
add_library(math SHARED ${MATH_SRCS})
target_link_libraries(math m)

add_executable(calculator 
    src/main.c 
    src/print_error.c 
    src/print_output.c
    src/expr_eval.c
    src/linenoise.c
)

# 链接 Windows GUI 所需的系统库
target_link_libraries(calculator math user32 gdi32 comctl32)

set_target_properties(calculator PROPERTIES OUTPUT_NAME "CalculatorGUI")