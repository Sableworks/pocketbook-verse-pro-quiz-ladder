cmake_minimum_required(VERSION 3.21)

#   cmake -DCMAKE_TOOLCHAIN_FILE=/SDK/share/cmake/arm_conf.cmake ..

project(quizladder C)

if(NOT CMAKE_CROSSCOMPILING)
  message(WARNING "PocketBook toolchain missing. "
                  "Use: -DCMAKE_TOOLCHAIN_FILE=/SDK/share/cmake/arm_conf.cmake")
endif()

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR})

if(DEFINED PB_INCLUDE_DIRECTORIES)
  include_directories(${PB_INCLUDE_DIRECTORIES})
endif()
if(DEFINED PB_LINK_DIRECTORIES)
  link_directories(${PB_LINK_DIRECTORIES})
endif()

if(DEFINED FREETYPE_INCLUDE_DIRS)
  include_directories(${FREETYPE_INCLUDE_DIRS})
endif()

set(_PB_SYSROOT "/SDK/usr/arm-obreey-linux-gnueabi/sysroot")
foreach(_inc
    "${_PB_SYSROOT}/usr/include"
    "${_PB_SYSROOT}/usr/include/freetype2"
    "/SDK/usr/include"
    "/SDK/usr/include/freetype2")
  if(EXISTS "${_inc}")
    include_directories("${_inc}")
  endif()
endforeach()

include_directories(${CMAKE_SOURCE_DIR})

add_executable(quizladder.app
  main.c
  game.c
  i18n.c
  questions.c
)

target_link_libraries(quizladder.app
  inkview
  freetype
  pthread
  m
  dl
  z
)

if(EXISTS "${CMAKE_SOURCE_DIR}/quizladder.png")
  add_custom_command(TARGET quizladder.app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
      "${CMAKE_SOURCE_DIR}/quizladder.png"
      "${CMAKE_BINARY_DIR}/quizladder.png")
endif()
