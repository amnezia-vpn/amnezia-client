find_program(SIGNTOOL_COMMAND signtool REQUIRED)

function(signtool_sign_files PACKAGE_FILES SUBJECT_NAME CERT_HAS_UI)
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

		if (CERT_HAS_UI)
			execute_process(
				COMMAND cmd /c start "" /wait ${cmd}
				RESULT_VARIABLE result
			)
			if(NOT result EQUAL 0)
				message(FATAL_ERROR "signtool failed with code ${result}")
			endif()
		else()
			execute_process(
                COMMAND ${cmd}
                RESULT_VARIABLE result
                ERROR_VARIABLE error
            )
			if(NOT result EQUAL 0)
				string(REPLACE "\n" "\n  " error "  ${error}")
				message(FATAL_ERROR "signtool failed:\n${error}")
			endif()
		endif()
    endif()
endfunction()
