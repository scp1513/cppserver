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
