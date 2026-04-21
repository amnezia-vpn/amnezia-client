# Filters Xcode LinkFileList entries before final link in iOS simulator UI-only
# builds. This strips Qt permission plugin objects/libraries that may come from a
# device-only Qt package and fail simulator linkage.

set(_obj_dir "$ENV{OBJECT_FILE_DIR_normal}")
set(_product_name "$ENV{PRODUCT_NAME}")

if(_obj_dir STREQUAL "" OR _product_name STREQUAL "")
    message(STATUS "ios-filter-linkfile: OBJECT_FILE_DIR_normal/PRODUCT_NAME are not set, skip")
    return()
endif()

set(_link_file "${_obj_dir}/${_product_name}.LinkFileList")
if(NOT EXISTS "${_link_file}")
    message(STATUS "ios-filter-linkfile: LinkFileList not found: ${_link_file}")
    return()
endif()

file(STRINGS "${_link_file}" _lines)

set(_kept_lines "")
set(_removed_count 0)
foreach(_line IN LISTS _lines)
    if(_line MATCHES "/plugins/permissions/")
        math(EXPR _removed_count "${_removed_count} + 1")
    else()
        list(APPEND _kept_lines "${_line}")
    endif()
endforeach()

if(_removed_count GREATER 0)
    string(REPLACE ";" "\n" _new_content "${_kept_lines}")
    file(WRITE "${_link_file}" "${_new_content}\n")
    message(STATUS "ios-filter-linkfile: removed ${_removed_count} permission plugin link entries")
else()
    message(STATUS "ios-filter-linkfile: no permission plugin entries found")
endif()
