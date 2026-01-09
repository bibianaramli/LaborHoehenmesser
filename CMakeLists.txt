cmake_minimum_required(VERSION 3.10)
project(MyCProject)
find_package(SQLite3 REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_BUILD_TYPE Debug)

add_executable(MyCProject-ex src/main.c)

add_executable(sql-ex sql_main.c)
target_link_libraries(sql-ex SQLite::SQLite3)
add_executable(serial_reader-example src/serial_reader-example.c)
