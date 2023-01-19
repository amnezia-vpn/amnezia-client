Simple make drivers for DOS, Windows, OS/2 and other systems
============================================================

This directory contains a bunch of simple build scripts - I've tried
to make them as foolproof as possible.

To build LZO for your system type 'b\OS\COMPILER' in the base directory,
e.g. 'b\win32\vc.bat' will build the Win32 Visual C/C++ version.

After building do a basic test by running 'lzotest.exe -mlzo COPYING'.
util\check.sh is an example of a more thorough test driver.

Please send me your additional/improved versions.


Overview:
---------

b\dos32\bc_pp.bat           Borland C/C++                           (1)
b\dos32\dj2.bat             djgpp v2 + gcc                          (1)
b\dos32\dm.bat              Digital Mars C/C++                      (1)
b\dos32\emx.bat             emx + gcc                               (1)
b\dos32\sc.bat              Symantec C/C++                          (1)
b\dos32\wc.bat              Watcom C/C++                            (1)

b\os2\emx.bat               emx + gcc                               (1)
b\os2\wc.bat                Watcom C/C++                            (1)

b\win32\bc.bat              Borland C/C++                           (1)
b\win32\cygwin.bat          Cygwin + gcc                            (1)
b\win32\dm.bat              Digital Mars C/C++                      (1)
b\win32\ic.bat              Intel C/C++                             (1)
b\win32\mingw.bat           MinGW + gcc                             (1)
b\win32\mwerks.bat          Metrowerks CodeWarrior C/C++            (1)
b\win32\pellesc.bat         Pelles C                                (1)
b\win32\pgi.bat             Portland Group PGI C/C++                (1)
b\win32\rsxnt.bat           rsxnt + gcc                             (1)
b\win32\sc.bat              Symantec C/C++                          (1)
b\win32\vc.bat              Visual C/C++                            (1)
b\win32\vc_dll.bat          Visual C/C++ (DLL version)              (1)
b\win32\wc.bat              Watcom C/C++                            (1)

b\win64\ic.bat              Intel C/C++ (Itanium)
b\win64\ic_dll.bat          Intel C/C++ (Itanium) (DLL version)
b\win64\vc.bat              Visual C/C++ (AMD64 or Itanium)
b\win64\vc_dll.bat          Visual C/C++ (AMD64 or Itanium) (DLL version)


Notes:
  (1) includes support for i386 assembler versions

