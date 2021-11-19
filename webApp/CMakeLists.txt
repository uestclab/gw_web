cmake_minimum_required(VERSION 3.5)
option(PUBLIC "This is a default option for PUBLIC" ON)
message(STATUS "build public platform switch:"${PUBLIC})
project(webAdapter)

include_directories("/home/dpdk/liqing/code/gw_web/webApp/migrate/gw_register/")
include_directories("/home/dpdk/liqing/code/gw_web/webApp/migrate/gw_utility/")
include_directories("/home/dpdk/liqing/code/gw_web/webApp/tmp_include/")
link_directories("/home/dpdk/liqing/code/gw_web/webApp/tmp_include")
include_directories("/home/dpdk/liqing/code/gw50/dependence/include/") #mosquitto.h

include_directories("/home/dpdk/liqing/code/component/dependence/include/")
include_directories("${PROJECT_SOURCE_DIR}/include")
include_directories("${PROJECT_SOURCE_DIR}/lib")
include_directories("${PROJECT_SOURCE_DIR}/utility_lib")
link_directories("${PROJECT_SOURCE_DIR}/utility_lib/lib/")
ADD_SUBDIRECTORY(utility_lib)

if(${PUBLIC} STREQUAL "ON")
	message(STATUS "start to build ARM platform.")
	# cross complie
	SET(CMAKE_SYSTEM_NAME Linux)
	SET(CMAKE_SYSTEM_PROCESSOR arm) 
	SET(TOOLCHAIN_DIR "/home/dpdk/liqing/petalinux/gcc-arm-linux-gnueabi")
	SET(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_DIR})
	SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
	SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
	SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

	SET(CMAKE_C_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-gcc-6.2.1)
	SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/bin/arm-linux-gnueabihf-g++)
	link_directories("/home/dpdk/liqing/code/component/dependence/lib/")
	link_directories("/home/dpdk/liqing/code/gw50/dependence/lib/")  #mosquitto
	link_directories("/home/dpdk/liqing/code/gw50/tools/bin/arm/") #mosquitto_client

	# zlog
	link_directories("/home/dpdk/liqing/common/lib/")
	include_directories("/home/dpdk/liqing/common/include/")
	# cross complie

elseif(${PUBLIC} STREQUAL "OFF")
    message(STATUS "start to build PC platform.")
	message(STATUS "PROJECT_SOURCE_DIR": ${PROJECT_SOURCE_DIR})
endif(${PUBLIC} STREQUAL "ON")

SET(CMAKE_BUILD_TYPE "Debug")  
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")  
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")


SET(CMAKE_VERBOSE_MAKEFILE ON)    
SET(ROOT_PATH ".")


aux_source_directory(${PROJECT_SOURCE_DIR}/migrate/gw_register REGISTER_SRCS)
aux_source_directory(${PROJECT_SOURCE_DIR}/migrate/gw_utility UTILITY_SRCS)

file(GLOB_RECURSE SOURCE_FILES "src/*.c")
file(GLOB_RECURSE ALL_EXECUTABLES "webAdapter.c")

add_executable(${PROJECT_NAME} ${ALL_EXECUTABLES} ${SOURCE_FILES} ${REGISTER_SRCS} ${UTILITY_SRCS})

target_link_libraries(${PROJECT_NAME} -pthread)
target_link_libraries(${PROJECT_NAME} -lzlog)
target_link_libraries(${PROJECT_NAME} -lm)
target_link_libraries(${PROJECT_NAME} webutil)
target_link_libraries(${PROJECT_NAME} fftw3f)
if(${PUBLIC} STREQUAL "ON") # arm
	target_link_libraries(${PROJECT_NAME} cjson)
	target_link_libraries(${PROJECT_NAME} cst)
	target_link_libraries(${PROJECT_NAME} mosquitto)
	target_link_libraries(${PROJECT_NAME} mosquitto_client)
	target_link_libraries(${PROJECT_NAME} gwcomponent)

endif(${PUBLIC} STREQUAL "ON")













