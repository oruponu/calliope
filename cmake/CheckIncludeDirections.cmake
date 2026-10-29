# Checks that every #include "..." under SOURCE_DIR goes in a direction that RULES allows. RULES sets
# LAYER_<layer> and FOLDER_<folder> to the layers or folders their files may include from other layers or other
# folders of the same layer. Layers and folders without a rule are not checked.
cmake_minimum_required(VERSION 3.22)

if(NOT DEFINED SOURCE_DIR OR NOT DEFINED RULES)
    message(FATAL_ERROR "usage: cmake -DSOURCE_DIR=<dir> -DRULES=<file> -P CheckIncludeDirections.cmake")
endif()

include(${RULES})

file(GLOB_RECURSE files RELATIVE ${SOURCE_DIR} ${SOURCE_DIR}/*.h ${SOURCE_DIR}/*.cpp ${SOURCE_DIR}/*.mm)
if(NOT files)
    message(FATAL_ERROR "no source files under ${SOURCE_DIR}")
endif()

set(includePattern "^[ \t]*#[ \t]*include[ \t]*\"([^\"]+)\"")
set(violationCount 0)
foreach(file IN LISTS files)
    get_filename_component(folder ${file} DIRECTORY)
    # Files directly under SOURCE_DIR belong to no layer.
    if(folder STREQUAL "")
        continue()
    endif()
    string(REGEX REPLACE "/.*" "" layer ${folder})

    file(STRINGS ${SOURCE_DIR}/${file} lines REGEX "${includePattern}")
    foreach(line IN LISTS lines)
        string(REGEX MATCH "${includePattern}" unused "${line}")
        set(path ${CMAKE_MATCH_1})
        string(REGEX REPLACE "/.*" "" targetLayer ${path})
        # Headers outside SOURCE_DIR, such as generated ones, are not checked.
        if(targetLayer STREQUAL path OR NOT IS_DIRECTORY ${SOURCE_DIR}/${targetLayer})
            continue()
        endif()
        get_filename_component(targetFolder ${path} DIRECTORY)

        set(allowed TRUE)
        if(NOT targetLayer STREQUAL layer)
            if(DEFINED LAYER_${layer} AND NOT targetLayer IN_LIST LAYER_${layer})
                set(allowed FALSE)
            endif()
        elseif(NOT targetFolder STREQUAL folder AND DEFINED FOLDER_${folder})
            if(NOT targetFolder IN_LIST FOLDER_${folder})
                set(allowed FALSE)
            endif()
        endif()

        if(NOT allowed)
            message("include direction violation: ${file} includes \"${path}\" (${folder} -> ${targetFolder})")
            math(EXPR violationCount "${violationCount} + 1")
        endif()
    endforeach()
endforeach()

if(violationCount GREATER 0)
    message(FATAL_ERROR "${violationCount} include direction violations")
endif()
