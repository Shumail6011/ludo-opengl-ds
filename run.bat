@echo off
echo =============================
echo   Building OpenGL Project...
echo =============================

REM Yahan maine -lopengl32 -lgdi32 aur -lwinmm add kar diya hai
g++ -g --std=c++17 -I"include" -L"lib" "src\main.cpp" "src\glad.c" -lglfw3dll -lopengl32 -lgdi32 -lwinmm -o "bin\main.exe"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed! Check the errors above.
    pause
    exit /b 1
)

REM Copy DLL so the exe can find it
copy /y "lib\glfw3.dll" "bin\glfw3.dll" >nul

echo.
echo [SUCCESS] Build complete. Running game...
echo =============================
echo.

"bin\main.exe"

echo.
echo Game exited.
pause