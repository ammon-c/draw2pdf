@echo off
rem #### Build all four configurations of zlib.

   if exist err del err

   call apath.bat vs2017 trunk win32
   if errorlevel 1 goto fail
   nmake -f Makefile %1 %2 %3 >> err
   if errorlevel 1 goto fail
   nmake -f Makefile "SHIP=1" %1 %2 %3 >> err
   if errorlevel 1 goto fail

   call apath.bat vs2017 trunk x64
   if errorlevel 1 goto fail
   nmake -f Makefile "WIN64=1" %1 %2 %3 >> err
   if errorlevel 1 goto fail
   nmake -f Makefile "SHIP=1" "WIN64=1" %1 %2 %3 >> err
   if errorlevel 1 goto fail

   type err
   echo Completed OK.
   exit /b 0

:fail
   type err
   echo !!!!!!!!!!!!!!!!!!!!!!!!
   echo Completed with error(s)!
   echo !!!!!!!!!!!!!!!!!!!!!!!!
   exit /b 1
