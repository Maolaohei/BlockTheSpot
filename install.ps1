#Requires -RunAsAdministrator

<#
.SYNOPSIS
    BlockTheSpot Improved - 一键安装脚本 (PowerShell)
    Fork: https://github.com/Maolaohei/BlockTheSpot
    
.DESCRIPTION
    自动下载并安装改进版 BlockTheSpot，包含防黑屏配置
    
.PARAMETER UninstallSpotifyStoreEdition
    卸载 Microsoft Store 版 Spotify（如果已安装）
    
.PARAMETER UpdateSpotify
    更新 Spotify 到最新版
    
.PARAMETER SafeMode
    使用安全模式配置（防黑屏，默认启用）
    
.PARAMETER CoreOnly
    仅安装核心功能（最稳定）
    
.EXAMPLE
    # 标准安装（安全模式）
    iwr -useb https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/install.ps1 | iex
    
    # 卸载 Store 版并更新 Spotify 后安装
    iwr -useb https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/install.ps1 | iex -UninstallSpotifyStoreEdition -UpdateSpotify
    
    # 仅核心功能（最稳定）
    iwr -useb https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/install.ps1 | iex -CoreOnly
#>

param (
    [switch]$UninstallSpotifyStoreEdition,
    [switch]$UpdateSpotify,
    [switch]$SafeMode = $true,  # 默认启用安全模式
    [switch]$CoreOnly
)

$ErrorActionPreference = "Stop"

# 设置 TLS 1.2
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

# 颜色输出
function Write-Info($msg) { Write-Host "[INFO] $msg" -ForegroundColor Cyan }
function Write-Success($msg) { Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warning($msg) { Write-Host "[WARN] $msg" -ForegroundColor Yellow }
function Write-Error($msg) { Write-Host "[ERROR] $msg" -ForegroundColor Red }

# Spotify 路径
$SpotifyPath = "$env:APPDATA\Spotify"
$SpotifyExe = "$SpotifyPath\Spotify.exe"

Write-Host ""
Write-Host "========================================" -ForegroundColor Magenta
Write-Host "  BlockTheSpot Improved 一键安装器" -ForegroundColor Magenta
Write-Host "  Fork: Maolaohei/BlockTheSpot" -ForegroundColor Magenta
Write-Host "========================================" -ForegroundColor Magenta
Write-Host ""

# 检查管理员权限
if (-not ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "请以管理员权限运行 PowerShell"
    exit 1
}

# 1. 卸载 Store 版 Spotify
if ($UninstallSpotifyStoreEdition) {
    Write-Info "检查 Microsoft Store 版 Spotify..."
    $storeSpotify = Get-AppxPackage -Name "SpotifyAB.SpotifyMusic" -ErrorAction SilentlyContinue
    if ($storeSpotify) {
        Write-Warning "发现 Store 版 Spotify，正在卸载..."
        Remove-AppxPackage -Package $storeSpotify.PackageFullName
        Write-Success "Store 版已卸载"
    } else {
        Write-Info "未找到 Store 版 Spotify"
    }
}

# 2. 更新 Spotify
if ($UpdateSpotify) {
    Write-Info "检查 Spotify 更新..."
    if (Test-Path $SpotifyExe) {
        # 停止 Spotify
        Get-Process -Name "Spotify" -ErrorAction SilentlyContinue | Stop-Process -Force
        Start-Sleep -Seconds 2
        
        # 下载最新版 Spotify
        $SpotifyInstaller = "$env:TEMP\SpotifySetup.exe"
        Write-Info "下载 Spotify 安装器..."
        Invoke-WebRequest -Uri "https://download.scdn.co/SpotifySetup.exe" -OutFile $SpotifyInstaller -UseBasicParsing
        
        Write-Info "运行 Spotify 安装器..."
        Start-Process -FilePath $SpotifyInstaller -Wait
        Remove-Item $SpotifyInstaller -ErrorAction SilentlyContinue
        Write-Success "Spotify 已更新"
    } else {
        Write-Error "未找到 Spotify，请先安装桌面版: https://www.spotify.com/download/windows/"
        exit 1
    }
}

# 3. 检查 Spotify 是否存在
if (-not (Test-Path $SpotifyExe)) {
    Write-Error "未找到 Spotify 桌面版"
    Write-Host "请从以下地址下载安装:"
    Write-Host "https://www.spotify.com/download/windows/" -ForegroundColor Yellow
    exit 1
}

Write-Info "Spotify 路径: $SpotifyPath"

# 4. 停止 Spotify
Write-Info "停止 Spotify 进程..."
Get-Process -Name "Spotify" -ErrorAction SilentlyContinue | Stop-Process -Force
Start-Sleep -Seconds 2

# 5. 备份原始 chrome_elf.dll
$ChromeElf = "$SpotifyPath\chrome_elf.dll"
$ChromeElfBackup = "$SpotifyPath\chrome_elf_required.dll"

if (Test-Path $ChromeElf) {
    if (-not (Test-Path $ChromeElfBackup)) {
        Write-Info "备份原始 chrome_elf.dll..."
        Copy-Item $ChromeElf $ChromeElfBackup -Force
        Write-Success "已备份"
    } else {
        Write-Info "备份已存在，跳过"
    }
} else {
    Write-Error "未找到 chrome_elf.dll，请确认 Spotify 安装正确"
    exit 1
}

# 6. 下载 BlockTheSpot 文件
$BaseUrl = "https://github.com/Maolaohei/BlockTheSpot/raw/master"
$Files = @(
    @{ Name = "chrome_elf.dll"; OutFile = "$SpotifyPath\chrome_elf.dll" }
    @{ Name = "blockthespot.dll"; OutFile = "$SpotifyPath\blockthespot.dll" }
)

Write-Info "下载 BlockTheSpot 文件..."
foreach ($File in $Files) {
    $Url = "$BaseUrl/$($File.Name)"
    Write-Info "下载 $($File.Name)..."
    try {
        Invoke-WebRequest -Uri $Url -OutFile $File.OutFile -UseBasicParsing
        Write-Success "$($File.Name) 下载完成"
    } catch {
        Write-Warning "$($File.Name) 下载失败，尝试备用源..."
        # 备用：从上游下载
        $BackupUrl = "https://github.com/mrpond/BlockTheSpot/raw/master/$($File.Name)"
        Invoke-WebRequest -Uri $BackupUrl -OutFile $File.OutFile -UseBasicParsing
        Write-Success "$($File.Name) 从备用源下载完成"
    }
}

# 7. 下载配置
$configUrl = if ($CoreOnly) {
    "https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/config_safe.ini"
} elseif ($SafeMode) {
    "https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/config_safe.ini"
} else {
    "https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/config.ini"
}

Write-Info "下载配置文件..."
$configFile = "$SpotifyPath\config.ini"
Invoke-WebRequest -Uri $configUrl -OutFile $configFile -UseBasicParsing

if ($CoreOnly) {
    # 仅核心功能：进一步简化配置
    @"
[Log]
Level=1

[Developer]
Enable=0

[URL_block]
Enable=1
1=/ads/
2=/ad-logic/
3=/gabo-receiver-service/
4=/desktop-update/

[LIBCEF]
Block_crashpad=1

[Buffer_modify]
Enable=0

[Homepage_vbar]
Enable=0

[xpui-snapshot.js]
adsEnabled=1
ishptohidden=0
skipsentry=0
disable_metric=0
premium_btn_begin=0
premium_btn_end=0

[xpui-pip-mini-player.js]
miniplayer_begin=0
miniplayer_end=0
"@ | Set-Content $configFile
    Write-Success "已应用核心功能配置（最稳定）"
} elseif ($SafeMode) {
    Write-Success "已应用安全模式配置（防黑屏）"
} else {
    Write-Success "已应用标准配置"
}

# 8. 清理旧的 dpapi.dll
$DpapiDll = "$SpotifyPath\dpapi.dll"
if (Test-Path $DpapiDll) {
    Remove-Item $DpapiDll -Force
    Write-Info "清理旧的 dpapi.dll"
}

# 9. 完成
Write-Host ""
Write-Success "BlockTheSpot 安装完成！"
Write-Host ""
Write-Host "配置模式: $(if($CoreOnly){'核心功能'}elseif($SafeMode){'安全模式'}else{'标准模式'})" -ForegroundColor Cyan
Write-Host ""
Write-Host "现在可以启动 Spotify 了" -ForegroundColor Green
Write-Host ""
Write-Host "如果仍遇到黑屏：" -ForegroundColor Yellow
Write-Host "  1. 删除 %APPDATA%\Spotify\config.ini"
Write-Host "  2. 重新运行本脚本加 -CoreOnly 参数"
Write-Host ""
Write-Host "故障排查: https://github.com/Maolaohei/BlockTheSpot/blob/master/TROUBLESHOOTING.md"
Write-Host ""

# 10. 询问是否启动 Spotify
$startSpotify = Read-Host "是否现在启动 Spotify? (y/n)"
if ($startSpotify -eq 'y' -or $startSpotify -eq 'Y') {
    Start-Process $SpotifyExe
}
