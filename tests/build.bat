@echo off
REM tests\build.bat [ctest arguments] - the whole test suite (tests\CMakeLists.txt) through CMake,
REM Ninja and CTest: configures build\tests the first time, builds what changed since the last run
REM and runs the tests on every core; then the oracle tests, 32-bit, in build\oracle
REM (tests\oracle), when the original executable is there (ORACLE_EXE, by default
REM ..\client-files\2022-04-06_Ragexe_1648707856.exe beside the checkout). Arguments go to both
REM ctest runs, for example -R chat_tabs (tests by name) or -L render (the tests of one library of
REM tools\libraries.txt). Tests labelled interactive, which need a real window, are left out.
setlocal
for %%i in ("%~dp0..") do set "ROOT=%%~fi"
set "JOBS=%NUMBER_OF_PROCESSORS%"
if not defined JOBS set "JOBS=4"

setlocal
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\tests"
REM A build\tests without a CMake cache holds the programs of the batch files before 2026-11-11.
if exist "%OUT%" if not exist "%OUT%\CMakeCache.txt" rd /s /q "%OUT%"
if not exist "%OUT%\CMakeCache.txt" (
  cmake -S "%ROOT%" -B "%OUT%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreaded
  if errorlevel 1 exit /b 1
)
cmake --build "%OUT%"
if errorlevel 1 exit /b 1
ctest --test-dir "%OUT%" -j %JOBS% --output-on-failure -LE interactive %*
if errorlevel 1 exit /b 1
endlocal

if not defined ORACLE_EXE set "ORACLE_EXE=%ROOT%\..\client-files\2022-04-06_Ragexe_1648707856.exe"
if not exist "%ORACLE_EXE%" (
  echo oracle tests skipped: no original executable at "%ORACLE_EXE%"
  exit /b 0
)
call "%ROOT%\tools\find_vcvars.bat" x86 || exit /b 1
call "%VCVARS%" >nul
set "OUT=%ROOT%\build\oracle"
if not exist "%OUT%\CMakeCache.txt" (
  cmake -S "%ROOT%" -B "%OUT%" -G Ninja -DCMAKE_BUILD_TYPE=Release -DRAGE_ORACLE=ON
  if errorlevel 1 exit /b 1
)
cmake --build "%OUT%"
if errorlevel 1 exit /b 1
ctest --test-dir "%OUT%" -j %JOBS% --output-on-failure %*
if errorlevel 1 exit /b 1
exit /b 0
