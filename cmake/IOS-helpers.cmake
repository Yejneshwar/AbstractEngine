# resource(<out-var> <source-path> <destination-path> <glob-pattern>)
function(resource VAR SOURCE_PATH DESTINATION PATTERN)
    file(GLOB_RECURSE _LIST CONFIGURE_DEPENDS ${SOURCE_PATH}/${PATTERN})
    foreach (RESOURCE ${_LIST})
        get_filename_component(_PARENT ${RESOURCE} DIRECTORY)
        if (${_PARENT} STREQUAL ${SOURCE_PATH})
            set(_DESTINATION ${DESTINATION})
        else ()
            file(RELATIVE_PATH _DESTINATION ${SOURCE_PATH} ${_PARENT})
            set(_DESTINATION ${DESTINATION}/${_DESTINATION})
        endif ()
        message("Resource ${RESOURCE} | Dest ${_DESTINATION}")
        set_property(SOURCE ${RESOURCE} PROPERTY MACOSX_PACKAGE_LOCATION ${_DESTINATION})
    endforeach (RESOURCE)
    set(${VAR} ${_LIST} PARENT_SCOPE)
endfunction()

function(codesign PROJECT_NAME)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Running custom post-build script..."
        COMMAND chmod +x ${CMAKE_SOURCE_DIR}/ios-codesign.sh
        COMMAND ${CMAKE_SOURCE_DIR}/ios-codesign.sh  # Path to your script
        COMMENT "Executing custom codesign script for ${PROJECT_NAME}"
    )
endfunction()