cmake_minimum_required(VERSION 2.8.4)
project(ssd)

add_custom_target(ssd COMMAND make all
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)