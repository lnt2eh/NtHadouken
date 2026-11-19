@echo off
color 0a
chcp 65001

:: Check for administrative privileges
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting Administrator privileges...
    powershell -Command "Start-Process cmd -ArgumentList '/c %~s0' -Verb RunAs"
    exit
)

:MENU
cls
echo d8b   db d888888b db   db  .d8b.  d8888b.  .d88b.  db    db db   dD d88888b d8b   db
echo 888o  88 `~~88~~' 88   88 d8' `8b 88  `8D .8P  Y8. 88    88 88 ,8P' 88'     888o  88
echo 88V8o 88    88    88ooo88 88ooo88 88   88 88    88 88    88 88,8P   88ooooo 88V8o 88
echo 88 V8o88    88    88~~~88 88~~~88 88   88 88    88 88    88 88`8b   88~~~~~ 88 V8o88
echo 88  V888    88    88   88 88   88 88  .8D `8b  d8' 88b  d88 88 `88. 88.     88  V888
echo VP   V8P    YP    YP   YP YP   YP Y8888D'  `Y88P'  ~Y8888P' YP   YD Y88888P VP   V8P
echo.
echo Select an option:
echo 1. Enable Debug Mode
echo 2. List Installed Drivers
echo 3. Check for Corrupted Files
echo 4. Collect System Information
echo 5. Exit
set /p option=Select an option: 

if "%option%"=="1" goto MODE_DEBUG
if "%option%"=="2" goto VERIFY_DRIVERS
if "%option%"=="3" goto VERIFY_FILES
if "%option%"=="4" goto COLLECT_SYSTEM_INFO
if "%option%"=="5" exit

echo Invalid option. Please try again.
pause
goto MENU

:MODE_DEBUG
echo Enabling Debug Mode...
bcdedit /set debug on
bcdedit /set testsigning off
echo Debug Mode enabled successfully!
pause
goto MENU

:VERIFY_DRIVERS
echo Listing installed drivers...
set "OUTPUT_FILE=%USERPROFILE%\Documents\installed_drivers.html"

(
echo ^<html^>
echo ^<head^>^<title^>Installed Drivers^</title^>^</head^>
echo ^<body^>
echo ^<h1^>Installed Drivers^</h1^>
echo ^<pre^>
) > "%OUTPUT_FILE%"

driverquery >> "%OUTPUT_FILE%"

(
echo ^</pre^>
echo ^</body^>
echo ^</html^>
) >> "%OUTPUT_FILE%"

echo The driver list was saved to: "%OUTPUT_FILE%"
pause
goto MENU

:VERIFY_FILES
echo Checking for corrupted files...
powershell -Command "sfc /scannow | Tee-Object -FilePath '%USERPROFILE%\Documents\corrupted_files.txt'"
echo The corrupted files report was saved to: "%USERPROFILE%\Documents\corrupted_files.txt"
pause
goto MENU

:COLLECT_SYSTEM_INFO
echo Collecting system information...
set "OUTPUT_FILE=%USERPROFILE%\Documents\system_information.html"

(
echo ^<html^>
echo ^<head^>^<title^>System Information^</title^>^</head^>
echo ^<body^>
echo ^<h1^>System Information^</h1^>
echo ^<pre^>
) > "%OUTPUT_FILE%"

systeminfo >> "%OUTPUT_FILE%"
(
echo.
echo Disks:
) >> "%OUTPUT_FILE%"
wmic logicaldisk get caption, description, size, freespace >> "%OUTPUT_FILE%"

(
echo.
echo Installed Drivers:
) >> "%OUTPUT_FILE%"
driverquery >> "%OUTPUT_FILE%"

(
echo ^</pre^>
echo ^</body^>
echo ^</html^>
) >> "%OUTPUT_FILE%"

echo System information was saved to: "%OUTPUT_FILE%"
pause
goto MENU
