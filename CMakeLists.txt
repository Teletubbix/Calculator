cmake_minimum_required(VERSION 3.10)
project(Calculator VERSION 2.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 可执行文件统一输出到 bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# 表达式求值核心库（静态库，方便测试和复用）
add_library(calc_core STATIC
    src/calculator.c
)
target_include_directories(calc_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(calc_core PUBLIC m)
endif()

# 主程序
add_executable(Calculator src/main.c)
target_link_libraries(Calculator PRIVATE calc_core)

# 单元测试
enable_testing()
add_executable(calculator_tests tests/test_calculator.c)
target_link_libraries(calculator_tests PRIVATE calc_core)
add_test(NAME calculator_tests COMMAND calculator_tests)
