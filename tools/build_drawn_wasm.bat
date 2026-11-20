@echo off
REM build_drawn_wasm.bat - build the drawn place, src\world\drawn.c and what it reads a record
REM with, the files the client compiles, to build\drawn.wasm with clang. The playground's runner
REM (the server's consumers\host\playground) loads it from beside ragexe_playground.exe, so that
REM what the client draws and what the runner expects come from one source.
REM
REM The module imports nothing. It exports memory, drawn_place, the place a record gives at a
REM tick, and __heap_base, the first address the host may use for its buffers.
REM
REM CLANG names the compiler; without it, tools\find_wasm_clang.bat finds one.
setlocal
set "ROOT=%~dp0.."
call "%~dp0find_wasm_clang.bat" || exit /b 1
"%CLANG%" --target=wasm32 -std=c11 -nostdlib -ffreestanding -O2 -Wall -Wextra -Werror -I"%ROOT%\src" -Wl,--no-entry -Wl,--export=drawn_place -Wl,--export=__heap_base -o "%ROOT%\build\drawn.wasm" "%ROOT%\src\world\drawn.c" "%ROOT%\src\world\move_track.c" "%ROOT%\src\world\view_floors.c" "%ROOT%\src\body\grid_path.c"
if not "%errorlevel%"=="0" exit /b 1
echo built build\drawn.wasm
exit /b 0
