@echo off
REM CGO driver for MSVC cl: drop GCC-only flags (-Werror, -f*, -d*, -m*, etc.).
setlocal EnableDelayedExpansion
set "_args="
:argloop
if "%~1"=="" goto invoke
set "_a=%~1"
if /i "!_a!"=="-Werror" goto next
if /i "!_a!"=="-Wall" goto next
if /i "!_a!"=="-Wno-error" goto next
if "!_a:~0,2!"=="-W" goto next
if "!_a:~0,2!"=="-f" goto next
if "!_a:~0,2!"=="-d" goto next
if "!_a:~0,2!"=="-m" goto next
if "!_a:~0,5!"=="-std=" goto next
if /i "!_a!"=="-pthread" goto next
if /i "!_a!"=="-O2" (set "_args=!_args! /O2") & goto next
if /i "!_a!"=="-O0" (set "_args=!_args! /Od") & goto next
if /i "!_a!"=="-g" (set "_args=!_args! /Zi") & goto next
if /i "!_a!"=="-c" (set "_args=!_args! /c") & goto next
if /i "!_a!"=="-x" (shift & goto next)
if /i "%~1"=="c" goto next
if "!_a:~0,2!"=="-D" (set "_args=!_args! /D!_a:~2!") & goto next
if "!_a:~0,2!"=="-I" (set "_args=!_args! /I!_a:~2!") & goto next
if /i "!_a!"=="-o" (
  set "_o=%~2"
  if /i "!_o:~-2!"==".o" set "_args=!_args! /Fo!_o!"
  shift
  goto next
)
if not "!_a:~0,1!"=="-" set "_args=!_args! "!_a!""
:next
shift
goto argloop
:invoke
cl !_args!
exit /b !ERRORLEVEL!
