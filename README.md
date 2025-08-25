[![构建状态](https://ci.appveyor.com/api/projects/status/31l6ynm0a1fhr2vs/branch/master?svg=true)](https://ci.appveyor.com/project/mrpond/blockthespot/branch/master)  [![Discord](https://discord.com/api/guilds/807273906872123412/widget.png)](https://discord.gg/eYudMwgYtY) <img src="https://img.shields.io/github/downloads/mrpond/blockthespot/total.svg" />

<center>
    <h1 align="center">BlockTheSpot</h1>
    <h4 align="center">一个适用于 <strong>Spotify Windows (64位)</strong> 的多功能广告拦截器和跳过器</h4>
    <h5 align="center">请通过购买高级会员来支持 Spotify</h5>
    <p align="center">
        <strong>最后更新日期:</strong> 2025年8月25日<br>
        <strong>最后测试版本:</strong> Spotify for Windows (64 bit) 1.2.71.421.g794ff5e5
    </p>
</center>

### Windows Defender 病毒警告问题：
* 代码在 Github 上，任何人都可以查看。
* BTS 使用 Appveyor，https://www.appveyor.com/。
* Github 上的任何代码更改，Appveyor 都会进行构建并在 Github 上发布。
* 误报是可能发生的。但不要完全相信我的话，尝试自己编译 BTS 并与发布版本进行比较来验证。

### 功能：
* 解锁大部分高级会员功能，但不支持下载和“你的DJ”功能
* 可以在 Spotify 更新后继续使用。不再需要在每次更新后重新打补丁。

#### 开发者模式中的实验性功能
- 点击 Spotify 左上角的两个点 > 开发者(Develop) > 显示调试窗口(Show debug window)。试着玩玩这些选项。
- 实时按需启用/禁用功能。
- 选择旧/新主题(YLX)。
- 启用右侧边栏。
- 隐藏顶部栏的升级按钮。

:warning: 此模组仅适用于 Windows 的 [**桌面应用版本**](https://www.spotify.com/download/windows/)，**不适用于微软商店版本**。

### 安装/更新：
* 只需下载并运行 [BlockTheSpot.bat](https://raw.githack.com/mrpond/BlockTheSpot/master/BlockTheSpot.bat)

或者

#### 通过 PowerShell 全自动安装
```powershell
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-Expression "& { $(Invoke-WebRequest -UseBasicParsing 'https://raw.githubusercontent.com/Maolaohei/BlockTheSpot/master/install.ps1') } -UninstallSpotifyStoreEdition -UpdateSpotify"
```

#### Manual installation/update

1.浏览到您的 Spotify 安装文件夹 %APPDATA%\Spotify
2.从 发布页面 下载 chrome_elf.zip
3.将 dpapi.dll 和 config.ini 解压到 Spotify 目录。
4.从 Github 下载最新的 blockthespot_settings.json 到 Spotify 目录。  
### Uninstall:
* 只需运行 uninstall.bat(https://raw.githack.com/mrpond/BlockTheSpot/master/uninstall.bat)
or
* 从 Spotify 目录中删除 dpapi.dll 和 config.ini。
or
* 重新安装 Spotify

#### BlockTheSpot 与 Spicetify 一起安装/更新:

* Just download and run [BlockTheSpot + Spicetify.bat](https://raw.githack.com/mrpond/BlockTheSpot/master/BlockTheSpot%20%2B%20Spicetify.bat) then answer the prompts when given

### BlockTheSpot with Spicetify Uninstall:

```powershell
spicetify restore
rmdir -r -fo $env:APPDATA\spicetify
rmdir -r -fo $env:LOCALAPPDATA\spicetify
rm -fo $env:APPDATA\spotify\dpapi.dll
rm -fo $env:APPDATA\spotify\config.ini
```

### Disabling Automatic Updates

The automatic update feature is enabled by default. To disable it:

1. Navigate to the directory where Spotify is installed: `%APPDATA%\Spotify`.
2. Open the `config.ini` file.
3. Set `Enable_Auto_Update` to `0` under the `[Config]` section.
4. Save your changes and close the file.

Automatic updates will now be disabled. If you wish to update, you'll need to do so manually.

### Additional Notes:

* Installation script automatically detects if your Spotify client version is supported, or not. If the version is not supported, you will be prompted to update your Spotify client. To enforce client update, supply an optional parameter `UpdateSpotify` to the installation script. 
* [Spicetify](https://github.com/khanhas/spicetify-cli) users will need to reapply BlockTheSpot after applying a Spicetify themes/patches.
* If the automatic install/uninstall scripts do not work, please contact [Nuzair46](https://github.com/Nuzair46).
* For more support and discussions, join our [Discord server](https://discord.gg/eYudMwgYtY).





