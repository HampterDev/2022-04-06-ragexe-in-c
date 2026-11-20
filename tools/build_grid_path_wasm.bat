@echo off
REM build_grid_path_wasm.bat - build src\body\grid_path.c, the file the client compiles, to
REM build\wasm\grid_path.wasm with clang, for tools\check_grid_path_wasm.mjs.
REM
REM The module imports nothing. It exports memory, grid_path_search, grid_path_scratch_size,
REM grid_path_step_ms and __heap_base, the first address the host may use for its buffers.
REM
REM CLANG names the compiler; without it, tools\find_wasm_clang.bat finds one.
setlocal
set "ROOT=%~dp0.."
call "%~dp0find_wasm_clang.bat" || exit /b 1
if not exist "%ROOT%\build\wasm" mkdir "%ROOT%\build\wasm"
"%CLANG%" --target=wasm32 -std=c11 -nostdlib -ffreestanding -O2 -Wall -Wextra -Werror -I"%ROOT%\src" -Wl,--no-entry -Wl,--export=grid_path_search -Wl,--export=grid_path_scratch_size -Wl,--export=grid_path_step_ms -Wl,--export=__heap_base -o "%ROOT%\build\wasm\grid_path.wasm" "%ROOT%\src\body\grid_path.c"
if not "%errorlevel%"=="0" exit /b 1
echo built build\wasm\grid_path.wasm
exit /b 0
