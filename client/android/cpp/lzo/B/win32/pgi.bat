@echo // Copyright (C) 1996-2017 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 32-bit
@echo //   Portland Group PGI C/C++
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set BLIB=lib%BNAME%.a
set CC=pgcc
set CF=-fast %CFI% %CFASM%
set LF=%BLIB% -lwinmm

%CC% %CF% -c src\*.c
@if errorlevel 1 goto error
ar rcs %BLIB% *.o asm/i386/obj/win32/*.obj
@if errorlevel 1 goto error

%CC% %CF% -o dict.exe examples\dict.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o lzopack.exe examples\lzopack.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o precomp.exe examples\precomp.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o precomp2.exe examples\precomp2.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o simple.exe examples\simple.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -o lzotest.exe lzotest\lzotest.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -Iinclude\lzo minilzo\testmini.c minilzo\minilzo.c
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
