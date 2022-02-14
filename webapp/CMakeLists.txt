cmake_minimum_required(VERSION 3.12)
project (lvgl)

set(CMAKE_C_STANDARD 11)#C11
set(CMAKE_CXX_STANDARD 17)#C17
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2")

include_directories(${PROJECT_SOURCE_DIR})

file(GLOB_RECURSE INCLUDES "./*.h" "../common/src/*.h")
file(GLOB_RECURSE LIB_SOURCES "lvgl/*.c" "lv_examples/*.c" "lv_drivers/*.c")
file(GLOB_RECURSE MY_SOURCES "../common/src/*.c" "../common/src/*.cpp")
set(SOURCES ${LIB_SOURCES} ${MY_SOURCES})

add_executable(index main.cpp ${SOURCES} ${INCLUDES})

if(NOT LVGL_CHOSEN_DEMO)
    set(LVGL_CHOSEN_DEMO lv_demo_widgets)
endif()
set_source_files_properties(main.cpp PROPERTIES COMPILE_FLAGS -DCHOSEN_DEMO=${LVGL_CHOSEN_DEMO})

set(CMAKE_EXECUTABLE_SUFFIX ".html")
set_target_properties(index PROPERTIES LINK_FLAGS "--shell-file ${PROJECT_SOURCE_DIR}/lvgl_shell.html -s SINGLE_FILE=1 -s LLD_REPORT_UNDEFINED")
