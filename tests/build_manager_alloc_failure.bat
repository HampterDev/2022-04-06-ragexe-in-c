@echo off
setlocal
set "ROOT=%~dp0.."
set "OUT=%ROOT%\build\tests\manager_alloc_failure"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
python "%ROOT%\tests\extract_manager_alloc_failure.py" "%ROOT%" "%OUT%"
if errorlevel 1 exit /b 1
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /TC /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%OUT%" /I"%ROOT%\vendor\lua-5.1.4\src" "%ROOT%\tests\test_manager_alloc_failure.c" "%ROOT%\tests\manager_alloc_failure_stubs.c" "%ROOT%\src\contents\repute_mgr.c" "%ROOT%\src\contents\item_db_to_id.c" "%ROOT%\src\contents\item_reform.c" "%ROOT%\src\contents\enchant_mgr.c" "%ROOT%\src\std\string.c" "%ROOT%\src\std\container_memory.c" "%ROOT%\src\std\vec.c" "%ROOT%\src\std\tree.c" /Fe:test_manager_alloc_failure.exe /link user32.lib
if errorlevel 1 (popd & exit /b 1)
test_manager_alloc_failure.exe
set "CODE=%errorlevel%"
popd
exit /b %CODE%
