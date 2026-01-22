cmake_minimum_required(VERSION 3.10)
project(LaborHeightProfile C)

set(CMAKE_C_STANDARD 11)

# Find SQLite3
find_package(SQLite3 REQUIRED)

# Debug: Überprüfen, was gefunden wurde
message(STATUS "SQLite3_FOUND: ${SQLite3_FOUND}")
message(STATUS "SQLite3_INCLUDE_DIRS: ${SQLite3_INCLUDE_DIRS}")
message(STATUS "SQLite3_LIBRARIES: ${SQLite3_LIBRARIES}")

add_executable(LaborTask1 
    src/main.c 
    src/sensor.c 
)

target_include_directories(LaborTask1 PRIVATE ${SQLite3_INCLUDE_DIRS})
target_link_libraries(LaborTask1 PRIVATE ${SQLite3_LIBRARIES})