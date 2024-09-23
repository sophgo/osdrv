cmake_minimum_required(VERSION 3.16.5)

project(ipcm)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)


SET( CMAKE_SYSTEM_NAME Generic)
set( CMAKE_SYSTEM_PROCESSOR riscv64 )
set( ARCH riscv64 )
#set( CROSS_COMPILE aarch64-linux-gnu-)
set( CROSS_COMPILE riscv64-unknown-linux-musl-)

set(CMAKE_C_COMPILER ${CROSS_COMPILE}gcc)
set(CMAKE_CXX_COMPILER ${CROSS_COMPILE}g++)
set(CMAKE_S_COMPILER ${CROSS_COMPILE}gcc)

set(CMAKE_EXE_LINKER_FLAGS "-static")

set(CMAKE_INSTALL_PREFIX ${TOP_DIR}/install)
set(CMAKE_INSTALL_INC_PREFIX ${TOP_DIR}/install/include)

add_definitions("-Wall -g")

include_directories(../include/common/uapi core core/mailbox plat/include plat/common plat/linux/include
    plat/linux/message plat/linux/system test/common/ test/linux)

add_compile_definitions(__LINUX_USR__)

set(SRC_LIST core/ipcm_common.c
    plat/plat_common/ipcm_port_common.c
    plat/linux/linux_common/ipcm_linux_port_common.c
    plat/linux/message/ipcm_message.c
    plat/linux/system/ipcm_system.c
    plat/linux/anonymous/ipcm_anonymous.c)

add_library(ipcmuser ${SRC_LIST})

set(SRC_LIST2 test/common/ipcm_test_common.c
    test/common/ipcm_test_common_anon.c
    test/linux/ipcm_test_msg.c
    test/linux/ipcm_test_sys.c
    test/linux/ipcm_test_anon.c
    test/linux/test.c)

add_executable(test ${SRC_LIST} ${SRC_LIST2})

set(ALIOS_CLI_SRC test/common/ipcm_test_common.c
    test/common/ipcm_test_common_anon.c
    test/linux/ipcm_test_msg.c
    test/linux/alios_cli.c)

add_executable(alios_cli ${SRC_LIST} ${ALIOS_CLI_SRC})

