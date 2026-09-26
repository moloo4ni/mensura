cmake_minimum_required(VERSION 3.19)

project(mensura VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Test LinguistTools)
find_package(Fooyin REQUIRED)

set(MENSURA_PLUGIN_DIR "$ENV{HOME}/.local/lib/fooyin/plugins"
    CACHE PATH "Directory the plugin is installed into")

include(CTest)

add_subdirectory(src)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
