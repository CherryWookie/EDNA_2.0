cmake_minimum_required(VERSION 3.16)

# Register all custom components so IDF can find them
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_SOURCE_DIR}/components"
    # "${CMAKE_SOURCE_DIR}/Libraries"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(FlightControl)
