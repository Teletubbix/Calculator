cmake_minimum_required(VERSION 3.16)
project(Calculator VERSION 6.0.1 LANGUAGES C)

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

# —— 各算法独立共享库（分布式架构，互不依赖）——
add_library(calc_core SHARED src/calculator.c)
target_include_directories(calc_core PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(calc_core PUBLIC m)
endif()

add_library(calc_units SHARED src/units.c)
target_include_directories(calc_units PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(calc_units PUBLIC m)
endif()

add_library(calc_matrix SHARED src/matrix.c)
target_include_directories(calc_matrix PUBLIC ${CMAKE_SOURCE_DIR}/include)

add_library(calc_complex SHARED src/complex.c)
target_include_directories(calc_complex PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(calc_complex PUBLIC m)
endif()

add_library(calc_db SHARED src/db.c)
target_include_directories(calc_db PUBLIC ${CMAKE_SOURCE_DIR}/include)
if(NOT WIN32)
    target_link_libraries(calc_db PUBLIC m)
endif()

# 主程序
add_executable(Calculator src/main.c)
target_link_libraries(Calculator PRIVATE calc_core calc_units calc_matrix calc_complex calc_db)

# —— GTK4 图形界面（可选：找到 gtk4 时构建）——
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(GTK4 QUIET gtk4)
endif()
if(GTK4_FOUND)
    add_executable(Calculator-gui gui/gui_gtk4.c)
    target_link_libraries(Calculator-gui PRIVATE calc_core calc_matrix calc_complex calc_db ${GTK4_LIBRARIES})
    target_include_directories(Calculator-gui PRIVATE ${GTK4_INCLUDE_DIRS})
    message(STATUS "找到 GTK4，构建 GUI 目标 Calculator-gui")
else()
    message(STATUS "未找到 GTK4，跳过 GUI 目标（需要 libgtk-4-dev）")
endif()

# —— 单元测试 ——
option(BUILD_TESTING "Build and register unit tests" ON)
if(BUILD_TESTING)
    enable_testing()
    add_executable(calculator_tests tests/test_calculator.c)
    target_link_libraries(calculator_tests PRIVATE calc_core calc_units calc_matrix calc_complex calc_db)
    add_test(NAME calculator_tests COMMAND calculator_tests)
endif()

# —— 安装规则 ——
include(GNUInstallDirs)
foreach(t calc_core calc_units calc_matrix calc_complex calc_db)
    install(TARGETS ${t}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
endforeach()
if(TARGET Calculator)
    install(TARGETS Calculator
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
