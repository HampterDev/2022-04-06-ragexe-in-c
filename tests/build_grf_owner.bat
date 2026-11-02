@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\grf_owner" mkdir "%ROOT%\build\tests\grf_owner"
pushd "%ROOT%\build\tests\grf_owner" || exit /b 1
set "ZLIB=%ROOT%\vendor\zlib-1.2.11"
cl /nologo /w /O2 /MD /c "%ZLIB%\adler32.c" "%ZLIB%\compress.c" "%ZLIB%\crc32.c" "%ZLIB%\deflate.c" "%ZLIB%\inffast.c" "%ZLIB%\inflate.c" "%ZLIB%\inftrees.c" "%ZLIB%\trees.c" "%ZLIB%\uncompr.c" "%ZLIB%\zutil.c"
if not "%errorlevel%"=="0" (popd & exit /b 1)
lib /nologo /out:grf_zlib.lib adler32.obj compress.obj crc32.obj deflate.obj inffast.obj inflate.obj inftrees.obj trees.obj uncompr.obj zutil.obj
if not "%errorlevel%"=="0" (popd & exit /b 1)
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_grf_archive_owner.c" "%ROOT%\src\std\std_sort.c" grf_zlib.lib "%ROOT%\src\core\cstr.c" "%ROOT%\src\platform\thread.c" "%ROOT%\src\platform\system_memory.c" /Fe:test_grf_archive_owner.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_grf_archive_owner.exe "%ROOT%\tests\fixtures\grf_records.bin"
set "CODE=%errorlevel%"
popd
exit /b %CODE%
