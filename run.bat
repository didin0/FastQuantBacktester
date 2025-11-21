@echo off
echo Starting FastQuantBacktester...

if not exist build (
    echo Creating build directory...
    mkdir build
)

cd build
echo Configuring CMake...
cmake ..
if %errorlevel% neq 0 (
    echo CMake configuration failed.
    pause
    exit /b %errorlevel%
)

echo Building project...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed.
    pause
    exit /b %errorlevel%
)

echo Starting Server...
if exist Release\fastquant_server.exe (
    cd Release
    fastquant_server.exe
) else (
    if exist fastquant_server.exe (
        fastquant_server.exe
    ) else (
        echo Could not find fastquant_server.exe.
        pause
        exit /b 1
    )
)
pause
