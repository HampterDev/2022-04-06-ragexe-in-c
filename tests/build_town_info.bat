@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OWNER=%ROOT%\src"
set "REFERENCE="
if not exist "%ROOT%\build\tests\town_info" mkdir "%ROOT%\build\tests\town_info"
pushd "%ROOT%\build\tests\town_info" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\lua_data_manager_boundaries.h" /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\src\contents" "%ROOT%\tests\test_town_info.c" "%OWNER%\contents\town_info.c" "%ROOT%\src\contents\lua_data.c" "%ROOT%\src\core\once_win32.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\tree.c" /Fe:test_town_info.exe /link /OPT:REF "%ROOT%\build\vendor\lua514.lib" user32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_town_info.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
