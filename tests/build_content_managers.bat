@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OWNER=%ROOT%\src"
set "REFERENCE="
set "EXTRA="
if not exist "%ROOT%\build\tests\content_managers" mkdir "%ROOT%\build\tests\content_managers"
pushd "%ROOT%\build\tests\content_managers" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\content_manager_allocations.h" /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\src\contents" "%ROOT%\tests\test_content_managers.c" "%OWNER%\core\map_sky.c" %EXTRA% "%ROOT%\src\contents\client_lua.c" "%ROOT%\src\core\text_util.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\std\tree.c" "%ROOT%\src\platform\code_page.c" "%ROOT%\src\platform\code_page_win32.c" /Fe:test_content_managers.exe /link /OPT:REF "%ROOT%\build\vendor\lua514.lib" user32.lib winmm.lib oleaut32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_content_managers.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
