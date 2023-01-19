@echo // Copyright (C) 1996-2017 Markus F.X.J. Oberhumer
@echo //
@echo //   Windows 32-bit
@echo //   lcc-win32
@echo //
@echo // NOTE: some lcc-win32 versions are buggy, so we disable optimizations
@echo //
@call b\prepare.bat
@if "%BECHO%"=="n" echo off


set CC=lcc
set CF=-O -A %CFI% -Iinclude\lzo %CFASM%
set CF=-A %CFI% -Iinclude\lzo %CFASM%
set LF=%BLIB% winmm.lib

for %%f in (src\*.c) do %CC% %CF% -c %%f
@if errorlevel 1 goto error
lcclib /out:%BLIB% @b\win32\vc.rsp
@if errorlevel 1 goto error

%CC% -c %CF% examples\dict.c
@if errorlevel 1 goto error
lc dict.obj %LF%
@if errorlevel 1 goto error
%CC% -c %CF% examples\lzopack.c
@if errorlevel 1 goto error
lc lzopack.obj %LF%
@if errorlevel 1 goto error
%CC% -c %CF% examples\precomp.c
@if errorlevel 1 goto error
lc precomp.obj %LF%
@if errorlevel 1 goto error
%CC% -c %CF% examples\precomp2.c
@if errorlevel 1 goto error
lc precomp2.obj %LF%
@if errorlevel 1 goto error
%CC% -c %CF% examples\simple.c
@if errorlevel 1 goto error
lc simple.obj %LF%
@if errorlevel 1 goto error

%CC% -c %CF% lzotest\lzotest.c
@if errorlevel 1 goto error
lc lzotest.obj %LF%
@if errorlevel 1 goto error

%CC% -c %CF% -Iinclude\lzo minilzo\testmini.c minilzo\minilzo.c
@if errorlevel 1 goto error
lc testmini.obj minilzo.obj
@if errorlevel 1 goto error


@call b\done.bat
@goto end
:error
@echo ERROR during build!
:end
@call b\unset.bat
