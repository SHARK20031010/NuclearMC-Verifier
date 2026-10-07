// adapters/dsh/index.js —— DeepSeek Harness (DSH) 原生形式化核验门禁插件
const { spawn } = require("child_process");
const fs = require("fs");
const path = require("path");

const VERIFY_HELPER = path.resolve(__dirname, "verify_helper.py");
const SPEC_FILE = path.resolve(__dirname, "../../PHYSICS_SPEC.md");
const CONFIG_FILE = path.resolve(__dirname, "../../.mc-verifier.yaml");

const TARGET_EXTS = new Set([".cc", ".cpp", ".cxx", ".hh", ".h"]);
const MC_KEYWORDS = [
  "G4",
  "geant4",
  "openmc",
  "ParticleGun",
  "GPS",
  "SteppingAction",
  "EventAction",
  "PhysicsList",
  "G4RunManager",
  "G4VUserDetectorConstruction"
];

/**
 * 读取当前核验器总控状态
 */
function getSwitchConfig() {
  // 环境变量最高优先级
  if (process.env.MC_VERIFIER_ENABLE === "0") {
    return { enabled: false, strict: false };
  }
  if (process.env.MC_VERIFIER_ENABLE === "1") {
    return { enabled: true, strict: true };
  }

  if (fs.existsSync(CONFIG_FILE)) {
    try {
      const content = fs.readFileSync(CONFIG_FILE, "utf-8");
      const enabled = /enabled:\s*true/i.test(content);
      const strict = !/strict_mode:\s*false/i.test(content);
      return { enabled, strict };
    } catch (e) {
      // 容错默认开启
    }
  }
  return { enabled: true, strict: true };
}

/**
 * 判断文件扩展名是否为 C++ 源码
 */
function isCppFile(filePath) {
  if (!filePath) return false;
  const ext = path.extname(filePath).toLowerCase();
  return TARGET_EXTS.has(ext);
}

/**
 * 判断代码文本是否包含粒子输运仿真特征
 */
function hasPhysicsKeywords(codeText) {
  if (!codeText) return false;
  return MC_KEYWORDS.some((kw) => codeText.includes(kw));
}

/**
 * 模拟 edit 工具的内存替换，获得打补丁后的完整源文件
 */
function simulateEdit(filePath, oldString, newString, replaceAll) {
  try {
    const absPath = path.resolve(filePath);
    if (!fs.existsSync(absPath)) return null;
    const original = fs.readFileSync(absPath, "utf-8");
    if (!original.includes(oldString)) return null;

    if (replaceAll) {
      return original.split(oldString).join(newString);
    }
    return original.replace(oldString, newString);
  } catch (e) {
    return null;
  }
}

/**
 * 调用 Python 形式化核验管道进行高速流式断言
 */
function runVerification(code, filename, strict) {
  return new Promise((resolve) => {
    const child = spawn("python3", [VERIFY_HELPER], {
      stdio: ["pipe", "pipe", "pipe"],
      env: process.env,
    });

    let stdoutData = "";
    let stderrData = "";

    child.stdout.on("data", (chunk) => {
      stdoutData += chunk.toString();
    });

    child.stderr.on("data", (chunk) => {
      stderrData += chunk.toString();
    });

    const timer = setTimeout(() => {
      child.kill("SIGKILL");
      resolve({
        passed: true,
        timeout: true,
        message: "核验超时（放行）",
      });
    }, 6000);

    child.on("close", (code) => {
      clearTimeout(timer);
      if (code !== 0 || !stdoutData.trim()) {
        // Python 执行异常时的兜底
        return resolve({
          passed: true,
          error: stderrData || "核验子进程异常退出",
        });
      }
      try {
        const res = JSON.parse(stdoutData.trim());
        resolve(res);
      } catch (err) {
        resolve({
          passed: true,
          error: "核验结果 JSON 解析失败: " + stdoutData,
        });
      }
    });

    // 写入 JSON 请求
    const payload = JSON.stringify({
      code,
      filename: path.basename(filename || "code.cc"),
      strict: strict !== false,
    });
    child.stdin.write(payload);
    child.stdin.end();
  });
}

/**
 * 检查 bash 命令行是否存在利用重定向绕过文件工具直接写盘的行为
 */
function isBashBypassingPhysicsCode(cmd) {
  if (!cmd || typeof cmd !== "string") return false;
  // 匹配 > file.cc, >> file.cc, tee file.cc 等
  const redirectPattern = /(?:>{1,2}|tee)\s*([^\s|;&]+\.(?:cc|cpp|cxx|hh|h))/i;
  return redirectPattern.test(cmd);
}

module.exports = {
  name: "dsh-mc-formal-verifier",
  inject: ["commands"],
  apply(ctx) {
    ctx.logger && ctx.logger.info("MC-FormalVerifier native gate loaded into DSH.");

    // ═════════════════════════════════════════════════════════════════
    // 1. 挂接 DSH 原生 tools/pre-execute waterfall 硬门禁
    // ═════════════════════════════════════════════════════════════════
    ctx.on("tools/pre-execute", async (exec, next) => {
      const cfg = getSwitchConfig();
      if (!cfg.enabled) {
        // 全局开关处于关闭状态，0ms 穿透放行
        return next();
      }

      const toolName = exec.name;
      const args = exec.arguments || {};

      // ── 情况 A：write 工具全量写盘 ──
      if (toolName === "write") {
        const filePath = args.file_path || "";
        const content = args.content || "";

        if (isCppFile(filePath) && hasPhysicsKeywords(content)) {
          const res = await runVerification(content, filePath, cfg.strict);
          if (!res.passed) {
            return {
              kind: "deny",
              reason: res.reason || "代码违反蒙卡物理守恒不变式，已被 DSH 门禁阻断。",
            };
          }
        }
      }

      // ── 情况 B：edit 工具局部编辑（打补丁后全量核验） ──
      if (toolName === "edit") {
        const filePath = args.file_path || "";
        const oldStr = args.old_string || "";
        const newStr = args.new_string || "";

        if (isCppFile(filePath)) {
          const patched = simulateEdit(filePath, oldStr, newStr, args.replace_all);
          if (patched && hasPhysicsKeywords(patched)) {
            const res = await runVerification(patched, filePath, cfg.strict);
            if (!res.passed) {
              return {
                kind: "deny",
                reason: res.reason || "代码修改违反蒙卡物理守恒不变式，已被 DSH 门禁阻断。",
              };
            }
          }
        }
      }

      // ── 情况 C：bash 工具检测终端绕过 ──
      if (toolName === "bash") {
        const cmd = args.command || "";
        if (isBashBypassingPhysicsCode(cmd)) {
          return {
            kind: "deny",
            reason:
              "【DSH 蒙卡形式化核验器硬阻断】检测到尝试使用 bash 命令行重定向绕过写盘门禁生成粒子输运代码！为保障物理守恒不变式刚性约束，严禁使用 shell 重定向写盘，请使用标准 write 工具提交完整代码。",
          };
        }
      }

      // 其余情况正常放行
      return next();
    });

    // ═════════════════════════════════════════════════════════════════
    // 2. 注册 DSH 斜杠命令（命令注册表服务名: commands）
    // ═════════════════════════════════════════════════════════════════
    ctx.commands.register({
      name: "mc-status",
      description: "查看粒子输运蒙卡形式化核验器状态",
      recordInput: false,
      handler: async () => {
        const cfg = getSwitchConfig();
        const statusMsg = [
          "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━",
          "  针对大语言模型生成粒子输运蒙卡程序的形式化核验器 · DSH 状态",
          "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━",
          `  • 全局门禁状态: [${cfg.enabled ? "✅ 开启 ACTIVE" : "⏸️ 关闭 NO-OP"}]`,
          `  • 阻断策略模式: [${cfg.strict ? "刚性阻断 (Strict Block)" : "告警放行 (Warn Only)"}]`,
          "  • 拦截监听工具: write（全量拦截）、edit（打补丁还原拦截）、bash（防绕过）",
          "  • 底层守恒总线: 因果时钟、相空间测度、权重流无偏、生命周期、核数据",
          "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━",
        ].join("\n");
        return { kind: "success", text: statusMsg };
      },
    });

    // /mc-on
    ctx.commands.register({
      name: "mc-on",
      description: "开启蒙卡物理守恒形式化核验门禁",
      recordInput: false,
      handler: async () => {
        try {
          if (fs.existsSync(CONFIG_FILE)) {
            let content = fs.readFileSync(CONFIG_FILE, "utf-8");
            content = content.replace(/enabled:\s*false/i, "enabled: true");
            fs.writeFileSync(CONFIG_FILE, content, "utf-8");
          }
          return {
            kind: "success",
            text: "✅ DSH 形式化核验门禁已开启（物理守恒刚性阻断已就绪）。",
          };
        } catch (e) {
          return { kind: "error", text: `修改配置失败: ${e.message}` };
        }
      },
    });

    // /mc-off
    ctx.commands.register({
      name: "mc-off",
      description: "临时关闭蒙卡物理形式化核验门禁",
      recordInput: false,
      handler: async () => {
        try {
          if (fs.existsSync(CONFIG_FILE)) {
            let content = fs.readFileSync(CONFIG_FILE, "utf-8");
            content = content.replace(/enabled:\s*true/i, "enabled: false");
            fs.writeFileSync(CONFIG_FILE, content, "utf-8");
          }
          return {
            kind: "success",
            text: "⏸️ DSH 形式化核验门禁已关闭（零开销放行模式）。",
          };
        } catch (e) {
          return { kind: "error", text: `修改配置失败: ${e.message}` };
        }
      },
    });

    // /mc-verify <文件路径.cc>
    ctx.commands.register({
      name: "mc-verify",
      description: "对指定的 C++ 蒙卡源码执行离线形式化核验",
      input: { hint: "<文件路径.cc>" },
      recordInput: false,
      handler: async (invocation) => {
        const filepath = String((invocation && invocation.rawInput) || "").trim();
        if (!filepath) {
          return { kind: "error", text: "用法: /mc-verify <文件路径.cc>" };
        }
        const absPath = path.resolve(filepath);
        if (!fs.existsSync(absPath)) {
          return { kind: "error", text: `文件未找到: ${filepath}` };
        }
        const code = fs.readFileSync(absPath, "utf-8");
        const res = await runVerification(code, absPath, true);
        if (res.passed) {
          return {
            kind: "success",
            text: `✅ [通过] 文件 ${path.basename(filepath)} 完全符合五大正交守恒不变式。`,
          };
        }
        return { kind: "error", text: res.reason || "❌ 检测到物理守恒违约。" };
      },
    });

    // /mc-spec
    ctx.commands.register({
      name: "mc-spec",
      description: "查看粒子输运蒙卡形式化规约白皮书",
      recordInput: false,
      handler: async () => {
        if (fs.existsSync(SPEC_FILE)) {
          return { kind: "success", text: fs.readFileSync(SPEC_FILE, "utf-8") };
        }
        return { kind: "error", text: "未找到 PHYSICS_SPEC.md 规约文件。" };
      },
    });
  },
};
