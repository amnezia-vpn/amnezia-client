# POST_BUILD helper: HKCU\Software\Classes\vpn\shell\open\command -> "app.exe" "%1"
# Invoke: cmake -DEXE_PATH="..." -P register_vpn_url_win.cmake
if(NOT DEFINED EXE_PATH OR EXE_PATH STREQUAL "")
    message(FATAL_ERROR "register_vpn_url_win.cmake: EXE_PATH is empty")
endif()
if(NOT EXISTS "${EXE_PATH}")
    message(WARNING "register_vpn_url_win.cmake: EXE not found (POST_BUILD order?): ${EXE_PATH}")
endif()

file(TO_NATIVE_PATH "${EXE_PATH}" _exe_native)
string(REPLACE "/" "\\" _exe_native "${_exe_native}")

# reg.exe: "C:\path\app.exe" "%1" — %1 is the full vpn://... string
set(_reg_dval "\"${_exe_native}\" \"%1\"")

execute_process(
    COMMAND reg ADD "HKCU\\Software\\Classes\\vpn\\shell\\open\\command"
        /ve /t REG_SZ /d "${_reg_dval}" /f
    RESULT_VARIABLE _reg_res
    ERROR_VARIABLE _reg_err
)
if(NOT _reg_res EQUAL 0)
    message(WARNING "register_vpn_url_win.cmake: reg ADD failed (code ${_reg_res}): ${_reg_err}")
else()
    message(STATUS "vpn:// HKCU shell\\open\\command = ${_reg_dval}")
endif()
