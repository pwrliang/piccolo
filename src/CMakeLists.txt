cmake_minimum_required (VERSION 2.6) 
cmake_policy(SET CMP0014 OLD)

project (PICCOLO) 

set(ENV{LANG} "C")

find_package(SWIG REQUIRED)
find_package(PythonLibs REQUIRED)
find_package(MPI REQUIRED)
find_package( Boost COMPONENTS system thread REQUIRED )


include(FindProtobuf)
find_package(Protobuf REQUIRED)


set(CMAKE_CXX_COMPILE_FLAGS ${CMAKE_CXX_COMPILE_FLAGS} ${MPI_COMPILE_FLAGS})
set(CMAKE_CXX_LINK_FLAGS ${CMAKE_CXX_LINK_FLAGS} ${MPI_LINK_FLAGS})

include("${PICCOLO_SOURCE_DIR}/PiccoloPP.cmake")
include(${SWIG_USE_FILE})

set(CMAKE_CXX_FLAGS "-fPIC -march=core2 -ggdb2 -Wall -Wno-sign-compare -Wno-unused-function -fno-omit-frame-pointer")
if ($ENV{TCMALLOC})
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHEAPPROF ")
endif()

if ($ENV{OPROFILE})
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCPUPROF ")
endif()

set(CMAKE_SWIG_FLAGS -ignoremissing -O -c++)
                     
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

add_subdirectory (external/google-test)
add_subdirectory (external/google-flags)
add_subdirectory (external/google-logging)
add_subdirectory (external/libunwind)
add_subdirectory (external/webgraph)

include_directories (${PICCOLO_SOURCE_DIR})
include_directories (${Boost_INCLUDE_DIR})
include_directories (external/google-test)
include_directories (external/google-flags)
include_directories (external/google-logging)
include_directories (external/libunwind)
include_directories (external/webgraph)

exec_program("mpic++ -showme:compile" OUTPUT_VARIABLE MPI_COMPILE_FLAGS)
exec_program("mpic++ -showme:incdirs" OUTPUT_VARIABLE MPI_INCDIRS)
exec_program("mpic++ -showme:link" OUTPUT_VARIABLE MPI_LINK_FLAGS)
exec_program("mpic++ -showme:libdirs" OUTPUT_VARIABLE MPI_LIBDIRS)

string(REPLACE " " ";" MPI_INCDIRS "${MPI_INCDIRS}")
string(REPLACE " " ";" MPI_LINK_FLAGS "${MPI_LINK_FLAGS}")

include_directories( ${CMAKE_CURRENT_SOURCE_DIR}
                     ${PICCOLO_BINARY_DIR}
                     ${PROTOBUF_INCLUDE_DIRS}
                     ${PYTHON_INCLUDE_PATH}
                     ${MPI_INCDIRS} )

link_directories( ${MPI_LIBDIRS} )
include_directories(${MPI_INCDIRS})

add_subdirectory (piccolo)
# add_subdirectory (client/python)
add_subdirectory (examples)
add_subdirectory (examples/crawler)
add_subdirectory (examples/nas)
	
