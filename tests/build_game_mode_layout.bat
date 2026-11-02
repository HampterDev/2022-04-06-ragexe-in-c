@echo off
setlocal
set "ROOT=%~dp0.."
set "SOURCE=%ROOT%\src"
set "MODE=current"
set "EXTRA="
set "LIFETIME=mode_lifetime.c"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\game_mode_layout_%MODE%" mkdir "%ROOT%\build\tests\game_mode_layout_%MODE%"
pushd "%ROOT%\build\tests\game_mode_layout_%MODE%" || exit /b 1
cl /nologo /W1 /WX /we4013 /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 %EXTRA% /I"%SOURCE%" /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_game_mode_layout.c" "%SOURCE%\world\%LIFETIME%" "%SOURCE%\world\mode_draw.c" "%SOURCE%\world\mode_dispatch.c" "%SOURCE%\web\name_cards.c" "%SOURCE%\core\once_win32.c" "%SOURCE%\std\string.c" "%SOURCE%\std\container_memory.c" "%SOURCE%\std\vec.c" "%SOURCE%\ui\std\ui_std_list.c" "%SOURCE%\ui\wndmgr\wndmgr_std.c" "%SOURCE%\std\list.c" "%SOURCE%\std\tree.c" winmm.lib user32.lib shell32.lib "%ROOT%\src\platform\clock_win32.c" "%ROOT%\src\platform\window_win32.c" "%ROOT%\src\platform\win_utf8.c" "%ROOT%\src\core\utf8.c" /Fe:test_game_mode_layout.exe /link /OPT:REF
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_game_mode_layout.exe %2
set "CODE=%errorlevel%"
popd
exit /b %CODE%
