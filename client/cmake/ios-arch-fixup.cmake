if(NOT XCODE)
    return()
endif()

## Enumerate all the targets in the project
get_directory_property(IOS_SUBDIRS SUBDIRECTORIES)
get_directory_property(IOS_TARGETS BUILDSYSTEM_TARGETS)
while(IOS_SUBDIRS)
    list(POP_FRONT IOS_SUBDIRS SUBDIR)

    get_directory_property(SUBDIR_TARGETS DIRECTORY ${SUBDIR} BUILDSYSTEM_TARGETS)
    list(APPEND IOS_TARGETS ${SUBDIR_TARGETS})

    get_directory_property(SUBDIR_NESTED DIRECTORY ${SUBDIR} SUBDIRECTORIES)
    list(APPEND IOS_SUBDIRS ${SUBDIR_NESTED})
endwhile()

## The set of target types that we want to modify.
set(IOS_TARGET_COMPILED_TYPES
    STATIC_LIBRARY
    MODULE_LIBRARY
    SHARED_LIBRARY
    OBJECT_LIBRARY
    EXECUTABLE
)

## Inspect all the targets, and add extra properties if necessary.
while(IOS_TARGETS)
    list(POP_FRONT IOS_TARGETS TARGET_NAME)

    get_target_property(TARGET_TYPE ${TARGET_NAME} TYPE)
    list(FIND IOS_TARGET_COMPILED_TYPES ${TARGET_TYPE} IOS_TARGET_TYPE_INDEX)
    if(IOS_TARGET_TYPE_INDEX LESS 0)
        continue()
    endif()

    ## I just want to say it's amazing this doesn't explode with syntax errors.
    message("Patching architectures for ${TARGET_NAME}")
    set_target_properties(${TARGET_NAME} PROPERTIES
        XCODE_ATTRIBUTE_ARCHS[sdk=iphoneos*] "arm64"
        XCODE_ATTRIBUTE_ARCHS[sdk=iphonesimulator*] "x86_64"
    )
endwhile()