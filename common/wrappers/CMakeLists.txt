project(wrappers C CXX)

if (NOT WIN32)
    message(FATAL_ERROR "Wrappers are dependent on WinAPI and can not be used on windows machine")
endif ()

collect_source_files_recursively("${PROJECT_SOURCE_DIR}" "WRAPPERS_SOURCES")

list(APPEND WRAPPERS_INCLUDES ${PROJECT_SOURCE_DIR}/include)

add_library(${PROJECT_NAME} STATIC ${WRAPPERS_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${WRAPPERS_INCLUDES})
