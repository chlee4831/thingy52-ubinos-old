cmake_minimum_required(VERSION 3.3)

include("${CMAKE_CURRENT_LIST_DIR}/../library/ubinos/config/common.cmake")

project(thingy52)
project_begin()
project_add_library(ubinos)
project_add_library(seggerrtt_wrapper)
project_add_library(nrf5sdk_wrapper)
project_end()

