##############################################################
#V0.0 ----create by sjw 2024/01
##############################################################
cmake_minimum_required(VERSION 3.00)
project(platform)


##############################################################
OPTION(PRODUCE_SO_ON "produce .so" OFF)  #动态库           
##############################################################

##############################################################
#************************************************************#
#checkinstall打包工具开关
#1）由于工具局限性，使用deb/rpm打包时，需要关闭.so.xxx.x软链接
#   管理功能软件版本的管理依靠系统自带dpkg和rpm命令
#2）在使用源码tar包安装时，打开.so.xxx.x版本管理功能
#************************************************************#
OPTION(SOLIBAPI_ON ".so.xxx.x" OFF)
#set(SOLIBAPI_ON          OFF)             #.so.xxx.x版本管理开关
##############################################################

##############################################################
OPTION(SWIG_ON "SWIG depends" OFF)
#set(SWIG_ON              OFF)             #跨语言支持开关
##############################################################

##############################################################
OPTION(TOOLCHAIN_ON "toolchain depends" ON) #交叉编译
##############################################################

###################
#system path
###################
if(TOOLCHAIN_ON)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules/Toolchain.cmake)
else(TOOLCHAIN_ON)
###################
#获取操作系统
###################
if(CMAKE_SYSTEM_NAME MATCHES "Linux")
   set(CURRENT_SYSTEM "Linux")
elseif(CMAKE_SYSTEM_NAME MATCHES "Windows")
   set(CURRENT_SYSTEM "Windows")
elseif(CMAKE_SYSTEM_NAME MATCHES "FreeBSD")
   set(CURRENT_SYSTEM "FreeBSD")
endif()
cmake_host_system_information(RESULT UNIX_NAME QUERY OS_VERSION)
message(STATUS "${UNIX_NAME}")

#if(CMAKE_CL_64)
#   set(CURRENT_PLATFORM "x64")
#else(CMAKE_CL_64)
#   set(CURRENT_PLATFORM "x86")
#endif(CMAKE_CL_64)
message(STATUS "current platform is ${CURRENT_SYSTEM}")

set(LINUX_HEADER_PATH        /usr/include)
set(LINUX_HEADER_PATH_LOCAL  /usr/local/include)
set(LINUX_LIB_PATH           /usr/lib
                             /usr/lib64)
set(LINUX_LIB_PATH_LOCAL     /usr/local/lib
                             /usr/local/lib64)
include_directories(${LINUX_HEADER_PATH_LOCAL})
link_directories(${LINUX_LIB_PATH_LOCAL})
endif(TOOLCHAIN_ON)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
###################
#版本控制
###################
# Version of the source code
set(FD_PROJECT_VERSION_MAJOR 0)
set(FD_PROJECT_VERSION_MINOR 0)
set(FD_PROJECT_VERSION_REV 0)
# Version of the API with the library
set(FD_PROJECT_VERSION_API 1)

###################
#调试模式开关
###################
set(CMAKE_BUILD_TYPE "DEBUG")
set(CMAKE_CFLAG "$ENV{CFLAGS} -Wextra -fPIC")
set(CMAKE_C_FLAGS_DEBUG "$ENV{CFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_C_FLAGS_RELEASE "$ENV{CFLAGS} -O3 -Wall -ffunction-sections -fdata-sections")

set(CMAKE_CXXFLAG "$ENV{CXXFLAGS} -Wextra -fPIC")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g3 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

###################
#root path
###################
set(OS_ROOT_PATH  ${CMAKE_CURRENT_SOURCE_DIR})
set(OS_BUILD_PATH ${CMAKE_CURRENT_BINARY_DIR})

###################
#code path
###################
set(OS_CORE_PATH  ${OS_ROOT_PATH}/core)
set(OS_TEST_PATH  ${OS_ROOT_PATH}/tests)
set(OS_TOOLS_PATH ${OS_ROOT_PATH}/tools)
#core path
set(OS_CORE_BSAE_PATH ${OS_CORE_PATH}/base)
set(OS_CORE_MODS_PATH ${OS_CORE_PATH}/mods)
set(OS_CORE_DEPS_PATH ${OS_CORE_PATH}/deps)
###################
#install path
###################
set(INSTALL_ROOT ${CMAKE_INSTALL_PREFIX}/platform)
message(STATUS "OS INSTALL PATH = ${INSTALL_ROOT}")
set(OS_INSTALL_PATH      ${INSTALL_ROOT})
set(OS_LIB_INSTALL_PATH  ${OS_INSTALL_PATH}/lib)
set(OS_TOOL_INSTALL_PATH ${OS_INSTALL_PATH}/tools)
set(OS_INC_INSTALL_PATH  ${OS_INSTALL_PATH})

set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${OS_LIB_INSTALL_PATH}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

###################
#uninstall path
###################
CONFIGURE_FILE(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY
)

ADD_CUSTOM_TARGET(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)

###################
#platform cmakeFind
###################
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")
#FIND_PACKAGE(APR REQUIRED)
#FIND_PACKAGE(APRUtil REQUIRED)
#include_directories(${APR_INCLUDE_DIRS}
#                    ${APRUtil_INCLUDE_DIRS})
#link_libraries(${APR_LIBRARIES}
#               ${APRUtil_LIBRARIES})

##############################################################
#************************************************************#
#平台编译
#************************************************************#
#src common include
include_directories(${OS_CORE_BSAE_PATH})
include_directories(${OS_CORE_BSAE_PATH}/inc)
##############################################################
#install tools
##############################################################
#IF(${UNIX_NAME} MATCHES "Ubuntu")
#install(FILES  ${OS_TOOLS_PATH}/linux_depends/ubuntu20.04.tar.gz  DESTINATION ${OS_TOOL_INSTALL_PATH}/linux_depends)
#ELSE()
#install(FILES  ${OS_TOOLS_PATH}/linux_depends/centos7.4.tar.gz  DESTINATION ${OS_TOOL_INSTALL_PATH}/linux_depends)
#ENDIF()


#install(DIRECTORY  ${OS_TOOLS_PATH}/os_depends  DESTINATION ${OS_TOOL_INSTALL_PATH})

#install(FILES  ${OS_TOOLS_PATH}/os_depends.sh 
#        DESTINATION ${OS_TOOL_INSTALL_PATH} 
#        PERMISSIONS OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_WRITE GROUP_EXECUTE WORLD_READ WORLD_WRITE WORLD_EXECUTE)

###################
#sub path
###################
add_subdirectory(${OS_CORE_BSAE_PATH}/src)




