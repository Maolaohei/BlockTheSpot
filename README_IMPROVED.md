# BlockTheSpot 改进版

这个 fork 包含对原项目的改进，主要解决**黑屏问题**和**版本兼容性**。

## 🚀 主要改进

### 1. 延迟加载机制
**问题**: Hook 在 CEF 未初始化完成时执行，导致渲染崩溃。  
**解决**: 添加 500ms 延迟 + 重试机制，确保 DLL 完全加载。

```cpp
// loader.cpp 改进
Sleep(300);  // 初始延迟
// ... 加载 DLL ...
Sleep(200);  // 额外延迟，确保 CEF 初始化
```

### 2. 版本检查
**问题**: Spotify 更新后 pattern 失效，导致崩溃。  
**解决**: 启动时检查版本，不支持的版本自动降级到安全模式。

```cpp
if (!is_version_supported(ver)) {
    log_info("Version not supported, running in safe mode");
    // 只启用基本功能
}
```

### 3. 安全 Hook
**问题**: 单个 hook 失败导致整个程序崩溃。  
**解决**: 每个 hook 包装在 try-except 中，失败不崩溃。

```cpp
__try {
    hook_developer_mode();
} __except (EXCEPTION_EXECUTE_HANDLER) {
    log_debug("Developer mode hook failed, continuing");
}
```

### 4. Pattern 备用机制
**问题**: Pattern 签名在 Spotify 更新后失效。  
**解决**: 每个 pattern 有主签名 + 备用签名，都失败时使用模糊匹配。

```cpp
// 先尝试主 signature
if (!match) {
    // 尝试备用 signature
    if (!match) {
        // 模糊匹配（允许少量字节不同）
    }
}
```

### 5. 模块化加载
**问题**: CSS 修改失败导致整个界面黑屏。  
**解决**: CSS 修改单独处理，失败只影响美观，不影响功能。

## 📦 文件说明

| 文件 | 说明 |
|------|------|
| `loader_improved.cpp` | 改进的加载器，包含上述所有改进 |
| `pattern_safe.cpp` | 安全的 pattern 匹配，带备用机制 |
| `config_safe.ini` | 安全模式配置，禁用可能导致问题的功能 |
| `install.bat` | 改进的安装脚本，支持 3 种安装模式 |
| `sync-upstream.sh` | 自动同步上游更新脚本 |

## 🛠️ 安装方法

### 方法 1: 标准安装（推荐）
```batch
install.bat
# 选择选项 1: 标准安装
```

### 方法 2: 安全安装（如果黑屏）
```batch
install.bat
# 选择选项 2: 安全安装
# 这会禁用 CSS 修改等可能导致黑屏的功能
```

### 方法 3: 仅核心功能（最稳定）
```batch
install.bat
# 选择选项 3: 仅核心功能
# 只拦截广告，不修改界面
```

## 🔧 故障排查

### 问题: 安装后 Spotify 黑屏

**解决步骤:**

1. **使用安全模式**
   ```batch
   install.bat
   # 选择 2: 安全安装
   ```

2. **或手动修改 config.ini**
   ```ini
   [Buffer_modify]
   Enable=0  # 禁用 JS buffer 修改
   
   [Homepage_vbar]
   Enable=0  # 禁用 CSS 修改
   ```

3. **完全禁用**
   ```batch
   install.bat
   # 选择 4: 卸载
   ```

### 问题: Spotify 更新后又黑屏

**原因**: Pattern 签名过期  
**解决**:
```bash
# 同步上游更新
git remote add upstream https://github.com/mrpond/BlockTheSpot.git
git fetch upstream
git merge upstream/master
# 重新编译
```

或等待此 fork 更新。

### 问题: 部分功能失效（如开发者模式）

**原因**: 该功能的 hook 在新版本 Spotify 中失效  
**解决**: 在 `loader.cpp` 中会显示哪些功能失效，不影响其他功能。

## 🔨 编译方法

```bash
# 克隆仓库
git clone https://github.com/Maolaohei/BlockTheSpot.git
cd BlockTheSpot

# 打开解决方案
start BlockTheSpot.slnx

# 或命令行编译
msbuild BlockTheSpot.slnx /p:Configuration=Release /p:Platform=x64
```

## 📊 与原项目对比

| 特性 | 原项目 | 此改进版 |
|------|--------|----------|
| 延迟加载 | ❌ | ✅ 500ms |
| 版本检查 | ❌ | ✅ 自动降级 |
| Hook 失败处理 | ❌ 崩溃 | ✅ 安全跳过 |
| Pattern 备用 | ❌ | ✅ 主+备+模糊 |
| 安装模式 | ❌ 单一 | ✅ 3种模式 |
| 调试日志 | ❌ 有限 | ✅ 详细 |

## 📝 更新日志

### v1.1 (改进版)
- 添加延迟加载机制
- 添加版本检查
- 添加安全 hook 包装
- 添加 pattern 备用机制
- 改进安装脚本

### v1.0 (原项目)
- 基础功能实现

## ⚠️ 免责声明

与原版相同：
- 仅供学习研究使用
- 请支持 Spotify 正版
- 使用风险自负

## 🙏 致谢

感谢 [mrpond](https://github.com/mrpond) 的原版项目！

此 fork 仅做稳定性和兼容性改进。
