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
echo [ПОМИЛКА] Компилятор cl.exe не знайдено. Установiть Visual Studio C++.
pause
exit

:compile
echo Компиляцiя ServerConnect.asi...
rem Додано прапори /D_CRT_SECURE_NO_WARNINGS для відключення суворих попереджень розробника
cl.exe /LD /O2 /EHsc /D_CRT_SECURE_NO_WARNINGS main.cpp /link /OUT:ServerConnect.asi

if exist ServerConnect.asi (
    echo.
    echo [УСПЕХ] Сборка плагина ServerConnect.asi устiшно завершена!
    rem Очищення тимчасового сміття компілятора
    if exist main.obj del main.obj
    if exist ServerConnect.lib del ServerConnect.lib
    if exist ServerConnect.exp del ServerConnect.exp
) else (
    echo.
    echo [ПОМИЛКА] Збiрка завершилась невдало. Перевiрте логи вище.
)
pause
