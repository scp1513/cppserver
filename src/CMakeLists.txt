SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS ON)

CMAKE_MINIMUM_REQUIRED (VERSION 2.8)
PROJECT (server)

SET(CMAKE_USE_RELATIVE_PATHS ON)

IF(NOT CMAKE_BUILD_TYPE)
	SET(CMAKE_BUILD_TYPE "Release")
ENDIF()

#MESSAGE(STATUS "cmake build type: ${CMAKE_BUILD_TYPE}")

LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)
INCLUDE(FindPCHSupport)

#asio
#http://think-async.com/Asio/Download

#rapidjson
#http://rapidjson.org/

#mysql c connector
#http://dev.mysql.com/downloads/connector/c/

IF(WIN32)
	############################################################################
    FIND_PATH(ASIO_INCLUDE_DIR
        NAMES asio.hpp
        PATHS $ENV{DIR_ASIO}/include)
    MARK_AS_ADVANCED(ASIO_INCLUDE_DIR)

	############################################################################
    FIND_PATH(RAPIDJSON_INCLUDE_DIR
        NAMES rapidjson/rapidjson.h
        PATHS $ENV{DIR_RAPIDJSON}/include)
    MARK_AS_ADVANCED(RAPIDJSON_INCLUDE_DIR)

	############################################################################
    FIND_PATH(MYSQL_INCLUDE_DIR
        NAMES mysql.h
        PATHS $ENV{DIR_MYSQL_INCLUDE})
    MARK_AS_ADVANCED(MYSQL_INCLUDE_DIR)

    FIND_LIBRARY(LIB_MYSQL_DEBUG
        NAMES libmysql
        PATHS $ENV{DIR_MYSQL_LIB})

    FIND_LIBRARY(LIB_MYSQL_RELEASE
        NAMES libmysql
        PATHS $ENV{DIR_MYSQL_LIB})

    MARK_AS_ADVANCED(LIB_MYSQL_DEBUG LIB_MYSQL_RELEASE)

    SET(MYSQL_LIBRARIES
        debug ${LIB_MYSQL_DEBUG}
        optimized ${LIB_MYSQL_RELEASE})

ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS "-std=c++11")

	############################################################################
    FIND_PATH(ASIO_INCLUDE_DIR
        NAMES asio.hpp
        PATHS $ENV{DIR_ASIO}/include)
    MARK_AS_ADVANCED(ASIO_INCLUDE_DIR)

	############################################################################
    FIND_PATH(RAPIDJSON_INCLUDE_DIR
        NAMES rapidjson/rapidjson.h
        PATHS $ENV{DIR_RAPIDJSON}/include)
    MARK_AS_ADVANCED(RAPIDJSON_INCLUDE_DIR)

	############################################################################
    FIND_PATH(MYSQL_INCLUDE_DIR
        NAMES mysql.h
        PATHS $ENV{DIR_MYSQL_INCLUDE})
    MARK_AS_ADVANCED(MYSQL_INCLUDE_DIR)

    FIND_LIBRARY(LIB_MYSQL_DEBUG
        NAMES mysqlclient
        PATHS $ENV{DIR_MYSQL_LIB})

    FIND_LIBRARY(LIB_MYSQL_RELEASE
        NAMES mysqlclient
        PATHS $ENV{DIR_MYSQL_LIB})

    MARK_AS_ADVANCED(
        LIB_MYSQL_DEBUG
        LIB_MYSQL_RELEASE)

    SET(MYSQL_LIBRARIES
        debug ${LIB_MYSQL_DEBUG}
        optimized ${LIB_MYSQL_RELEASE})

ENDIF()

add_subdirectory (engine)
add_subdirectory (servers)
