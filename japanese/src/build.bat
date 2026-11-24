@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
cd /d "%~dp0"
set O=obj\release
set OUT=IWD2JP.exe
set DEF=
if "%1"=="test" (
  set O=obj\test
  set OUT=test\IWD2JP.exe
  set DEF=/DIWD2JP_TEST
  if not exist test mkdir test
)
if not exist %O% mkdir %O%
rc /nologo /fo %O%\iwd2jp.res iwd2jp.rc || exit /b 1
cl /nologo /O2 /W3 /MT /source-charset:utf-8 /execution-charset:.932 %DEF% /Fo%O%\ iwd2jp.c %O%\iwd2jp.res /Fe:%OUT% /link /MANIFEST:NO kernel32.lib advapi32.lib || exit /b 1
