# Library info
MACRO (PRINT_LIBRARY_INFO libName libDetect compilerFlags linkerFlags)
        IF (${libDetect})
                MESSAGE(STATUS "Found ${libName}")

                IF (VERBOSE)
                        SET(_compilerFlags "")
                        APPEND_FLAGS(_compilerFlags ${compilerFlags})
                        SET(_linkerFlags "")
                        APPEND_FLAGS(_linkerFlags ${linkerFlags})
                        MESSAGE(STATUS "Compiler Flags:${_compilerFlags}")
                        MESSAGE(STATUS "Linker Flags  :${_linkerFlags}")
                ENDIF (VERBOSE)
                
        ELSE (${libDetect})
            IF (${ARGV4})
                    MESSAGE(${ARGV4} "Could not find ${libName}")
                ELSE (${ARGV4})
                    MESSAGE(STATUS "Could not find ${libName}")
                ENDIF (${ARGV4})
        ENDIF (${libDetect})
ENDMACRO (PRINT_LIBRARY_INFO)

# Library summary
MACRO (PRINT_LIBRARY_SUMMARY libName libDetect)
        IF (${libDetect})
            MESSAGE(STATUS "###   ${libName} = YES")
        ELSE  (${libDetect})
                MESSAGE(STATUS "###   ${libName} = NO")
        ENDIF (${libDetect})
ENDMACRO (PRINT_LIBRARY_SUMMARY)

MACRO (APPEND_FLAGS _flags)
        IF (NOT ${_flags})
                SET(${_flags} "")
        ENDIF (NOT ${_flags})

        FOREACH (_flag ${ARGN})
                SET(${_flags} "${${_flags}} ${_flag}")
        ENDFOREACH (_flag ${ARGN})

        IF (${firstChar} AND ${firstChar} STREQUAL " ")
                STRING(LENGTH "${${_flags}}" stringLength)
                MATH(EXPR stringLength "${stringLength} - 1")
                STRING(SUBSTRING "${${_flags}}" 1 ${stringLength} ${_flags})
        ENDIF (${firstChar} AND ${firstChar} STREQUAL " ")
ENDMACRO (APPEND_FLAGS)

#ARGV1 = flags
MACRO (ADD_SOURCE_CFLAGS _target)
        GET_SOURCE_FILE_PROPERTY(_flags ${_target} COMPILE_FLAGS)
        APPEND_FLAGS(_flags ${ARGN})
        SET_SOURCE_FILES_PROPERTIES(${_target} PROPERTIES COMPILE_FLAGS "${_flags}")   
ENDMACRO (ADD_SOURCE_CFLAGS)


MACRO (ADD_TARGET_CFLAGS _target)
        GET_TARGET_PROPERTY(_flags ${_target} COMPILE_FLAGS)
        APPEND_FLAGS(_flags ${ARGN})
        SET_TARGET_PROPERTIES(${_target} PROPERTIES COMPILE_FLAGS "${_flags}")
ENDMACRO (ADD_TARGET_CFLAGS)

