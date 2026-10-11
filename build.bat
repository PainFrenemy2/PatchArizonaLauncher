@echo off
cls
echo =======================================================
echo          PAINFRENEMY SOFTWARE LABORATORY
echo          ASI Builder pipeline (Safe Mode)
echo =======================================================
echo.

echo [STATUS] Searching for Visual Studio Build Tools environment...

rem 1. Проверяем путь для 64-битной системы (самый вероятный в вашем случае)
if exist "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
    goto compile
)

rem 2. Резервная проверка старого пути (x86)
if exist "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars32.bat"
    goto compile
)

rem 3. Проверка пути для обычной Community версии (на всякий случай)
if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat" (
    call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars32.bat"
    goto compile
)

echo [ERROR] MSVC Compiler batch file (vcvars32.bat) was not found!
goto end

:compile
echo [STATUS] Compiling Windows resource tables (resource.rc)...
rc.exe /r /fo resource.res resource.rc

echo [STATUS] Executing high-performance C++ compilation (/O2)...
cl.exe /LD /O2 /EHsc /D_CRT_SECURE_NO_WARNINGS main.cpp resource.res /link /OUT:ConnectPatch.asi

if exist ConnectPatch.asi (
    echo [STATUS] Cleaning up temporary compiler artifacts...
    if exist main.obj del main.obj
    if exist resource.res del resource.res
    if exist ConnectPatch.lib del ConnectPatch.lib
    if exist ConnectPatch.exp del ConnectPatch.exp
    echo.
    echo =======================================================
    echo [SUCCESS] ConnectPatch.asi compiled successfully!
    echo =======================================================
) else (
    echo.
    echo [ERROR] Compilation failed. Check errors above.
)

:end
echo.
echo Press any key to exit...
pause >nul
