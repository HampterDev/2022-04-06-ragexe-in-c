@echo off
REM Sets WINDOW_SDL to the words that build platform\window_sdl.c, the client's window services, into
REM a test: SDL's headers (WINDOW_SDL_INCLUDE), the file, and the static SDL vendor\build_vendor.bat
REM builds with the system libraries SDL's video uses, as build.bat links them (WINDOW_SDL_LIBS). ROOT
REM is the repository; the test also needs core\utf8.c and platform\code_page_win32.c, which
REM window_sdl.c calls.
set WINDOW_SDL_INCLUDE=/I"%ROOT%\build\vendor\sdl3-src\include"
set WINDOW_SDL_LIBS="%ROOT%\build\vendor\SDL3-static.lib" setupapi.lib version.lib cfgmgr32.lib shlwapi.lib kernel32.lib user32.lib gdi32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib imm32.lib winmm.lib
set WINDOW_SDL=%WINDOW_SDL_INCLUDE% "%ROOT%\src\platform\window_sdl.c" %WINDOW_SDL_LIBS%
