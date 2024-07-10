cmake_minimum_required(VERSION 3.11 FATAL_ERROR)
project(pigz CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(PYBIND "Build the python bindings" ON)
# TODO: remove version specific compiler; I got internal error with gcc-11, so I switched to gcc-9
set(CMAKE_C_COMPILER gcc)
set(EXT_LIBS "")
set(CMAKE_C_FLAGS "-O3 -Wall -Wextra -Wno-unknown-pragmas -Wcast-qual")
set(CMAKE_POSITION_INDEPENDENT_CODE ON)   #https://stackoverflow.com/a/38297422/1506477
#set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#option(USE_CCACHE "Use ccache compiler cache (https://ccache.dev)" ON)
set(CMAKE_BUILD_TYPE Release)

#if(USE_CCACHE)
#  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -C")
#endif()
find_package(Git QUIET)

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
set(ZOPFLI "zopfli/src/zopfli")
include_directories(.)
include_directories(${ZOPFLI})

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
add_executable(pigz_o pigz_main.c ${PIGZ_SOURCES})   #_exe because we also want library pigz
set_property(TARGET pigz_o PROPERTY OUTPUT_NAME pigz)
# create it in the source directory
set_target_properties(pigz_o PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(pigz_o ${EXT_LIBS})
# copy pigz as upigz
add_custom_command(TARGET pigz_o POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:pigz_o> ${CMAKE_CURRENT_SOURCE_DIR}/unpigz
)

# build the static library for pigz
add_library(pigz STATIC ${PIGZ_SOURCES})
target_link_libraries(pigz PUBLIC ${EXT_LIBS})

#add_executable(tmpbin tmp.cpp ${PIGZ_SOURCES})   #_exe because we also want library pigz
#target_link_libraries(tmpbin ${EXT_LIBS})

######## PYBIND
if(PYBIND)

  # update the submodules if they are not already there
  execute_process(COMMAND git submodule update --init --recursive --no-fetch
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

  include_directories(pybind11/include)
  add_subdirectory(pybind11)
  include(PythonModules)

  add_custom_target(pigz_py)
  foreach(PYTHON_VERSION ${PYTHON_VERSIONS})
      py_add_module(pigz_py_${PYTHON_VERSION} pybind.cpp PYTHON_VERSION ${PYTHON_VERSION} PYTHON_MODULE pigz)
      target_link_libraries(pigz_py_${PYTHON_VERSION} PUBLIC pigz ${EXT_LIBS})
      add_dependencies(pigz_py pigz_py_${PYTHON_VERSION})
  endforeach()

endif(PYBIND)
