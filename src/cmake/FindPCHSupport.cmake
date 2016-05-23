FUNCTION(GET_COMMON_PCH_PARAMS PCH_HEADER PCH_FE INCLUDE_PREFIX)
  GET_FILENAME_COMPONENT(PCH_HEADER_N ${PCH_HEADER} NAME)
  GET_DIRECTORY_PROPERTY(TARGET_INCLUDES INCLUDE_DIRECTORIES)

  FOREACH(ITEM ${TARGET_INCLUDES})
    LIST(APPEND INCLUDE_FLAGS_LIST "${INCLUDE_PREFIX}\"${ITEM}\" ")
  ENDFOREACH(ITEM)

  SET(PCH_HEADER_NAME ${PCH_HEADER_N} PARENT_SCOPE)
  SET(PCH_HEADER_OUT ${CMAKE_CURRENT_BINARY_DIR}/${PCH_HEADER_N}.${PCH_FE} PARENT_SCOPE)
  SET(INCLUDE_FLAGS ${INCLUDE_FLAGS_LIST} PARENT_SCOPE)
ENDFUNCTION(GET_COMMON_PCH_PARAMS)

FUNCTION(GENERATE_CXX_PCH_COMMAND TARGET_NAME INCLUDE_FLAGS IN PCH_SRC OUT)
  IF (CMAKE_BUILD_TYPE)
    STRING(TOUPPER _${CMAKE_BUILD_TYPE} CURRENT_BUILD_TYPE)
  ENDIF ()

  SET(COMPILE_FLAGS ${CMAKE_CXX_FLAGS${CURRENT_BUILD_TYPE}})
  LIST(APPEND COMPILE_FLAGS ${CMAKE_CXX_FLAGS})

  IF ("${CMAKE_SYSTEM_NAME}" MATCHES "Darwin")
    IF (NOT "${CMAKE_OSX_ARCHITECTURES}" STREQUAL "")
      LIST(APPEND COMPILE_FLAGS "-arch ${CMAKE_OSX_ARCHITECTURES}")
    ENDIF ()
    IF (NOT "${CMAKE_OSX_SYSROOT}" STREQUAL "")
      LIST(APPEND COMPILE_FLAGS "-isysroot ${CMAKE_OSX_SYSROOT}")
    ENDIF ()
    IF (NOT "${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "")
      LIST(APPEND COMPILE_FLAGS "-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}")
    ENDIF ()
  ENDIF ()

  GET_DIRECTORY_PROPERTY(TARGET_DEFINITIONS COMPILE_DEFINITIONS)
  FOREACH(ITEM ${TARGET_DEFINITIONS})
    LIST(APPEND DEFINITION_FLAGS "-D${ITEM} ")
  ENDFOREACH(ITEM)

  SEPARATE_ARGUMENTS(COMPILE_FLAGS)
  SEPARATE_ARGUMENTS(INCLUDE_FLAGS)
  SEPARATE_ARGUMENTS(DEFINITION_FLAGS)

  GET_FILENAME_COMPONENT(PCH_SRC_N ${PCH_SRC} NAME)
  ADD_LIBRARY(${PCH_SRC_N}_dephelp MODULE ${PCH_SRC})

  ADD_CUSTOM_COMMAND(
    OUTPUT ${OUT}
    COMMAND ${CMAKE_CXX_COMPILER}
    ARGS ${DEFINITION_FLAGS} ${COMPILE_FLAGS} ${INCLUDE_FLAGS} -x c++-header ${IN} -o ${OUT}
    DEPENDS ${IN} ${PCH_SRC_N}_dephelp
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
  )

  ADD_CUSTOM_TARGET(generate_${PCH_SRC_N}
    DEPENDS ${OUT}
  )

  ADD_DEPENDENCIES(${TARGET_NAME} generate_${PCH_SRC_N})
ENDFUNCTION(GENERATE_CXX_PCH_COMMAND)

FUNCTION(ADD_CXX_PCH_GCC TARGET_NAME PCH_HEADER PCH_SOURCE)
  GET_COMMON_PCH_PARAMS(${PCH_HEADER} "gch" "-I")
  GENERATE_CXX_PCH_COMMAND(${TARGET_NAME} "${INCLUDE_FLAGS}" ${PCH_HEADER} ${PCH_SOURCE} ${PCH_HEADER_OUT})
  SET_TARGET_PROPERTIES(
    ${TARGET_NAME} PROPERTIES
    COMPILE_FLAGS "-include ${CMAKE_CURRENT_BINARY_DIR}/${PCH_HEADER_NAME}"
  )
ENDFUNCTION(ADD_CXX_PCH_GCC)

FUNCTION(ADD_CXX_PCH_CLANG TARGET_NAME PCH_HEADER PCH_SOURCE)
  GET_COMMON_PCH_PARAMS(${PCH_HEADER} "pch" "-I")
  GENERATE_CXX_PCH_COMMAND(${TARGET_NAME} "${INCLUDE_FLAGS}" ${PCH_HEADER} ${PCH_SOURCE} ${PCH_HEADER_OUT})
  SET_TARGET_PROPERTIES(
    ${TARGET_NAME} PROPERTIES
    COMPILE_FLAGS "-include-pch ${PCH_HEADER_OUT}"
  )
ENDFUNCTION(ADD_CXX_PCH_CLANG)

FUNCTION(ADD_CXX_PCH_MSVC TARGET_NAME PCH_HEADER PCH_SOURCE)
  GET_COMMON_PCH_PARAMS(${PCH_HEADER} "pch" "/I")
  SET_TARGET_PROPERTIES(
    ${TARGET_NAME} PROPERTIES
    COMPILE_FLAGS "/FI${PCH_HEADER_NAME} /Yu${PCH_HEADER_NAME}"
  )
  SET_SOURCE_FILES_PROPERTIES(
    ${PCH_SOURCE} PROPERTIES
    COMPILE_FLAGS "/Yc${PCH_HEADER_NAME}"
  )
ENDFUNCTION(ADD_CXX_PCH_MSVC)

MACRO(USE_MSVC_PCH PCH_TARGET PCH_HEADER_FILE PCH_SOURCE_FILE LIMIT)
    IF(MSVC)
        GET_FILENAME_COMPONENT(PCH_NAME ${PCH_HEADER_FILE} NAME_WE)

        # Compute a custom name for the precompiled header.
        IF(CMAKE_CONFIGURATION_TYPES)
            SET(PCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/PCH/${CMAKE_CFG_INTDIR}")
        ELSE(CMAKE_CONFIGURATION_TYPES)
            SET(PCH_DIR "${CMAKE_CURRENT_BINARY_DIR}/PCH")
        ENDIF(CMAKE_CONFIGURATION_TYPES)
        FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/PCH)

        SET(PCH_FILE "/Fp${PCH_DIR}/${PCH_NAME}.pch")

        SET_TARGET_PROPERTIES(${PCH_TARGET} PROPERTIES COMPILE_FLAGS
            "/Yu${PCH_HEADER_FILE} /FI${PCH_HEADER_FILE} ${PCH_FILE} /Zm${LIMIT}")
        SET_SOURCE_FILES_PROPERTIES(${PCH_SOURCE_FILE} PROPERTIES COMPILE_FLAGS
            "/Yc${PCH_HEADER_FILE}")

        SET_DIRECTORY_PROPERTIES(PROPERTIES
            ADDITIONAL_MAKE_CLEAN_FILES ${PCH_DIR}/${PCH_NAME}.pch)

    ENDIF(MSVC)
ENDMACRO(USE_MSVC_PCH)

FUNCTION(ADD_CXX_PCH TARGET_NAME PCH_HEADER PCH_SOURCE LIMIT)
  IF (MSVC)
    USE_MSVC_PCH(${TARGET_NAME} ${PCH_HEADER} ${PCH_SOURCE} ${LIMIT})
  ELSEIF ("${CMAKE_GENERATOR}" MATCHES "Xcode")
    SET_TARGET_PROPERTIES(${TARGET_NAME} PROPERTIES
      XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES
      XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/${PCH_HEADER}"
    )
  ELSEIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    ADD_CXX_PCH_CLANG(${TARGET_NAME} ${PCH_HEADER} ${PCH_SOURCE})
  ELSEIF ("${CMAKE_CXX_COMPILER_ID}" MATCHES "GNU")
    ADD_CXX_PCH_GCC(${TARGET_NAME} ${PCH_HEADER} ${PCH_SOURCE})
  ENDIF ()
ENDFUNCTION(ADD_CXX_PCH)
