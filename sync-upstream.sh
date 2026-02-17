#!/bin/bash
# sync-upstream.sh - 自动同步上游更新并检查 pattern 变化

set -e

UPSTREAM_REPO="https://github.com/mrpond/BlockTheSpot.git"
LOCAL_REPO="$(dirname "$0")"
cd "$LOCAL_REPO"

echo "🔄 BlockTheSpot 更新检查脚本"
echo "================================"
echo ""

# 检查 git 是否可用
if ! command -v git &> /dev/null; then
    echo "❌ 错误: 未找到 git"
    exit 1
fi

# 获取上游最新 commit
echo "📥 获取上游更新..."
git fetch upstream 2>/dev/null || git remote add upstream "$UPSTREAM_REPO" && git fetch upstream

LOCAL_COMMIT=$(git rev-parse HEAD)
UPSTREAM_COMMIT=$(git rev-parse upstream/master)

if [ "$LOCAL_COMMIT" = "$UPSTREAM_COMMIT" ]; then
    echo "✅ 已经是最新版本"
    exit 0
fi

echo ""
echo "📊 版本对比:"
echo "  本地: ${LOCAL_COMMIT:0:8}"
echo "  上游: ${UPSTREAM_COMMIT:0:8}"
echo ""

# 检查关键文件变化
echo "🔍 检查关键文件变化..."
git diff --name-only "$LOCAL_COMMIT" "$UPSTREAM_COMMIT" -- | while read file; do
    case "$file" in
        *.cpp|*.h)
            echo "  📝 代码文件: $file"
            ;;
        *.ini)
            echo "  ⚙️  配置文件: $file (重要!)"
            ;;
        README.md)
            echo "  📄 文档: $file"
            ;;
    esac
done

echo ""
echo "🔄 建议操作:"
echo ""
echo "  1. 自动合并 (可能产生冲突):"
echo "     git merge upstream/master"
echo ""
echo "  2. 只更新特定文件 (推荐):"
echo "     git checkout upstream/master -- config.ini"
echo "     git checkout upstream/master -- Hook/pattern.cpp"
echo ""
echo "  3. 手动查看差异:"
echo "     git diff $LOCAL_COMMIT $UPSTREAM_COMMIT -- config.ini"
echo ""
echo "⚠️  注意: 更新后请测试 Spotify 是否正常工作!"
echo ""

# 检查 pattern 相关文件是否有更新
PATTERN_CHANGED=$(git diff --name-only "$LOCAL_COMMIT" "$UPSTREAM_COMMIT" -- | grep -E "(pattern|config\.ini)" || true)
if [ -n "$PATTERN_CHANGED" ]; then
    echo "🚨 Pattern 相关文件有更新:"
    echo "$PATTERN_CHANGED"
    echo ""
    echo "这些更新可能修复了黑屏问题，建议同步。"
fi
