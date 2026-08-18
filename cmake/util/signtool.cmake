find_program(SIGNTOOL_COMMAND signtool REQUIRED)

function(signtool_sign_files PACKAGE_FILES SUBJECT_NAME)
    if(NOT SUBJECT_NAME)
        set(SUBJECT_NAME "Privacy Technologies OU")
    endif()

    if(PACKAGE_FILES)
        set(args sign
            /n ${SUBJECT_NAME}
            /fd sha256
            /td sha256
            /tr http://timestamp.comodoca.com/?td=sha256
        )

        foreach(file IN LISTS PACKAGE_FILES)
            set(local_args ${args})
            if (NOT "${file}" MATCHES "\\.msi$")
                list(APPEND local_args /as)
            endif()

            set(cmd ${SIGNTOOL_COMMAND} ${local_args} ${file})

            list(JOIN cmd " " cmd_str)
            message(STATUS "${cmd_str}")

            execute_process(
                COMMAND ${cmd}
                RESULT_VARIABLE result
                ERROR_VARIABLE error
            )
            if(NOT result EQUAL 0)
                string(REPLACE "\n" "\n  " error "  ${error}")
                message(FATAL_ERROR "signtool failed:\n${error}")
            endif()
        endforeach()
    endif()
endfunction()
