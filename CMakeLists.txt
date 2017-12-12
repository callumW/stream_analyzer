cmake_minimum_required(VERSION 2.8)
project(stream_analyzer C)

file(GLOB SRC_FILES "src/*.c")
file(GLOB_RECURSE LIBUCSI_SRC "libucsi/*.c")

set(CURSES_NEED_NCURSES TRUE)
find_package(Curses REQUIRED)

if(CURSES_FOUND)
    message("Found Curses")
    if(CURSES_HAVE_NCURSES_H)
        message("Found NCurses too!")
    endif(CURSES_HAVE_NCURSES_H)
else()
    message(FATAL_ERROR "Curses not found")
endif(CURSES_FOUND)

add_executable(stream_analyzer ${SRC_FILES} ${LIBUCSI_SRC})
include_directories("include" ${CURSES_INCLUDE_DIRS})
target_link_libraries(stream_analyzer ${CURSES_LIBRARIES})
