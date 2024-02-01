#配置 ARM 交叉编译
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

message(STATUS "CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")

#指定编译器的 sysroot 路径
set(TOOLCHAIN_DIR       /opt/v1010/x86_64-glibc-gnuabi64-2)
set(CMAKE_SYSROOT       ${TOOLCHAIN_DIR}/x86_64-unknown-linux-gnu/sys-root)
 
#指定交叉编译器 arm-gcc 和 arm-g++
set(CMAKE_C_COMPILER            ${TOOLCHAIN_DIR}/bin/x86_64-unknown-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER          ${TOOLCHAIN_DIR}/bin/x86_64-unknown-linux-gnu-g++)
message(STATUS "CMAKE_C_COMPILER=${CMAKE_C_COMPILER}")
message(STATUS "CMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}")

 
#为编译器添加编译选项
#set(CMAKE_C_FLAGS "-march=armv7ve -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7")
#set(CMAKE_CXX_FLAGS "-march=armv7ve -mfpu=neon -mfloat-abi=hard -mcpu=cortex-a7")

set(CMAKE_FIND_ROOT_PATH        ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
message(STATUS "CMAKE_FIND_ROOT_PATH=${CMAKE_FIND_ROOT_PATH}")