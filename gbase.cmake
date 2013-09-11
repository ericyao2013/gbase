include("${CMAKE_SOURCE_DIR}/common.cmake")

# 包含头文件
include_directories("${CMAKE_SOURCE_DIR}")

set(GBASE_DIR_CORE "${CMAKE_SOURCE_DIR}/core")
set(GBASE_DIR_DS "${CMAKE_SOURCE_DIR}/ds")
set(GBASE_DIR_NET "${CMAKE_SOURCE_DIR}/net")
set(GBASE_DIR_TEST "${CMAKE_SOURCE_DIR}/test")

set(GBASE_LIB gbase)

# 链接选项
set (SYSTEM_LIB_LINK z dl pthread rt m)

# 编译lib的源文件
aux_source_directory(${GBASE_DIR_CORE} GBASE_SOURCE)
aux_source_directory(${GBASE_DIR_DS} GBASE_SOURCE)
aux_source_directory(${GBASE_DIR_NET} GBASE_SOURCE)
CommonEcho(COLOR RED "source: ${GBASE_SOURCE}")

# 编译lib
add_library(${GBASE_LIB} ${GBASE_SOURCE})

# 递归增加test
file(GLOB GBASE_TEST_DIRS ${GBASE_DIR_TEST}/*test*)
foreach(GBASE_TEST_DIR ${GBASE_TEST_DIRS})
    CommonEcho(COLOR RED "dir: ${GBASE_TEST_DIR}")
    add_subdirectory(${GBASE_TEST_DIR})
endforeach()
