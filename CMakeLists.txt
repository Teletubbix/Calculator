cmake_minimum_required(VERSION 3.16)
project(Calculator VERSION 3.1.0 LANGUAGES C)

# —— 编译标准 ——
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# 生成 compile_commands.json，方便 clangd / 编辑器代码跳转
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 可执行文件统一输出到 bin/（bin 已被 .gitignore 忽略）
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

# —— 通用编译告警 ——
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

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

# —— 单元测试 ——
option(BUILD_TESTING "Build and register unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(calculator_tests tests/test_calculator.c)
    target_link_libraries(calculator_tests PRIVATE calc_core)
    add_test(NAME calculator_tests COMMAND calculator_tests)
endif()

# —— 安装规则 ——
include(GNUInstallDirs)
install(TARGETS calc_core
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})
# GUI 目标(Calculator)只在对应平台分支上存在；不存在时跳过安装，避免配置失败
if(TARGET Calculator)
    install(TARGETS Calculator
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
