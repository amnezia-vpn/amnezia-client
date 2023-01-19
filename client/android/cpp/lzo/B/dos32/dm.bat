@echo // Copyright (C) 1996-2017 Markus F.X.J. Oberhumer
@echo //
@echo //   DOS 32-bit
@echo //   Digital Mars C/C++
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=dmc -mx
set CF=-o -w- %CFI% %CFASM%
set LF=%BLIB% x32.lib

%CC% %CF% -c @b\src.rsp
@if errorlevel 1 goto error
lib %BLIB% /b /c /n /noi @b\win32\bc.rsp
@if errorlevel 1 goto error

%CC% %CF% examples\dict.c %LF%
@if errorlevel 1 goto error
%CC% %CF% examples\lzopack.c %LF%
@if errorlevel 1 goto error
%CC% %CF% examples\precomp.c %LF%
@if errorlevel 1 goto error
%CC% %CF% examples\precomp2.c %LF%
@if errorlevel 1 goto error
%CC% %CF% examples\simple.c %LF%
@if errorlevel 1 goto error

%CC% %CF% lzotest\lzotest.c %LF%
@if errorlevel 1 goto error

set LF=x32.lib
%CC% %CF% -Iinclude\lzo minilzo\testmini.c minilzo\minilzo.c %LF%
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
