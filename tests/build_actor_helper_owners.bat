@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OWNER=%ROOT%\src"
set "REFERENCE="
if not exist "%ROOT%\build\tests\actor_helper_owners" mkdir "%ROOT%\build\tests\actor_helper_owners"
pushd "%ROOT%\build\tests\actor_helper_owners" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %REFERENCE% /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_actor_helper_owners.c" "%OWNER%\actor\map_type_table.c" "%OWNER%\skills\skill_action_registry.c" "%OWNER%\std\vec.c" "%OWNER%\std\tree.c" /Fe:test_actor_helper_owners.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_actor_helper_owners.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
