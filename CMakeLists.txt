cmake_minimum_required(VERSION 3.16)
project(CASEFIRE LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(OpenAL REQUIRED)
find_package(glad CONFIG QUIET)

add_executable(casefire
    src/main.cpp
    src/Game.cpp
    src/Shop.cpp
    src/AdminPanel.cpp
)

target_include_directories(casefire PRIVATE src ${OPENAL_INCLUDE_DIR})

target_link_libraries(casefire PRIVATE
    OpenGL::GL
    glfw
    glm::glm
    nlohmann_json::nlohmann_json
)

if (TARGET OpenAL::OpenAL)
    target_link_libraries(casefire PRIVATE OpenAL::OpenAL)
else()
    target_link_libraries(casefire PRIVATE ${OPENAL_LIBRARY})
endif()

if (TARGET glad::glad)
    target_link_libraries(casefire PRIVATE glad::glad)
elseif (TARGET glad)
    target_link_libraries(casefire PRIVATE glad)
else()
    find_library(GLAD_LIBRARY glad)
    find_path(GLAD_INCLUDE_DIR NAMES glad/glad.h glad/gl.h)
    if (GLAD_LIBRARY AND GLAD_INCLUDE_DIR)
        target_include_directories(casefire PRIVATE ${GLAD_INCLUDE_DIR})
        target_link_libraries(casefire PRIVATE ${GLAD_LIBRARY})
    else()
        message(FATAL_ERROR "glad was not found. On Arch/CachyOS install it with: sudo pacman -S glad")
    endif()
endif()

add_custom_command(TARGET casefire POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_SOURCE_DIR}/shaders
            $<TARGET_FILE_DIR:casefire>/shaders)
