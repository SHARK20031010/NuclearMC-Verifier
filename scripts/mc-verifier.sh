#!/usr/bin/env bash
# ==============================================================================
# scripts/mc-verifier.sh —— 5 大 Harness 统一管理控制脚本
#
# 支持平台:
#   1. Google Antigravity (AGY)
#   2. OpenCode (opencode)
#   3. DSH (DeepSeek Harness)
#   4. Anthropic Claude Code
#   5. OpenAI Codex
# ==============================================================================

set -e

TARGET_SCRIPT="${BASH_SOURCE[0]}"
while [ -L "$TARGET_SCRIPT" ]; do
    TARGET_DIR="$(cd "$(dirname "$TARGET_SCRIPT")" && pwd)"
    TARGET_SCRIPT="$(readlink "$TARGET_SCRIPT")"
    [[ $TARGET_SCRIPT != /* ]] && TARGET_SCRIPT="$TARGET_DIR/$TARGET_SCRIPT"
done
SCRIPT_DIR="$(cd "$(dirname "$TARGET_SCRIPT")" && pwd)"
ROOT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
CONFIG_FILE="${ROOT_DIR}/.mc-verifier.yaml"
CHECK_PY="${ROOT_DIR}/guardrail/check.py"
MCP_SERVER="${ROOT_DIR}/adapters/mcp/server.py"

# 自动定位真实用户家目录（避免子账号沙箱隔离导致找不到 ~/.dsh 或 ~/.opencode）
if [ -d "/home/shark" ]; then
    USER_HOME="/home/shark"
else
    USER_HOME="$HOME"
fi

get_status() {
    python3 -c "
import sys
sys.path.insert(0, '${ROOT_DIR}')
from guardrail.switch import is_enabled, is_strict, require_spec
print('STATUS:' + ('ON' if is_enabled() else 'OFF'))
print('MODE:' + ('STRICT' if is_strict() else 'WARN_ONLY'))
print('SPEC:' + ('REQUIRED' if require_spec() else 'OPTIONAL'))
"
}

set_enabled() {
    local val=$1
    if [ ! -f "$CONFIG_FILE" ]; then
        echo "enabled: $val" > "$CONFIG_FILE"
    else
        python3 -c "
import re
with open('${CONFIG_FILE}', 'r', encoding='utf-8') as f:
    content = f.read()
content = re.sub(r'enabled:\s*(true|false)', 'enabled: $val', content, flags=re.IGNORECASE)
with open('${CONFIG_FILE}', 'w', encoding='utf-8') as f:
    f.write(content)
"
    fi
}

cmd_on() {
    set_enabled "true"
    echo "=================================================================="
    echo "  [MC-FormalVerifier] 全局开关已设置为：开启 (ON / ACTIVE)"
    echo "  所有 5 大 Harness (Antigravity, OpenCode, DSH, Claude Code, Codex)"
    echo "  将强制执行 5 大物理守恒不变式硬性拦截与输入规约核验！"
    echo "=================================================================="
}

cmd_off() {
    set_enabled "false"
    echo "=================================================================="
    echo "  [MC-FormalVerifier] 全局开关已设置为：关闭 (OFF / NO-OP)"
    echo "  所有 5 大 Harness 立即进入 0ms 直通模式，完全不拦截、零性能开销。"
    echo "=================================================================="
}

cmd_status() {
    echo "=================================================================="
    echo "  针对大语言模型生成粒子输运蒙卡程序的形式化核验器 · 状态看板"
    echo "=================================================================="
    local raw_info
    raw_info=$(get_status)
    local cur_status=$(echo "$raw_info" | grep '^STATUS:' | cut -d: -f2)
    local cur_mode=$(echo "$raw_info" | grep '^MODE:' | cut -d: -f2)
    local cur_spec=$(echo "$raw_info" | grep '^SPEC:' | cut -d: -f2)

    if [ "$cur_status" = "ON" ]; then
        echo "  全局核心开关状态: [✅ ON 开启] (执行物理守恒阻断)"
    else
        echo "  全局核心开关状态: [⏸️ OFF 关闭] (严格 0ms No-Op 直通)"
    fi
    echo "  工作阻断模式:     $cur_mode"
    echo "  输入规约契约要求: $cur_spec"
    echo ""
    echo "  --- DSH (DeepSeek Harness) 核心运行环境 ---"
    if [ -d "$USER_HOME/.dsh" ]; then
        local dsh_mount_ok="false"
        if [ -L "$USER_HOME/.dsh/profiles/web/node_modules/dsh-mc-formal-verifier" ]; then
            dsh_mount_ok="true"
        fi
        if [ "$dsh_mount_ok" = "true" ]; then
            echo "  • DSH 原生生命周期门禁: [已就绪 🚀 物理硬拦截生效中]"
            echo "    - 宿主加载: web profile (cordis.patch.yml)"
            echo "    - 拦截管道: tools/pre-execute waterfall"
            echo "    - 监控工具: write (全量代码检查), edit (局部修改还原检查), bash (拦截终端重定向写入)"
            echo "    - 交互命令: /mc-status, /mc-on, /mc-off, /mc-verify, /mc-spec"
            echo "    - 技能装载: ~/.dsh/skills/mc-verifier/SKILL.md"
        else
            echo "  • DSH 原生生命周期门禁: [未挂载 ⚠️ 可运行 ./mc-verifier install-dsh 自动挂载]"
        fi
    else
        echo "  • DSH 环境: [未检测到 $USER_HOME/.dsh]"
    fi

    echo ""
    echo "  --- 其它辅助 Harness 状态 ---"
    # 1. Antigravity
    if [ -f "${ROOT_DIR}/.agents/plugins/mc-formal-verifier/plugin.json" ]; then
        echo "  • Antigravity: [已就绪 ✅] (.agents/plugins/ + hooks.json 阻断钩子)"
    fi
    # 2. MCP Server
    if [ -f "${ROOT_DIR}/adapters/mcp/server.py" ]; then
        echo "  • 通用 MCP:    [已就绪 ✅] (adapters/mcp/server.py stdio 协议)"
    fi
    echo "=================================================================="
}

cmd_check() {
    local target="$1"
    if [ -z "$target" ]; then
        echo "用法: ./scripts/mc-verifier.sh check <代码路径.cc>"
        exit 1
    fi
    python3 "$CHECK_PY" --code "$target" "${@:2}"
}

cmd_install_all() {
    echo "正在向本地检测到的 5 大 Harness 注册与挂载形式化核验器..."

    # 1. Antigravity: 确保 .agents/skills/ 存在
    mkdir -p "${ROOT_DIR}/.agents/skills/mc-verifier"
    cp -f "${ROOT_DIR}/adapters/skills/mc-verifier/SKILL.md" "${ROOT_DIR}/.agents/skills/mc-verifier/SKILL.md"
    echo "  [OK] Antigravity: 插件与技能已就绪 (.agents)"

    # 2. DSH: 链接到 ~/.dsh/skills
    if [ -d "$USER_HOME/.dsh/skills" ]; then
        mkdir -p "$USER_HOME/.dsh/skills/mc-verifier"
        cp -f "${ROOT_DIR}/adapters/skills/mc-verifier/SKILL.md" "$USER_HOME/.dsh/skills/mc-verifier/SKILL.md"
        echo "  [OK] DSH: 技能已安装到 $USER_HOME/.dsh/skills/mc-verifier"
    fi

    # 3. Codex: 链接到 ~/.codex/skills
    if [ -d "$USER_HOME/.codex/skills" ]; then
        mkdir -p "$USER_HOME/.codex/skills/mc-verifier"
        cp -f "${ROOT_DIR}/adapters/skills/mc-verifier/SKILL.md" "$USER_HOME/.codex/skills/mc-verifier/SKILL.md"
        echo "  [OK] Codex: 技能已安装到 $USER_HOME/.codex/skills/mc-verifier"
    fi

    # 4. OpenCode: 链接到 ~/.opencode/skills
    if [ -d "$USER_HOME/.opencode/skills" ]; then
        mkdir -p "$USER_HOME/.opencode/skills/mc-verifier"
        cp -f "${ROOT_DIR}/adapters/skills/mc-verifier/SKILL.md" "$USER_HOME/.opencode/skills/mc-verifier/SKILL.md"
        echo "  [OK] OpenCode: 技能已安装到 $USER_HOME/.opencode/skills/mc-verifier"
    fi

    echo ""
    echo "=================================================================="
    echo "  一键 MCP 服务挂载命令（可选，支持智能体主动调用）："
    echo "  - Claude Code:  claude mcp add mc-verifier python3 ${MCP_SERVER}"
    echo "  - OpenCode:     opencode mcp add mc-verifier python3 ${MCP_SERVER}"
    echo "=================================================================="
    echo "全部 5 大 Harness 适配环境部署完毕！"
}

case "$1" in
    on)
        cmd_on
        ;;
    off)
        cmd_off
        ;;
    status)
        cmd_status
        ;;
    check)
        cmd_check "$2" "${@:3}"
        ;;
    install-all)
        cmd_install_all
        ;;
    *)
        echo "形式化核验器 (MC-FormalVerifier) 统一管理工具"
        echo "用法:"
        echo "  $0 on           - 开启核验（所有 5 个 Harness 强制执行守恒量门禁）"
        echo "  $0 off          - 关闭核验（所有 5 个 Harness 进入严格 0ms No-Op 直通）"
        echo "  $0 status       - 查看当前启闭状态及 5 大 Harness 就绪情况"
        echo "  $0 check <文件> - 手动执行单文件形式化核验"
        echo "  $0 install-all  - 一键向本地全部 5 大 Harness 挂载技能与适配器"
        exit 1
        ;;
esac
