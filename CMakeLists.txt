cmake_minimum_required(VERSION 3.2)
project(zprof)
#enable_language(ASM)

#from github.com/zsummer

# plat : WIN32 APPLE UNIX    (UNIX contain UNIX like)

# CMAKE_CXX_COMPILER_ID:  GNU Intel Clang AppleClang MSVC 
# example IF (CMAKE_CXX_COMPILER_ID MATCHES "Clang") ENDIF()
# SET(CMAKE_C_COMPILER /usr/bin/gcc-8)
# SET(CMAKE_CXX_COMPILER /usr/bin/g++-8)
# jump compiler works check
# if(WIN32)
#    set(CMAKE_C_COMPILER_WORKS TRUE)
#    set(CMAKE_CXX_COMPILER_WORKS TRUE)
# endif(WIN32)

# jump this project build when msvc 
# set_target_properties(${PROJECT_NAME} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)

# set this project setup build when msvc 
# set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

# show msvc folder
#  SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 

# use C++ 14
# set(CMAKE_CXX_FLAGS -std=c++14) 

# 
# CMAKE_SOURCE_DIR   cmake root dir 
# CMAKE_CURRENT_SOURCE_DIR current cmakelist.txt dir  
# EXECUTABLE_OUTPUT_PATH can set it change bin out dir
# CMAKE_MODULE_PATH can set it change module dir 
# PROJECT_NAME cur project name 

# include 
# include_directories  
# link_directories 
# link_libraries 

# 
# execute_process
set(CMAKE_VERBOSE_MAKEFILEON ON)
if( NOT CMAKE_CONFIGURATION_TYPES )
    set( CMAKE_CONFIGURATION_TYPES Debug Release )
endif( NOT CMAKE_CONFIGURATION_TYPES )

set(CMAKE_BUILD_TYPE Debug CACHE STRING "cache debug release "  )
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS ${CMAKE_CONFIGURATION_TYPES} )

#分组添加到IDE
MACRO(GROUP_SRC_BY_DIR FILES)
    FOREACH(PATH_NAME ${${FILES}})
        STRING(REGEX REPLACE ${CMAKE_CURRENT_SOURCE_DIR}/\(.*\) \\1 RELATIVE_PATH ${PATH_NAME})
        STRING(REGEX MATCH "(.+)/.*" HAS_GROUP ${RELATIVE_PATH})
        IF(HAS_GROUP)
                STRING(REGEX REPLACE "(.+)/.*" \\1 GROUP_NAME ${RELATIVE_PATH})
                STRING(REPLACE "/" "\\" GROUP_NAME ${GROUP_NAME})
                MESSAGE(${PATH_NAME} " into " ${GROUP_NAME})
                SOURCE_GROUP(${GROUP_NAME} FILES ${PATH_NAME})
        ELSE()
                SOURCE_GROUP("" FILES ${PATH_NAME})
        ENDIF()
    ENDFOREACH(PATH_NAME)
ENDMACRO(GROUP_SRC_BY_DIR)

FILE(GLOB_RECURSE ARENA_H ${CMAKE_SOURCE_DIR}/src/*.h )
FILE(GLOB_RECURSE ARENA_C ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.c)



SET(SOURCES_H ${ARENA_H} )
SET(SOURCES_C ${ARENA_C})


# 自动INCLUDE所有头文件的目录  (可能会影响到include的优先级)
# list(FILTER <list> <INCLUDE|EXCLUDE> REGEX <regular_expression>)  


set(SOURCES ${SOURCES_H} ${SOURCES_C})
GROUP_SRC_BY_DIR(SOURCES)

FOREACH (FILE_NAME ${SOURCES})
    MESSAGE(${FILE_NAME})
endforeach()

FOREACH(FILE_NAME ${SOURCES_H})
        STRING( REGEX REPLACE "[^/\\\\]+$" " " DIR_NAME ${FILE_NAME} )
        LIST(APPEND DIR_NAME_LIST ${DIR_NAME})
ENDFOREACH()
LIST(REMOVE_DUPLICATES DIR_NAME_LIST)
FOREACH(DIR_NAME ${DIR_NAME_LIST})
        MESSAGE("auto include " ${DIR_NAME} )
        INCLUDE_DIRECTORIES(${DIR_NAME})
ENDFOREACH()

#通用设置部分 包括启用分组 设置 启动项目  
if(WIN32)
    ADD_DEFINITIONS(-DWIN32 -W3)
    SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON) 
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT coverage_test)
else()
    #ADD_DEFINITIONS(-Wall -O3 )
    ADD_DEFINITIONS(-Wall -O2 )
#    ADD_DEFINITIONS(-Wall -g -ggdb -O0 )
    set(CMAKE_CXX_FLAGS -std=c++14)
#    link_directories(${CMAKE_SOURCE_DIR}/res3.5/libs)
#    link_libraries(protobuf pthread m c)
     link_libraries(pthread m c)
endif()

if(Loop)
    ADD_DEFINITIONS(-DLOOP_IMPL_VERSION=${Loop} )
    MESSAGE(${Loop})
endif()
#输出
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}")


MESSAGE(${CMAKE_CXX_COMPILER})
#自定义部分 
ADD_DEFINITIONS(-DGLFW_USE_CONFIG_H -D_GLFWWIN32 -DOPEN_ZPROF)

if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
#  set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
#  set_target_properties(${test_ld_preload} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
elseif (NOT APPLE)
            SET_TARGET_PROPERTIES(${PROJECT_NAME}
                PROPERTIES 
                COMPILE_FLAGS "" 
                LINK_FLAGS " -lrt" )
endif()

macro(SUB_DIR_LIST result curdir)
    file(GLOB children RELATIVE ${curdir} ${curdir}/*)
    set(dirlist "")
    foreach(child ${children})
        if(IS_DIRECTORY ${curdir}/${child})
            LIST(APPEND dirlist ${child})
        endif()
    endforeach()
    set(${result} ${dirlist})
endmacro()



include_directories(${CMAKE_SOURCE_DIR}/example)
include_directories(${CMAKE_SOURCE_DIR}/example/common)
include_directories(${CMAKE_SOURCE_DIR}/example/common/include)
include_directories(${CMAKE_SOURCE_DIR}/example/common/include/fn-log)
include_directories(${CMAKE_SOURCE_DIR}/example/common/include/zprof)
include_directories(${CMAKE_SOURCE_DIR}/example/common/include/zbase)
SUB_DIR_LIST(SUB_DIRS ${CMAKE_SOURCE_DIR}/example/)

FOREACH(SUB_DIR ${SUB_DIRS}) 
    set(EXAMPLE_SOURCE "")
    if (${SUB_DIR} STREQUAL "common") 
        continue()
    endif()
    #FILE(GLOB_RECURSE EXAMPLE_SOURCE
    FILE(GLOB EXAMPLE_SOURCE
    ${CMAKE_SOURCE_DIR}/example/*.h 
    ${CMAKE_SOURCE_DIR}/example/common/*.h 
    ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.h 
    ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.c 
    ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.cc 
    ${CMAKE_SOURCE_DIR}/example/${SUB_DIR}/*.cpp)

    Message(${EXAMPLE_SOURCE})
    GROUP_SRC_BY_DIR(EXAMPLE_SOURCE)
    ADD_EXECUTABLE(${SUB_DIR} ${ARENA_C} ${ARENA_H} ${EXAMPLE_SOURCE})
    if (UNIX)
            SET_TARGET_PROPERTIES(${SUB_DIR}
                PROPERTIES 
                COMPILE_FLAGS "" 
                LINK_FLAGS "-ldl" 
            )
    else()
        if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
            set_target_properties(${SUB_DIR} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
        endif()
    endif()

    add_dependencies(${SUB_DIR} ${PROJECT_NAME})
ENDFOREACH()



IF (ENABLE_GCOV AND NOT WIN32 AND NOT APPLE)
    SET_TARGET_PROPERTIES(${PROJECT_NAME} 
        PROPERTIES 
        COMPILE_FLAGS "-ftest-coverage -fprofile-arcs" 
        LINK_FLAGS "-lgcov --coverage" 
    )
ENDIF()


