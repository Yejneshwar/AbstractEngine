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
    source_group("Resources" FILES ${_LIST})
    set(${VAR} ${_LIST} PARENT_SCOPE)
endfunction()

# asset(<out-var> <destination-path> <file1> [<file2>...])
#
# Gathers a list of specified asset files and sets their bundle destination property.
# This function does not preserve the source path structure, placing all
# found resources directly into the specified destination directory.
#
# <out-var>          - The variable to append the list of files to.
# <destination-path> - The destination inside the bundle (e.g., "Resources").
# <file...>:         - An explicit list of files to add as assets.
#
function(asset VAR DESTINATION)
    set(_ASSET_LIST ${ARGN})
    source_group("Resources" FILES ${_ASSET_LIST})
    # Iterate over each file provided in the arguments.
    foreach(ASSET_FILE ${_ASSET_LIST})
        # Set the property that tells CMake where to place this file
        # inside the macOS .app bundle. All files go to the same flat directory.
        set_property(SOURCE ${ASSET_FILE} PROPERTY MACOSX_PACKAGE_LOCATION ${DESTINATION})
        message(STATUS "Asset: ${ASSET_FILE} -> ${DESTINATION}")
    endforeach()
    # Append the file list to the output variable in the parent scope.
    set(${VAR} ${${VAR}} ${_ASSET_LIST} PARENT_SCOPE)
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