@echo off
setlocal
chcp 65001 >nul

echo [INFO] PHP 설치 확인 중...

:: 1. 시스템 PATH 확인
where php >nul 2>nul
if %errorlevel% equ 0 (
    echo [OK] 시스템 PATH에서 PHP를 찾았습니다.
    goto :START_SERVER
)

:: 2. 일반적인 설치 경로 확인 (Winget Fuzzy Search)
echo [INFO] 일반 경로 검색 중...

:: a) Winget Packages 패턴 검색
if exist "%LOCALAPPDATA%\Microsoft\WinGet\Packages\" (
    for /d %%D in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\PHP.*") do (
        if exist "%%~D\php.exe" (
            set "PHP_PATH=%%~D"
            goto :FOUND_MANUAL
        )
    )
)

:: b) C:\Program Files\PHP
if exist "C:\Program Files\PHP\php.exe" (
    set "PHP_PATH=C:\Program Files\PHP"
    goto :FOUND_MANUAL
)

:: c) C:\tools\php (Scoop 등)
if exist "C:\tools\php\php.exe" (
    set "PHP_PATH=C:\tools\php"
    goto :FOUND_MANUAL
)

:: d) xampp
if exist "C:\xampp\php\php.exe" (
    set "PHP_PATH=C:\xampp\php"
    goto :FOUND_MANUAL
)

:: 못 찾음
echo.
echo [ERROR] PHP를 찾을 수 없습니다.
echo.
echo [CHECK] 다음 경로들을 확인해보았으나 실패했습니다:
echo  - 시스템 PATH
echo  - %LOCALAPPDATA%\Microsoft\WinGet\Packages\PHP.*
echo  - C:\Program Files\PHP
echo  - C:\tools\php
echo  - C:\xampp\php
echo.
echo 1. 터미널 창(PowerShell)을 완전히 닫고 다시 열어보세요.
echo 2. PC를 재부팅하면 해결될 수 있습니다.
echo.
pause
exit /b 1

:FOUND_MANUAL
echo [OK] 찾았습니다: %PHP_PATH%
set "PATH=%PHP_PATH%;%PATH%"

:START_SERVER
echo.
php -v
echo.
echo [INFO] 로컬 개발 서버를 시작합니다...
echo [INFO] http://localhost:9900
echo.

php.exe -S localhost:9900 -t www 2>&1 | findstr /V "poll get_latest_call get_cti_logs Accepted Closing"

endlocal
