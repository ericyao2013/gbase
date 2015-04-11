# 禁止共享库
option(BUILD_SHARED_LIBS "build shared libraries." OFF)

# 编译类型
if (CMAKE_CONFIGURATION_TYPES)
    message(STATUS "CMAKE_CONFIGURATION_TYPES: ${CMAKE_CONFIGURATION_TYPES}")
elseif (CMAKE_BUILD_TYPE)
    message(STATUS "CMAKE_BUILD_TYPE: ${CMAKE_BUILD_TYPE}")
else()
    set(CMAKE_BUILD_TYPE "Debug")
    message(STATUS "default build type: ${CMAKE_BUILD_TYPE}")
endif()

# 编译选项（仅gcc、clang、vc）
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    set(COMMON_DEBUG "-g -ggdb -Wall -Werror -pg -O0 -Wno-unused-but-set-variable")
    set(COMMON_RELEASE "-g -ggdb -Wall -Werror -pg -O1 -Wno-unused-but-set-variable")
    set(COMMON_LINK_LIB z dl pthread m curl)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(COMMON_DEBUG "-g -ggdb -Wall -Werror -pg -O0 -Wno-unused-const-variable")
    set(COMMON_RELEASE "-g -ggdb -Wall -Werror -pg -O1 -Wno-unused-const-variable")
    set(COMMON_LINK_LIB z dl pthread m curl)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    set(COMMON_DEBUG "/Od /MDd")
    set(COMMON_RELEASE "/O2 /MD /D NDEBUG")
endif()

set(CMAKE_CXX_FLAGS_DEBUG "${COMMON_DEBUG}")
set(CMAKE_CXX_FLAGS_RELEASE "${COMMON_RELEASE}")
set(CMAKE_C_FLAGS_DEBUG "${COMMON_DEBUG}")
set(CMAKE_C_FLAGS_RELEASE "${COMMON_RELEASE}")

# 递归包含工程定义 *.cmake文件
macro(COMMON_PROJECT)
    set(COMMON_PROJECT_FILTER "*.cmake")
    foreach(basedir ${ARGV})
        file(GLOB_RECURSE COMMON_PROJECT_FILES "${basedir}/${COMMON_PROJECT_FILTER}")
        foreach(project_file ${COMMON_PROJECT_FILES})
            message(STATUS "project file found -- ${project_file}")
            include("${project_file}")
        endforeach()
    endforeach()
endmacro(COMMON_PROJECT)

# 颜色回显
function(CommonEcho)
    # ${ARGV}, ${ARGN}
    set(ECHO_WITH_COLOR_CMD "echo")
    set(ECHO_WITH_COLOR_CMD_DP "")

    if(UNIX OR CYGWIN OR APPLE)
        set(TAG_RED     "\\033[31;1m")
        set(TAG_GREEN   "\\033[32;1m")
        set(TAG_YELLOW  "\\033[33;1m")
        set(TAG_BLUE    "\\033[34;1m")
        set(TAG_PURPLE  "\\033[35;1m")
        set(TAG_CYAN    "\\033[36;1m")
        set(TAG_RESET   "\\033[;0m")

        set(ECHO_WITH_COLOR_CMD_DP "-e")

    elseif(WIN32)
        set(TAG_RED     "")
        set(TAG_GREEN   "")
        set(TAG_YELLOW  "")
        set(TAG_BLUE    "")
        set(TAG_PURPLE  "")
        set(TAG_CYAN    "")
        set(TAG_RESET   "")
    endif()

    set(ECHO_WITH_COLOR_PREFIX "")
    set(ECHO_WITH_COLOR_SUFFIX "")
    set(ECHO_WITH_COLOR_FLAG "false")

    foreach (msg IN LISTS ARGV)
        if ( "${msg}" STREQUAL "COLOR" )
            set(ECHO_WITH_COLOR_FLAG "true")
        elseif( "${ECHO_WITH_COLOR_FLAG}" STREQUAL "true" )
            set(ECHO_WITH_COLOR_FLAG "false")
            if ("${msg}" STREQUAL "RED")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_RED}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            elseif ("${msg}" STREQUAL "GREEN")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_GREEN}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            elseif ("${msg}" STREQUAL "YELLOW")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_YELLOW}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            elseif ("${msg}" STREQUAL "BLUE")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_BLUE}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            elseif ("${msg}" STREQUAL "PURPLE")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_PURPLE}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            elseif ("${msg}" STREQUAL "CYAN")
                set(ECHO_WITH_COLOR_PREFIX "${TAG_CYAN}")
                set(ECHO_WITH_COLOR_SUFFIX "${TAG_RESET}")
            else ()
                message(WARNING "EchoWithColor ${msg} not supported.")
            endif()
        else()
            execute_process(COMMAND ${ECHO_WITH_COLOR_CMD} ${ECHO_WITH_COLOR_CMD_DP} "${ECHO_WITH_COLOR_PREFIX}${msg}${ECHO_WITH_COLOR_SUFFIX}")
        endif()
    endforeach()

endfunction(CommonEcho)

# 编译平台
if ("${CMAKE_CXX_SIZEOF_DATA_PTR}" STREQUAL "4")
    CommonEcho(COLOR RED "-- platform ${CMAKE_CXX_PLATFORM_ID} 32")
elseif ("${CMAKE_CXX_SIZEOF_DATA_PTR}" STREQUAL "8")
    CommonEcho(COLOR RED "-- platform ${CMAKE_CXX_PLATFORM_ID} 64")
else()
    CommonEcho(COLOR RED "-- platform ${CMAKE_CXX_PLATFORM_ID} ??")
endif()

