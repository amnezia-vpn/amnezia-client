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

		if (NOT "${PACKAGE_FILES}" MATCHES "\\.msi$")
			list(APPEND args /as)
		endif()

		set(cmd ${SIGNTOOL_COMMAND} ${args} ${PACKAGE_FILES})

		list(JOIN cmd " " cmd_str)
		message(STATUS "${cmd_str}")

		execute_process(
			COMMAND cmd /c start "" /wait ${cmd}
			RESULT_VARIABLE result
		)
		if(NOT result EQUAL 0)
			message(FATAL_ERROR "signtool failed with code ${result}")
		endif()
    endif()
endfunction()
