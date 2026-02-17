@echo off
chcp 65001 >nul
title BlockTheSpot 安装脚本 (改进版)
echo ==========================================
echo    BlockTheSpot 安装脚本
echo    改进版 - 带故障排查
echo ==========================================
echo.

set "SPOTIFY_PATH=%APPDATA%\Spotify"
set "SAFE_MODE=0"

:menu
echo 请选择安装模式:
echo   1. 标准安装 (全功能)
echo   2. 安全安装 (禁用可能导致黑屏的功能)
echo   3. 仅核心功能 (仅广告拦截，最稳定)
echo   4. 卸载
echo   5. 诊断问题
echo.
set /p choice="请输入选项 (1-5): "

if "%choice%"=="1" goto standard
if "%choice%"=="2" goto safe
if "%choice%"=="3" goto minimal
if "%choice%"=="4" goto uninstall
if "%choice%"=="5" goto diagnose
goto menu

:standard
echo.
echo [标准安装模式]
echo 安装全功能版本...
call :install config.ini
goto end

:safe
echo.
echo [安全安装模式]
echo 禁用 CSS 修改等可能导致黑屏的功能...
call :install config_safe.ini
echo.
echo 已使用安全模式配置。
echo 如果仍有问题，请选择"仅核心功能"。
goto end

:minimal
echo.
echo [仅核心功能模式]
echo 仅启用广告拦截，禁用所有修改功能...
if not exist "%SPOTIFY_PATH%" (
    echo 错误: 找不到 Spotify 安装目录
    pause
    exit /b 1
)
copy /y chrome_elf.dll "%SPOTIFY_PATH%\chrome_elf.dll" >nul
copy /y blockthespot.dll "%SPOTIFY_PATH%\blockthespot.dll" >nul
(
echo [Log]
echo Level=1
echo [Developer]
echo Enable=0
echo [URL_block]
echo Enable=1
echo [Buffer_modify]
echo Enable=0
echo [Homepage_vbar]
echo Enable=0
echo [xpui-snapshot.js]
echo adsEnabled=1
echo ishptohidden=0
echo skipsentry=0
echo disable_metric=0
echo premium_btn_begin=0
echo premium_btn_end=0
echo [xpui-pip-mini-player.js]
echo miniplayer_begin=0
echo miniplayer_end=0
) > "%SPOTIFY_PATH%\config.ini"
echo 已安装（仅核心功能）
goto end

:uninstall
echo.
echo [卸载]
if exist "%SPOTIFY_PATH%\chrome_elf_required.dll" (
    del /f "%SPOTIFY_PATH%\chrome_elf.dll" 2>nul
    ren "%SPOTIFY_PATH%\chrome_elf_required.dll" chrome_elf.dll
    del /f "%SPOTIFY_PATH%\blockthespot.dll" 2>nul
    del /f "%SPOTIFY_PATH%\config.ini" 2>nul
    echo 已卸载 BlockTheSpot
) else (
    echo 未找到 BlockTheSpot 或已卸载
)
goto end

:diagnose
echo.
echo [诊断]
echo 检查 Spotify 版本...
if exist "%SPOTIFY_PATH%\Spotify.exe" (
    for /f "tokens=*" %%a in ('"%SPOTIFY_PATH%\Spotify.exe" --version 2^>^&1') do (
        echo Spotify 版本: %%a
    )
) else (
    echo 错误: 找不到 Spotify.exe
)
echo.
echo 检查配置文件...
if exist "%SPOTIFY_PATH%\config.ini" (
    echo config.ini: 存在
    findstr "Enable" "%SPOTIFY_PATH%\config.ini" | head -10
) else (
    echo config.ini: 不存在
)
echo.
echo 检查日志文件...
if exist "%SPOTIFY_PATH%\debug.log" (
    echo debug.log: 存在
    echo 最后10行日志:
    type "%SPOTIFY_PATH%\debug.log" | tail -10
) else (
    echo debug.log: 不存在（请在 config.ini 中设置 Level=1）
)
goto end

:install
set "CONFIG_FILE=%~1"
if not exist "%SPOTIFY_PATH%" (
    echo 错误: 找不到 Spotify 安装目录: %SPOTIFY_PATH%
    pause
    exit /b 1
)

if not exist "%SPOTIFY_PATH%\chrome_elf.dll" (
    echo 错误: 找不到 chrome_elf.dll
    echo 请先安装 Spotify 桌面版
    pause
    exit /b 1
)

echo 备份原始文件...
if not exist "%SPOTIFY_PATH%\chrome_elf_required.dll" (
    copy /y "%SPOTIFY_PATH%\chrome_elf.dll" "%SPOTIFY_PATH%\chrome_elf_required.dll" >nul
    echo 已备份 chrome_elf.dll
)

echo 安装 BlockTheSpot...
copy /y chrome_elf.dll "%SPOTIFY_PATH%\chrome_elf.dll" >nul
copy /y blockthespot.dll "%SPOTIFY_PATH%\blockthespot.dll" >nul
copy /y "%CONFIG_FILE%" "%SPOTIFY_PATH%\config.ini" >nul

echo.
echo 安装完成！
echo 请重启 Spotify。
exit /b 0

:end
echo.
pause
