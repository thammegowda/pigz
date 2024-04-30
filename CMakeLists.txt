cmake_minimum_required(VERSION 3.20 FATAL_ERROR)
project(pigz CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -Wno-unknown-pragmas -Wcast-qual")
set(EXT_LIBS "")

#find zlib and add it to EXT_LIBS
find_package(ZLIB REQUIRED)
list(APPEND EXT_LIBS ${ZLIB_LIBRARIES})

# find pthread and add it to EXT_LIBS
find_package(Threads REQUIRED)
list(APPEND EXT_LIBS Threads::Threads)

# find libm and add it to EXT_LIBS
find_library(MATH_LIBRARY m)
if(MATH_LIBRARY)
    list(APPEND EXT_LIBS ${MATH_LIBRARY})
endif()

################
set(ZOPFLI "${CMAKE_SOURCE_DIR}/zopfli/src/zopfli")
include_directories(
    "${CMAKE_SOURCE_DIR}"
    "${ZOPFLI}"
  )

set(PIGZ_SOURCES
  pigz.c
  try.c
  yarn.c
  ${ZOPFLI}/blocksplitter.c
  ${ZOPFLI}/cache.c
  ${ZOPFLI}/deflate.c
  ${ZOPFLI}/hash.c
  ${ZOPFLI}/katajainen.c
  ${ZOPFLI}/lz77.c
  ${ZOPFLI}/squeeze.c
  ${ZOPFLI}/symbols.c
  ${ZOPFLI}/tree.c
  ${ZOPFLI}/util.c
)

# produce executable pigz
add_executable(pigz ${PIGZ_SOURCES})
# link the external libraries
target_link_libraries(pigz ${EXT_LIBS})

########