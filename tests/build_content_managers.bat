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
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /FI"%ROOT%\tests\content_manager_allocations.h" /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\curl-7.46.0\include" /I"%ROOT%\src\contents" "%ROOT%\tests\test_content_managers.c" "%OWNER%\core\lua_instance.c" "%OWNER%\core\map_sky.c" "%OWNER%\contents\enchant_mgr.c" "%OWNER%\contents\item_db_to_id.c" "%OWNER%\contents\item_reform.c" "%OWNER%\contents\repute_mgr.c" %EXTRA% "%ROOT%\src\contents\client_lua.c" "%ROOT%\src\contents\enchant_lua.c" "%ROOT%\src\contents\enchant_maps.c" "%ROOT%\src\contents\enchant_tables_free.c" "%ROOT%\src\contents\item_reform_std.c" "%ROOT%\src\contents\repute_std.c" "%ROOT%\src\contents\repute_maps.c" "%ROOT%\src\core\text_util.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\string_vec.c" "%ROOT%\src\std\tree.c" /Fe:test_content_managers.exe /link /OPT:REF "%ROOT%\build\vendor\lua514.lib" user32.lib winmm.lib oleaut32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_content_managers.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
