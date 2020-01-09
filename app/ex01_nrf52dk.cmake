include(${PROJECT_UBINOS_DIR}/config/ubinos_nrf52dk.cmake)

set(INCLUDE__APP TRUE)
set(APP__NAME "ex01")

get_filename_component(_tmpdir "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

file(GLOB_RECURSE _tmpsrc
    "${_tmpdir}/*.c"
    "${_tmpdir}/*.cpp"
    "${_tmpdir}/*.S"
    "${_tmpdir}/*.s")

set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmpsrc})

