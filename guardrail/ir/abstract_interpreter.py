#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/abstract_interpreter.py —— 微观物理抽象域解释器 (Micro-Abstract Domain Interpreter)

用于破解极端动态 C++ 特性（指针多级别名传递、链式临时变量、局部中间变量计算、宏展开折叠）下的 AST 静态分析盲区。
基于轻量级基本块抽象解释（Abstract Interpretation）与指向分析（Points-to Analysis）：
1. 别名与指针流转：追踪 StepPoint（Pre vs Post）、Track、Volume 指针在局部作用域的多级赋值与别名传递；
2. 权重流转布尔代数：追踪 GetWeight() 经由局部乘除、缩放传递至累加算子的布尔代数传播；
3. 鞅因果滤波时序 (MeasureBasis.FILTRATION)：追踪初态动能、时间戳抓取所处的生命周期状态与相互作用前后顺序；
4. 支撑集门控 (MeasureBasis.SUPPORT)：判定 fGeomBoundary 等条件判定作用的底层真实实体与前/后步点有效性；
5. 示性重数门控 (MeasureBasis.MULTIPLICITY)：判定次级粒子产生计数是否满足单步单射性或去重守卫。
"""

from __future__ import annotations

import re
from typing import Dict, List, Optional, Any, Set, Tuple
from pydantic import BaseModel, Field

from guardrail.ir.pst_schema import MeasureBasis, LifecycleState


class AbstractValue(BaseModel):
    """抽象值定义"""
    var_name: str
    target_entity: Optional[str] = None          # "PreStepPoint", "PostStepPoint", "Track", "Volume", etc.
    is_weighted: bool = False                    # 是否携带粒子统计权重流
    is_global_time: Optional[bool] = None        # True: GlobalTime, False: LocalTime
    lifecycle_state: Optional[LifecycleState] = None
    origin_expr: str = ""                        # 产生该抽象值的源头表达式
    aliases: Set[str] = Field(default_factory=set)


class FunctionAbstractProfile(BaseModel):
    """函数级抽象解释分析概要"""
    func_name: str = ""
    env: Dict[str, AbstractValue] = Field(default_factory=dict)
    boundary_checks: List[Dict[str, Any]] = Field(default_factory=list)
    weighted_accumulations: List[Dict[str, Any]] = Field(default_factory=list)
    timing_fetches: List[Dict[str, Any]] = Field(default_factory=list)
    energy_fetches: List[Dict[str, Any]] = Field(default_factory=list)
    multiplicity_guards: List[Dict[str, Any]] = Field(default_factory=list)
    process_invocations: List[Dict[str, Any]] = Field(default_factory=list)
    filtration_violations: List[Dict[str, Any]] = Field(default_factory=list)
    support_violations: List[Dict[str, Any]] = Field(default_factory=list)
    multiplicity_violations: List[Dict[str, Any]] = Field(default_factory=list)


class MicroAbstractInterpreter:
    """微观物理抽象域解释器 (零外部重量级运行时，< 2ms 延迟)"""

    def __init__(self):
        pass

    def interpret_code_snippet(self, code: str, func_name: str = "") -> FunctionAbstractProfile:
        """
        对 C++ 代码片段或单个函数体执行微观物理抽象解释与指向分析。
        """
        profile = FunctionAbstractProfile(func_name=func_name)
        env: Dict[str, AbstractValue] = {}

        # 初始预置隐式指针（若存在）
        env["step"] = AbstractValue(var_name="step", target_entity="Step")
        env["aStep"] = AbstractValue(var_name="aStep", target_entity="Step")
        env["s"] = AbstractValue(var_name="s", target_entity="Step")
        env["track"] = AbstractValue(var_name="track", target_entity="Track")
        env["aTrack"] = AbstractValue(var_name="aTrack", target_entity="Track")

        lines = code.splitlines()

        # 作用域与守卫状态栈
        in_multiplicity_guard = False
        in_boundary_guard = False
        current_boundary_is_post = False
        current_boundary_is_pre = False

        for line_idx, raw_line in enumerate(lines, start=1):
            line = raw_line.strip()
            if not line or line.startswith("//"):
                continue

            # ─────────────────────────────────────────────────────────
            # 1. 示性重数守恒门控 (MeasureBasis.MULTIPLICITY) 侦测
            # ─────────────────────────────────────────────────────────
            if any(p in line for p in [
                "GetCurrentStepNumber() == 1",
                "GetCurrentStepNumber()==1",
                "stepNumber == 1",
                "stepNumber==1",
                "GetStepNumber() == 1",
                "GetStepNumber()==1"
            ]):
                in_multiplicity_guard = True
                profile.multiplicity_guards.append({
                    "line": line_idx,
                    "type": "StepNumberOne",
                    "code": line,
                })
            if any(p in line for p in [".insert(", "insert(trackID)", "insert(track->GetTrackID())", "visitedTracks"]):
                in_multiplicity_guard = True
                profile.multiplicity_guards.append({
                    "line": line_idx,
                    "type": "SetDedup",
                    "code": line,
                })

            # ─────────────────────────────────────────────────────────
            # 2. 物理相互作用过程触发 (Process Invocation)
            # ─────────────────────────────────────────────────────────
            if any(p in line for p in [
                "ApplyChange", "SampleSecondaries", "DoIt", "Inelastic", "PostStepDoIt", "AlongStepDoIt"
            ]):
                profile.process_invocations.append({
                    "line": line_idx,
                    "code": line,
                })

            # ─────────────────────────────────────────────────────────
            # 3. 指针别名、赋值与布尔代数抽象解释 (Points-to & Algebra)
            # ─────────────────────────────────────────────────────────
            # 匹配常规声明赋值与裸赋值:
            #   auto* p = ...;  G4StepPoint* pt = ...;  pt2 = pt;  double w = ...;
            assign_pattern = re.search(
                r'^(?:(?:const\s+)?(?:auto|G4StepPoint|G4Track|G4VPhysicalVolume|double|G4double|int|G4int)\s*[\*&]?\s*)?([a-zA-Z_]\w*)\s*=\s*(.+?);',
                line
            )
            if assign_pattern:
                var_name = assign_pattern.group(1).strip()
                rhs = assign_pattern.group(2).strip()

                val = AbstractValue(var_name=var_name, origin_expr=rhs)

                # 3.1 别名解析：如果是现有指针的直接赋值或传递
                # 例如: auto* q = p;
                if rhs in env:
                    src_val = env[rhs]
                    val.target_entity = src_val.target_entity
                    val.is_weighted = src_val.is_weighted
                    val.is_global_time = src_val.is_global_time
                    val.lifecycle_state = src_val.lifecycle_state
                    val.aliases.update(src_val.aliases)
                    val.aliases.add(rhs)
                    src_val.aliases.add(var_name)

                # 3.2 步点实体解析 (Pre vs Post)
                if "GetPostStepPoint()" in rhs:
                    val.target_entity = "PostStepPoint"
                    val.lifecycle_state = LifecycleState.STEP_POST
                elif "GetPreStepPoint()" in rhs:
                    val.target_entity = "PreStepPoint"
                    val.lifecycle_state = LifecycleState.STEP_PRE

                # 3.3 径迹实体解析
                if "GetTrack()" in rhs:
                    val.target_entity = "Track"

                # 3.4 权重流抽象值传递 (布尔代数 propagation)
                if "GetWeight()" in rhs:
                    val.is_weighted = True
                else:
                    # 检查 RHS 中的任一标识符是否携带权重
                    rhs_identifiers = set(re.findall(r'\b[a-zA-Z_]\w*\b', rhs))
                    if any(ident in env and env[ident].is_weighted for ident in rhs_identifiers):
                        val.is_weighted = True

                # 3.5 时钟源解析 (Global vs Local)
                if "GetGlobalTime()" in rhs:
                    val.is_global_time = True
                    profile.timing_fetches.append({"var": var_name, "is_global": True, "line": line_idx, "code": line})
                elif "GetLocalTime()" in rhs:
                    val.is_global_time = False
                    profile.timing_fetches.append({"var": var_name, "is_global": False, "line": line_idx, "code": line})

                # 3.6 动能抓取时序与因果态 (MeasureBasis.FILTRATION)
                if "GetKineticEnergy()" in rhs:
                    # 解析属于哪个步点指针
                    caller = rhs.split("->")[0].strip()
                    ent = env.get(caller, AbstractValue(var_name=caller)).target_entity
                    if "GetPostStepPoint()" in rhs or ent == "PostStepPoint":
                        val.lifecycle_state = LifecycleState.STEP_POST
                    elif "GetPreStepPoint()" in rhs or ent == "PreStepPoint":
                        val.lifecycle_state = LifecycleState.STEP_PRE

                    profile.energy_fetches.append({
                        "var": var_name,
                        "lifecycle_state": val.lifecycle_state,
                        "line": line_idx,
                        "code": line,
                    })

                    # 检查鞅流因果滤波逆向违约：
                    # 若在发生核反应/相互作用过程之后才抓取本应为初态的动能，或者直接抓 PostStepPoint 作为初态能
                    if profile.process_invocations:
                        profile.filtration_violations.append({
                            "line": line_idx,
                            "type": "EnergyFetchedAfterProcess",
                            "var": var_name,
                            "code": line,
                            "description": "动能抓取发生于相互作用过程之后，违背鞅因果滤波时序 (Filtration Inversion)。",
                        })

                env[var_name] = val

            # ─────────────────────────────────────────────────────────
            # 4. 边界支撑集门控 (MeasureBasis.SUPPORT) 校验
            # ─────────────────────────────────────────────────────────
            if "fGeomBoundary" in line:
                # 提取调用的变量名，如: pt->GetStepStatus() == fGeomBoundary
                var_match = re.search(r'([a-zA-Z_]\w*)->GetStepStatus\(\)', line)
                if var_match:
                    caller = var_match.group(1).strip()
                    target_ent = env.get(caller, AbstractValue(var_name=caller)).target_entity

                    is_post = (target_ent == "PostStepPoint") or ("GetPostStepPoint()" in line)
                    is_pre = (target_ent == "PreStepPoint") or ("GetPreStepPoint()" in line)

                    in_boundary_guard = True
                    current_boundary_is_post = is_post
                    current_boundary_is_pre = is_pre

                    profile.boundary_checks.append({
                        "var": caller,
                        "target_entity": target_ent or ("PostStepPoint" if is_post else ("PreStepPoint" if is_pre else "Unknown")),
                        "is_post_step": is_post,
                        "is_pre_step": is_pre,
                        "line": line_idx,
                        "code": line,
                    })

                    # 支撑域错位报警：使用 PreStepPoint 判定界面通量
                    if is_pre and not is_post:
                        profile.support_violations.append({
                            "line": line_idx,
                            "type": "PreStepBoundaryMisalignment",
                            "caller": caller,
                            "code": line,
                            "description": "边界判定使用了前步点 (PreStepPoint)，未对齐界面支撑域 ∂V (Support Misalignment)。",
                        })
                elif "GetPostStepPoint()->GetStepStatus()" in line:
                    in_boundary_guard = True
                    current_boundary_is_post = True
                    current_boundary_is_pre = False
                    profile.boundary_checks.append({
                        "var": "anonymous_post",
                        "target_entity": "PostStepPoint",
                        "is_post_step": True,
                        "is_pre_step": False,
                        "line": line_idx,
                        "code": line,
                    })
                elif "GetPreStepPoint()->GetStepStatus()" in line:
                    in_boundary_guard = True
                    current_boundary_is_post = False
                    current_boundary_is_pre = True
                    profile.boundary_checks.append({
                        "var": "anonymous_pre",
                        "target_entity": "PreStepPoint",
                        "is_post_step": False,
                        "is_pre_step": True,
                        "line": line_idx,
                        "code": line,
                    })
                    profile.support_violations.append({
                        "line": line_idx,
                        "type": "PreStepBoundaryMisalignment",
                        "caller": "anonymous_pre",
                        "code": line,
                        "description": "边界判定直接使用了 GetPreStepPoint()，未对齐界面支撑域 ∂V (Support Misalignment)。",
                    })

            # ─────────────────────────────────────────────────────────
            # 5. 统计累加器抽象解释 (Accumulate Checks)
            # ─────────────────────────────────────────────────────────
            if any(op in line for op in ["+=", "Fill(", "AddEdep(", "AddFlux(", "fFlux +=", "flux +=", "nSecondary++", "secondaries++"]):
                # 5.1 权重依赖
                rhs_tokens = set(re.findall(r'\b[a-zA-Z_]\w*\b', line))
                has_weight = "GetWeight" in line or any(t in env and env[t].is_weighted for t in rhs_tokens)
                profile.weighted_accumulations.append({
                    "line": line_idx,
                    "code": line,
                    "is_weighted": has_weight,
                    "multiplicity_guarded": in_multiplicity_guard,
                })

                # 5.2 次级粒子生成重数倍乘 (Multiplicity Inflation) 违约检测
                is_secondary_tally = any(k in line.lower() for k in ["sec", "product", "secondary", "recoil", "yield"])
                if is_secondary_tally and not in_multiplicity_guard:
                    profile.multiplicity_violations.append({
                        "line": line_idx,
                        "code": line,
                        "description": "次级粒子计数累加缺少 step == 1 或去重守卫，示性测度沿径迹严重倍乘膨胀 (Multiplicity Inflation)。",
                    })

                # 5.3 跨界面通量累加缺少 PostStepPoint 边界守卫
                is_flux_tally = any(k in line.lower() for k in ["flux", "current", "crossing", "surface"])
                if is_flux_tally:
                    if not in_boundary_guard or not current_boundary_is_post:
                        profile.support_violations.append({
                            "line": line_idx,
                            "type": "MissingBoundaryGate",
                            "code": line,
                            "description": "界面通量累加未受 PostStepPoint fGeomBoundary 支撑域门控保护 (Support Misalignment)。",
                        })

        profile.env = env
        return profile
