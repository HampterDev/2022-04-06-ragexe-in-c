@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
call "%ROOT%\vendor\build_vendor.bat" ifneeded
if errorlevel 1 exit /b 1
if not exist "%ROOT%\build\tests\audio" mkdir "%ROOT%\build\tests\audio"
pushd "%ROOT%\build\tests\audio" || exit /b 1
set "VDIR=%ROOT%\build\vendor"
cl /nologo /std:c11 /W3 /WX /O2 /Gy /Gw /MT /D_CRT_SECURE_NO_WARNINGS /DDR_MP3_NO_STDIO /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%VDIR%\sdl3-src\include" "%ROOT%\tests\test_audio.c" "%ROOT%\src\fileio\grf_pak.c" "%ROOT%\src\fileio\grf_cipher.c" "%ROOT%\src\fileio\grf_mapfile.c" "%ROOT%\src\std\std_sort.c" "%ROOT%\src\core\cstr.c" "%ROOT%\src\core\memory.c" "%ROOT%\src\platform\filesystem_win32.c" "%ROOT%\src\platform\thread_win32.c" "%ROOT%\src\platform\code_page_win32.c" "%ROOT%\src\core\utf8.c" "%VDIR%\dr_mp3.lib" "%VDIR%\SDL3-static.lib" "%VDIR%\imglibs.lib" /Fe:test_audio.exe /link /OPT:REF setupapi.lib version.lib cfgmgr32.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib oleaut32.lib ole32.lib uuid.lib winmm.lib imm32.lib
if errorlevel 1 (popd & exit /b 1)
REM The game's own sounds are tested when the game folder is there (AUDIO_GAME_DIR, by default
REM client-files beside the checkout), and skipped without it.
if "%AUDIO_GAME_DIR%"=="" set "AUDIO_GAME_DIR=%ROOT%\..\client-files"
test_audio.exe "%ROOT%\tests\fixtures" "%AUDIO_GAME_DIR%"
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
