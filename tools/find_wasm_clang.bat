@echo off
REM find_wasm_clang.bat - sets CLANG to a clang that builds for wasm32, for the .wasm builds
REM (tools\build_drawn_wasm.bat, tools\build_grid_path_wasm.bat). A CLANG already set is kept.
REM Otherwise the first of clang from PATH and %ProgramFiles%\LLVM\bin\clang.exe whose targets
REM include wasm32: Visual Studio's own clang, which its environment puts first on PATH, has none.
REM Fails when neither has it.
if defined CLANG exit /b 0
for %%c in (clang.exe) do if not "%%~$PATH:c"=="" call :wasm "%%~$PATH:c"
if not defined CLANG call :wasm "%ProgramFiles%\LLVM\bin\clang.exe"
if defined CLANG exit /b 0
echo *** No clang that builds for wasm32: install LLVM, or set CLANG to one.
exit /b 1

:wasm
if not exist "%~1" exit /b 0
"%~1" --print-targets 2>nul | findstr /c:"wasm32" >nul && set "CLANG=%~1"
exit /b 0
