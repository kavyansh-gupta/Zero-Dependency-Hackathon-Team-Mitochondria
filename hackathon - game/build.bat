@echo off
echo Building game...

g++ -std=c++17 -Isrc ^
src/main.cpp ^
src/Game.cpp ^
src/GameState.cpp ^
src/Input.cpp ^
src/Renderer.cpp ^
-o game.exe

if %errorlevel% neq 0 (
    echo.
    echo Build failed.
    pause
    exit /b %errorlevel%
)

echo.
echo Build complete. Run game.exe to start.
pause