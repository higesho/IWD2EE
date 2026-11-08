@echo off
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" >nul
cd /d "%~dp0"
cl /nologo /c /O2 /GS- /Gy- /Zl /utf-8 kinsoku.c /Fo:kinsoku.obj
cl /nologo /O2 /utf-8 /DTEST test.c kinsoku.c /Fe:test.exe
