@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OWNER=%ROOT%\src\effects\ez2str_effect_mgr.c"
set "REFERENCE="
set "EXTRA="
if not exist "%ROOT%\build\tests\ez2str_manager" mkdir "%ROOT%\build\tests\ez2str_manager"
pushd "%ROOT%\build\tests\ez2str_manager" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" /I"%ROOT%\vendor\curl-7.46.0\include" "%ROOT%\tests\test_ez2str_manager.c" "%OWNER%" %EXTRA% "%ROOT%\src\json\json_tree.c" "%ROOT%\src\json\json_bson.c" "%ROOT%\src\json\json_loader.c" "%ROOT%\src\core\text_util.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\std_comdats.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\tree.c" /Fe:test_ez2str_manager.exe /link /OPT:REF user32.lib winmm.lib oleaut32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_ez2str_manager.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
