function(collect_source_files_recursively FOLDERS OUT_VAR)
    foreach(FOLDER ${FOLDERS})
        file(GLOB_RECURSE HPP "${FOLDER}/*.h" "${FOLDER}/*.hpp")
        file(GLOB_RECURSE SRC "${FOLDER}/*.c" "${FOLDER}/*.cc" "${FOLDER}/*.cpp")

        list(APPEND SOURCES ${SRC} ${HPP})
    endforeach()

    set(${OUT_VAR} ${SOURCES} PARENT_SCOPE)
endfunction()
