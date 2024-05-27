function(QJSONSTRUCT_ADD_TEST TEST_NAME TEST_SOURCE)
    add_executable(${TEST_NAME} ${TEST_SOURCE} catch.hpp ${QJSONSTRUCT_SOURCES})
    target_include_directories(${TEST_NAME}
        PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        )
    target_link_libraries(
        ${TEST_NAME}
        PRIVATE
        Qt::Core
        )
    add_test(NAME QJSONSTRUCT_TEST_${TEST_NAME} COMMAND $<TARGET_FILE:${TEST_NAME}> -s)
endfunction()

QJSONSTRUCT_ADD_TEST(serialization serialize/main.cpp)
#QJSONSTRUCT_ADD_TEST(serialize_strings serialize/strings.cpp)
