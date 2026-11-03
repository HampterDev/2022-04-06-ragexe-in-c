@echo off
setlocal
set "ROOT=%~dp0.."
set "SRC=%ROOT%\src"
set "OUT=%ROOT%\build\tests\window_lists"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%OUT%" mkdir "%OUT%"
pushd "%OUT%" || exit /b 1
cl /nologo /W3 /WX /we4013 /O2 /std:c11 /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%SRC%" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\yyjson-0.13.0" "%ROOT%\tests\test_window_lists.c" "%ROOT%\tests\window_lists_stubs.c" "%SRC%\ui\wndmgr\make_window.c" "%SRC%\ui\wndmgr\make_window_login.c" "%SRC%\ui\wndmgr\make_window_hud.c" "%SRC%\ui\wndmgr\make_window_trade.c" "%SRC%\ui\wndmgr\make_window_social.c" "%SRC%\ui\wndmgr\make_window_character.c" "%SRC%\ui\wndmgr\window_slots.c" "%SRC%\ui\wndmgr\forget_window.c" "%SRC%\ui\wndmgr\teardown.c" "%SRC%\ui\wndmgr\window_table.c" "%SRC%\ui\wndmgr\window_registry.c" "%SRC%\std\list.c" "%SRC%\std\tree.c" "%SRC%\std\string.c" /Fe:test_window_lists.exe
if errorlevel 1 (popd & exit /b 1)
test_window_lists.exe %*
set "CODE=%errorlevel%"
popd
exit /b %CODE%
