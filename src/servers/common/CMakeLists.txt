FILE(GLOB_RECURSE common_source *.h *.cpp)

INCLUDE_DIRECTORIES(
	${CMAKE_SOURCE_DIR}/engine
	${CMAKE_SOURCE_DIR}/servers
)

ADD_LIBRARY(common STATIC
	${common_source}
)

IF(WIN32)
	IF(MSVC)
		SET_TARGET_PROPERTIES(common PROPERTIES FOLDER "server/common")
		SET(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		SET(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
		SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../bin/)
	ENDIF()
ELSEIF(UNIX)
    SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/../bin/${CMAKE_BUILD_TYPE}/)
ENDIF()
