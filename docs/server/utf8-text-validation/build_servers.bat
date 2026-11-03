@echo off
REM Usage: build_servers.bat <source tree root> <log file>
REM Builds map-, login- and api-server (Release|Win32, v143) of an isolated Hercules tree.
setlocal
set "TREE=%~1"
set "LOG=%~2"
set "VCVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
call "%VCVARS%" >nul || exit /b 1
set "RC=0"
for %%P in (map-server login-server api-server) do (
  echo ===== %%P ===== >> "%LOG%"
  msbuild "%TREE%\vcproj-17\%%P.vcxproj" /m:4 /p:Configuration=Release /p:Platform=Win32 /nologo /verbosity:minimal /clp:Summary >> "%LOG%" 2>&1
  if errorlevel 1 (
    echo ***** %%P FAILED ***** >> "%LOG%"
    set "RC=1"
  ) else (
    echo ***** %%P OK ***** >> "%LOG%"
  )
)
exit /b %RC%
