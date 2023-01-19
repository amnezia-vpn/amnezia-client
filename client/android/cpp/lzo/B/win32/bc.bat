@echo // Copyright (C) 1996-2017 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 32-bit
@echo //   Borland C/C++
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=bcc32
set CF=-O2 -w -w-aus %CFI% -Iinclude\lzo %CFASM%
set LF=%BLIB%

%CC% %CF% -Isrc -c @b\src.rsp
@if errorlevel 1 goto error
tlib %BLIB% @b\win32\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% -ls -Iexamples examples\dict.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -ls -Iexamples examples\lzopack.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -ls -Iexamples examples\precomp.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -ls -Iexamples examples\precomp2.c %LF%
@if errorlevel 1 goto error
%CC% %CF% -ls -Iexamples examples\simple.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -ls -Ilzotest lzotest\lzotest.c %LF%
@if errorlevel 1 goto error

%CC% %CF% -ls -Iminilzo minilzo\testmini.c minilzo\minilzo.c
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
