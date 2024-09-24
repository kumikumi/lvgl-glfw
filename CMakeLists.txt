cmake_minimum_required(VERSION 3.10)
project(lvgl_glfw_example)

set(CMAKE_C_STANDARD 11)

# Add LVGL configuration
add_definitions(-DLV_CONF_INCLUDE_SIMPLE)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/lvgl)  # Add this line

# Add LVGL
add_subdirectory(lvgl)

# Find GLFW
find_package(glfw3 REQUIRED)

# Find OpenGL
find_package(OpenGL REQUIRED)

# Your source file
add_executable(${PROJECT_NAME} src/main.c)

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    lvgl
    glfw
    OpenGL::GL
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/lvgl
)