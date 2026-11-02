@echo off
setlocal
set "ROOT=%~dp0.."
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
if not exist "%ROOT%\build\tests\local_account" mkdir "%ROOT%\build\tests\local_account"
pushd "%ROOT%\build\tests\local_account" || exit /b 1
cl /nologo /W3 /WX /O2 /Gy /Gw /MD /D_CRT_SECURE_NO_WARNINGS /source-charset:utf-8 /execution-charset:.949 /I"%ROOT%\src" /I"%ROOT%\vendor\lua-5.1.4\src" /I"%ROOT%\vendor\zlib-1.2.11" /I"%ROOT%\vendor\libpng-1.6.36" /I"%ROOT%\vendor\yyjson-0.13.0" /I"%ROOT%\vendor\giflib" "%ROOT%\tests\test_local_account.c" "%ROOT%\src\contents\account_settings.c" "%ROOT%\src\contents\account_settings_json.c" "%ROOT%\src\contents\settings_json.c" "%ROOT%\src\contents\user_hotkeys.c" "%ROOT%\src\contents\user_hotkey_apply.c" "%ROOT%\src\contents\whisper_block.c" "%ROOT%\src\contents\lua_calls.c" "%ROOT%\src\session\session_chat_macros.c" "%ROOT%\src\session\chat_macro_json.c" "%ROOT%\src\core\owned_text.c" "%ROOT%\src\core\text_array.c" "%ROOT%\src\core\text_encoding.c" "%ROOT%\src\json\service_json.c" "%ROOT%\src\json\json_tree.c" "%ROOT%\src\platform\window.c" "%ROOT%\src\platform\code_page.c" "%ROOT%\src\platform\sha256.c" "%ROOT%\src\platform\system.c" /Fe:test_local_account.exe /link /OPT:REF "%ROOT%\build\vendor\yyjson.lib" "%ROOT%\build\vendor\lua514.lib" user32.lib advapi32.lib
if not "%errorlevel%"=="0" (popd & exit /b 1)
test_local_account.exe
if not "%errorlevel%"=="0" (popd & exit /b 1)
popd
exit /b 0
