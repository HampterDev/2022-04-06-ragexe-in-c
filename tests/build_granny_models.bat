@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\granny_models" mkdir "%ROOT%\build\tests\granny_models"
pushd "%ROOT%\build\tests\granny_models" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /DGRANNY_IMPORT= /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_granny_model_storage.c" "%ROOT%\tests\granny_model_geometry.c" "%ROOT%\src\render\math3d.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\core\fatal.c" /Fe:test_granny_models.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_granny_models.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
