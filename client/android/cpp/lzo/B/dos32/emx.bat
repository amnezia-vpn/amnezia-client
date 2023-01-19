@echo // Copyright (C) 1996-2017 Markus F.X.J. Oberhumer
@echo //
@echo //   DOS 32-bit
@echo //   emx + gcc
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set BLIB=%BNAME%.a
set CC=gcc
set CF=@b/dos32/dj2.opt %CFI% %CFASM%
set LF=%BLIB% -s

%CC% %CF% -c @b\src.rsp
@if errorlevel 1 goto error
%CC% -x assembler-with-cpp -c asm/i386/src_gas/*.S
@if errorlevel 1 goto error
ar rcs %BLIB% @b/win32/cygwin.rsp
@if errorlevel 1 goto error

%CC% %CF% -o dict.exe examples/dict.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o lzopack.exe examples/lzopack.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o precomp.exe examples/precomp.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o precomp2.exe examples/precomp2.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -o simple.exe examples/simple.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -o lzotest.exe lzotest/lzotest.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -Iinclude/lzo -o testmini.exe minilzo/testmini.c minilzo/minilzo.c
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
