@echo off
REM build_move_place_wasm.bat - build the place a move record gives at a tick, the piece rules'
REM view\move_place.c and what it reads a record with, the files the client compiles, to
REM build\move_place.wasm with clang. The playground's runner (the server's
REM consumers\host\playground) loads it from beside ragexe_playground.exe, so that what the client
REM draws toward and what the runner expects come from one source.
REM
REM The module imports nothing. It exports memory, move_place, the place a record gives at a
REM tick, and __heap_base, the first address the host may use for its buffers.
REM
REM CLANG names the compiler; without it, tools\find_wasm_clang.bat finds one.
setlocal
set "ROOT=%~dp0.."
set "RULES=%ROOT%\game\rules"
call "%~dp0find_wasm_clang.bat" || exit /b 1
"%CLANG%" --target=wasm32 -std=c11 -nostdlib -ffreestanding -O2 -Wall -Wextra -Werror -I"%RULES%" -Wl,--no-entry -Wl,--export=move_place -Wl,--export=__heap_base -o "%ROOT%\build\move_place.wasm" "%RULES%\view\move_place.c" "%RULES%\view\move_track.c" "%RULES%\view\view_floors.c" "%RULES%\map\grid_path.c"
if not "%errorlevel%"=="0" exit /b 1
echo built build\move_place.wasm
exit /b 0
