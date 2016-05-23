file(GLOB_RECURSE headers_encryption_H *.h)
file(GLOB_RECURSE sources_encryption_CPP *.cpp)

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

set(encryption_SRCS
  ${headers_encryption_H}
  ${sources_encryption_CPP}
)

IF(WIN32)
    IF(MSVC)
        #SET(CMAKE_CXX_FLAGS "-DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x0501 -EHsc")
        SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
        SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
    ENDIF(MSVC)
ENDIF(WIN32)

add_library(encryption STATIC
  ${encryption_SRCS}
)

IF(WIN32)
  IF(MSVC)
    SET_TARGET_PROPERTIES(encryption PROPERTIES FOLDER "engine")
    SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin)
  ENDIF()
ELSEIF(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall")

    SET(LIBRARY_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/../bin/${CMAKE_BUILD_TYPE}/)
ENDIF()
