@echo off
REM tools\shaders\build.bat - regenerates src\render\shaders\<name>.h from each <name>.hlsl beside
REM it (tools\shaders\shaders.c says what they hold). Run it after changing a shader source; the
REM builds never compile shaders, and tools\shaders\check.py (build.bat check, the test suite)
REM fails while a source and its header disagree. Needs the Vulkan SDK (VULKAN_SDK, as its
REM installer sets it: dxc, spirv-val and the SPIRV-Cross C API) and Visual Studio's C compiler.
REM The tool and the SPIR-V it compiles on the way go to build\shaders.
setlocal
for %%i in ("%~dp0..\..") do set "ROOT=%%~fi"
if not defined VULKAN_SDK ( echo *** tools\shaders\build.bat needs the Vulkan SDK ^(VULKAN_SDK is not set^) & exit /b 1 )
if not exist "%VULKAN_SDK%\Bin\dxc.exe" ( echo *** no dxc.exe in "%VULKAN_SDK%\Bin" & exit /b 1 )
for %%i in ("%VULKAN_SDK%") do set "SDKVERSION=%%~nxi"
set "WORK=%ROOT%\build\shaders"
if not exist "%WORK%" mkdir "%WORK%"
call "%ROOT%\tools\find_vcvars.bat" || exit /b 1
call "%VCVARS%" >nul
cl /nologo /O1 /W4 /WX /I"%VULKAN_SDK%\Include" "%~dp0shaders.c" /Fo:"%WORK%\\" /Fe:"%WORK%\shaders.exe" ^
 /link /LIBPATH:"%VULKAN_SDK%\Lib" spirv-cross-c-shared.lib
if errorlevel 1 ( echo *** building tools\shaders FAILED & exit /b 1 )
set "PATH=%VULKAN_SDK%\Bin;%PATH%"
set "SOURCES="
for %%f in ("%ROOT%\src\render\shaders\*.hlsl") do call set "SOURCES=%%SOURCES%% "%%~ff""
"%WORK%\shaders.exe" "%VULKAN_SDK%\Bin\dxc.exe" "%VULKAN_SDK%\Bin\spirv-val.exe" "%~dp0shaders.c" "%WORK%" "%SDKVERSION%" %SOURCES%
if errorlevel 1 ( echo *** generating the shaders FAILED & exit /b 1 )
exit /b 0
