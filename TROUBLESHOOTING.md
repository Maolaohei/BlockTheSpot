# BlockTheSpot 黑屏问题排查指南

## 问题现象
- Spotify 启动后界面全黑
- 重新安装后正常，用一段时间后又黑屏
- 有声音但无画面

## 原因分析

### 1. Pattern 签名过期（最常见）
Spotify 更新后，代码字节变化，旧的 signature 匹配失败。

**症状：**
- 黑屏或闪退
- 日志中出现 `FindPattern failed`

**解决：**
- 更新到最新版 BlockTheSpot
- 或暂时禁用所有 hook，仅用基础功能

### 2. CSS/JS 注入失败
修改 `xpui-snapshot.js` 等文件时出错。

**症状：**
- 界面元素缺失
- 部分功能正常，部分黑屏

**解决：**
- 使用 `config_safe.ini` 禁用 CSS 修改
- 或等待开发者更新 pattern

### 3. 内存保护冲突
Spotify 的代码完整性检查与 hook 冲突。

**症状：**
- 随机黑屏
- 特定操作后黑屏（如切换页面）

**解决：**
- 添加延迟加载（见 loader_improved.cpp）
- 或禁用开发者模式

## 快速修复步骤

### 方法 1: 安全模式（推荐）
1. 备份当前的 `config.ini`
2. 复制 `config_safe.ini` 为 `config.ini`
3. 重启 Spotify

### 方法 2: 完全禁用
1. 删除 `%APPDATA%\Spotify\blockthespot.dll`
2. 重命名 `chrome_elf.dll` 为 `blockthespot.dll`
3. 重启 Spotify（此时无广告拦截，但稳定）

### 方法 3: 重新安装
1. 完全卸载 Spotify
2. 删除 `%APPDATA%\Spotify` 文件夹
3. 重新安装 Spotify
4. 重新安装 BlockTheSpot

## 长期解决方案

### 自动更新检查
建议添加版本检查，不匹配时自动跳过 hooks：

```cpp
// 在 loader.cpp 中添加
bool check_spotify_version() {
    // 检查 Spotify 版本是否在支持列表
    // 不支持时优雅降级，不执行 hooks
}
```

### 模块化加载
将功能拆分为独立模块，失败不影响其他：
- URL 拦截模块（核心，必须成功）
- CSS 修改模块（可选，失败不崩溃）
- 开发者模式（可选，失败不崩溃）

## 日志分析

启用日志（`config.ini` 中 `Level=1`），查看：

```
[ERROR] FindPattern failed: adsEnabled  → Pattern 过期
[ERROR] Failed to load libcef.dll       → 加载顺序问题
[INFO]  Partial initialization         → 部分功能禁用（正常）
```

## 联系开发者

如果以上方法无效：
1. 收集 `%APPDATA%\Spotify\debug.log`
2. 提供 Spotify 版本号（设置 > 关于）
3. 在 GitHub Issues 提交问题
