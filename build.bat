@echo off
title ASI Plugin Compiler
cl.exe >nul 2>&1
if %errorlevel% neq 9009 goto compile

if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
    goto compile
)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars32.bat"
    goto compile
)
echo [ОШИБКА] Компилятор cl.exe не найден. Установите Visual Studio C++.
pause
exit

:compile
echo Компиляция ServerConnect.asi...
cl.exe /LD /O2 /EHsc main.cpp /link /OUT:ServerConnect.asi
if exist ServerConnect.asi (
    echo [УСПЕХ] Сборка плагина успешно завершена!
    del main.obj ServerConnect.lib ServerConnect.exp
)
pause
