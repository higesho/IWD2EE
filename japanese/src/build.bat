@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
cd /d "%~dp0"
cl /nologo /O2 /W3 /MT /source-charset:utf-8 /execution-charset:.932 iwd2jp.c /Fe:IWD2JP.exe /link /MANIFEST:NO kernel32.lib advapi32.lib
