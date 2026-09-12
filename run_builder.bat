@echo off
setlocal EnableDelayedExpansion

set "BUILDER_EXE=%~dp0builder.exe"
set "RED=[91m"
set "GREEN=[92m"
set "YELLOW=[93m"
set "CYAN=[96m"
set "RESET=[0m"

if not exist "!BUILDER_EXE!" (
    echo !RED![ERROR] builder.exe not found at !BUILDER_EXE!!RESET!
    echo !YELLOW!Please compile CBuilder first.!RESET!
    pause
    exit /b 1
)

:MAIN_MENU
cls
echo !CYAN!=======================================!RESET!
echo !CYAN!         CBuilder Interactive          !RESET!
echo !CYAN!           By: Anthony Gaius           !RESET!
echo !CYAN!=======================================!RESET!
echo.
echo Please select an action:
echo !GREEN!1.!RESET! Compile and Run a single C file
echo !GREEN!2.!RESET! Build a project directory
echo !GREEN!3.!RESET! Create a new C project
echo !GREEN!4.!RESET! Exit
echo.

set "action="
set /p "action=Enter your choice (!GREEN!1-4!RESET!): "

if "!action!"=="1" goto OPTION_1
if "!action!"=="2" goto OPTION_2
if "!action!"=="3" goto OPTION_3
if "!action!"=="4" exit /b 0

echo.
echo !RED![ERROR] Invalid choice. Please try again.!RESET!
pause
goto MAIN_MENU

:OPTION_1
echo.
set "file_path="
set /p "file_path=Enter the path to the .c file: "
if "!file_path!"=="" (
    echo !RED![ERROR] Path cannot be empty.!RESET!
    pause
    goto MAIN_MENU
)

set "file_path=!file_path:"=!"

if not exist "!file_path!" (
    echo !RED![ERROR] File not found: "!file_path!"!RESET!
    pause
    goto MAIN_MENU
)

echo.
echo !YELLOW!Running: builder.exe compile "!file_path!" --run!RESET!
"!BUILDER_EXE!" compile "!file_path!" --run
echo.
pause
goto MAIN_MENU

:OPTION_2
echo.
set "proj_path="
set /p "proj_path=Enter the path to the project directory (leave empty for current dir): "

if "!proj_path!"=="" set "proj_path=."

set "proj_path=!proj_path:"=!"

if not exist "!proj_path!\*" (
    echo !RED![ERROR] Directory not found: "!proj_path!"!RESET!
    pause
    goto MAIN_MENU
)

echo.
set "run_opt="
set /p "run_opt=Do you want to run the project after building? (Y/N): "

echo.
if /I "!run_opt!"=="Y" (
    echo !YELLOW!Running: builder.exe run "!proj_path!"!RESET!
    "!BUILDER_EXE!" run "!proj_path!"
) else (
    echo !YELLOW!Running: builder.exe build "!proj_path!"!RESET!
    "!BUILDER_EXE!" build "!proj_path!"
)
echo.
pause
goto MAIN_MENU

:OPTION_3
echo.
set "proj_name="
set /p "proj_name=Enter the name/path for the new project: "

if "!proj_name!"=="" (
    echo !RED![ERROR] Project name cannot be empty.!RESET!
    pause
    goto MAIN_MENU
)

set "proj_name=!proj_name:"=!"

echo.
echo !YELLOW!Running: builder.exe new "!proj_name!"!RESET!
"!BUILDER_EXE!" new "!proj_name!"
echo.
pause
goto MAIN_MENU
