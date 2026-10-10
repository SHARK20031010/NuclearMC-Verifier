#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/intent_extractor.py —— LLM 驱动的物理需求结构化拆解引擎 (Ring 0 Intent Extractor)

负责将用户输入的非结构化自然语言（任务 Prompt、实验设计、命令行文本）
或源码头部注释，结构化提取为形式化的物理意图卡 (PhysicsIntent)。

架构策略：
1. 若配置了 LLM (OpenAI / DeepSeek / Gemini / Ollama / vLLM API)，优先调用大模型执行结构化抽取；
2. 若无 API Key 或离线运行，无缝自动回退到基于语义槽位的确定性意图分类器；
3. 输出纯类型化、无歧义的 PhysicsIntent 槽位卡，供给确定性 PST 守恒断言引擎。
"""

from __future__ import annotations

import os
import re
import json
import logging
from enum import Enum
from typing import Dict, List, Optional, Any, Literal
from pydantic import BaseModel, Field

logger = logging.getLogger("MC-IntentExtractor")


class ObservableType(str, Enum):
    """物理观测量槽位"""
    FLUENCE = "Fluence"                     # 粒子通量 / 注量
    ABSORBED_DOSE = "AbsorbedDose"         # 吸收剂量 (Gy = J/kg)
    PEAK_EFFICIENCY = "PeakEfficiency"     # 探测器全能峰效率
    ENERGY_SPECTRUM = "EnergySpectrum"     # 能量沉积谱 / 脉冲高度谱
    TIME_OF_FLIGHT = "TimeOfFlight"         # 飞行时间谱 (TOF)
    REACTION_YIELD = "ReactionYield"       # 核反应产额 / 活度 / 产额
    GENERAL_TRANSPORT = "GeneralTransport" # 通用粒子输运微观统计


class SourceSymmetry(str, Enum):
    """辐射源相空间与角分布对称性槽位"""
    ISOTROPIC_4PI = "Isotropic4Pi"         # 4π 球面度空间各向同性 (dOmega = sin(theta) dtheta dphi)
    DIRECTIONAL_BEAM = "DirectionalBeam"   # 定向准直束流 / 笔束 (Pencil Beam)
    COSMIC_2PI = "Cosmic2Pi"               # 上半球 2π 余弦天顶角分布
    PHASE_SPACE = "PhaseSpace"             # 预先录制的外部相空间文件
    CUSTOM = "Custom"                      # 自定义复杂分布


class TimingClock(str, Enum):
    """时间演化测度槽位"""
    TIME_INDEPENDENT = "TimeIndependent"   # 稳态输运 (时间无关)
    GLOBAL_LAB_CLOCK = "GlobalLabClock"   # 绝对实验室全局时钟 (GetGlobalTime，如 TOF、符合测量)
    TRANSIENT_PULSE = "TransientPulse"     # 瞬态微观脉冲 (如微秒/纳秒单脉冲剂量率，需脉宽归一化)


class PhysicsIntent(BaseModel):
    """标准物理规约意图卡 (Ring 0 Intent Contract)"""
    observable: ObservableType = Field(default=ObservableType.GENERAL_TRANSPORT, description="核心物理观测量")
    source_symmetry: SourceSymmetry = Field(default=SourceSymmetry.CUSTOM, description="辐射源空间角分布对称性")
    timing_clock: TimingClock = Field(default=TimingClock.TIME_INDEPENDENT, description="时间演化测度模式")
    variance_reduction: bool = Field(default=False, description="是否启用了减方差偏倚 (需粒子权重无偏传递)")

    observable_desc: str = Field(default="粒子输运微观统计", description="观测量人类可读描述")
    source_desc: str = Field(default="辐射源项", description="辐射源项人类可读描述")
    normalization_basis: str = Field(default="每初级粒子 (Per Event)", description="统计归一化基准")
    extraction_source: str = Field(default="规则推断", description="意图提取来源 (LLM驱动 / 离线语义解析)")


SYSTEM_PROMPT = """你是一个蒙特卡罗物理仿真 (Geant4 / OpenMC) 领域的专业形式化规约分析器。
请从用户提供的物理仿真需求描述（或代码背景说明）中，抽取以下 4 个核心物理决策槽位，严格输出合法的 JSON 格式：
{
  "observable": "Fluence" | "AbsorbedDose" | "PeakEfficiency" | "EnergySpectrum" | "TimeOfFlight" | "ReactionYield" | "GeneralTransport",
  "source_symmetry": "Isotropic4Pi" | "DirectionalBeam" | "Cosmic2Pi" | "PhaseSpace" | "Custom",
  "timing_clock": "TimeIndependent" | "GlobalLabClock" | "TransientPulse",
  "variance_reduction": true | false,
  "observable_desc": "简短中文物理量说明",
  "source_desc": "简短中文辐射源说明",
  "normalization_basis": "归一化分母基准说明"
}

分类指导原则：
1. 放射性核素点源 (如 Cs-137, Co-60, 点源, 4pi, 各向同性) 必须映射为 source_symmetry = "Isotropic4Pi"；
2. 准直束流、笔束、单向平行束 (如 10 MeV 电子束, 中子束流) 映射为 source_symmetry = "DirectionalBeam"；
3. 全能峰效率、探测效率映射为 observable = "PeakEfficiency"；
4. 吸收剂量、剂量当量、FLASH 剂量率映射为 observable = "AbsorbedDose"；
5. 飞行时间谱 (TOF)、时间符合测量映射为 timing_clock = "GlobalLabClock", observable = "TimeOfFlight"；
6. 凡提及深穿透、铁/水屏蔽、重要抽样、分裂、轮盘赌、权窗、方差缩减，必须将 variance_reduction 设为 true。
只输出 JSON，不要任何多余问候或 Markdown 标记。"""


class IntentExtractor:
    """物理意图拆解器 (LLM + 确定性回退)"""

    def __init__(self):
        self.openai_client = None
        self._init_llm_client()

    def _init_llm_client(self):
        """尝试初始化大模型客户端"""
        api_key = os.environ.get("OPENAI_API_KEY") or os.environ.get("DEEPSEEK_API_KEY")
        base_url = os.environ.get("OPENAI_BASE_URL") or os.environ.get("OPENAI_API_BASE")
        if api_key:
            try:
                import openai
                self.openai_client = openai.OpenAI(api_key=api_key, base_url=base_url)
            except Exception:
                pass

    def extract(self, text_corpus: str, user_prompt: Optional[str] = None) -> PhysicsIntent:
        """主入口：从用户输入或代码语料提取结构化意图卡"""
        query_text = (user_prompt or "").strip()
        if not query_text:
            # 从代码头部前 40 行提取自然语言注释
            lines = [l.lstrip("/#* -") for l in text_corpus.splitlines()[:40]]
            query_text = "\n".join(lines).strip()

        if not query_text:
            return PhysicsIntent(extraction_source="默认缺省规约")

        # 1. 如果存在 LLM 客户端，优先使用大模型调用
        if self.openai_client:
            intent = self._extract_with_llm(query_text)
            if intent:
                return intent

        # 2. 离线/无 API Key 时：无缝调用确定性语义槽位分类器
        return self._extract_with_semantic_parser(query_text)

    def _extract_with_llm(self, query_text: str) -> Optional[PhysicsIntent]:
        """通过 LLM API 进行泛化意图拆解"""
        try:
            model = os.environ.get("MC_LLM_MODEL", "gpt-4o-mini")
            response = self.openai_client.chat.completions.create(
                model=model,
                messages=[
                    {"role": "system", "content": SYSTEM_PROMPT},
                    {"role": "user", "content": f"用户物理需求输入:\n{query_text}"},
                ],
                temperature=0.0,
            )
            content = response.choices[0].message.content.strip()
            # 提取 JSON 内容
            if "```" in content:
                content = re.sub(r"^```(?:json)?\s*", "", content, flags=re.MULTILINE)
                content = re.sub(r"\s*```$", "", content, flags=re.MULTILINE)
            data = json.loads(content)
            data["extraction_source"] = f"LLM 结构化抽取 ({model})"
            return PhysicsIntent(**data)
        except Exception as e:
            logger.debug(f"LLM 抽取回退: {e}")
            return None

    def _extract_with_semantic_parser(self, query_text: str) -> PhysicsIntent:
        """确定性自然语言语义槽位分类器 (离线高泛化回退)"""
        t = query_text.lower()

        # 1. 观测量判定 (Observable)
        if any(w in t for w in ["全能峰", "峰效率", "探测效率", "peak eff", "efficiency", "hpge", "nai"]):
            obs = ObservableType.PEAK_EFFICIENCY
            obs_desc = "探测器全能峰绝对效率 (Peak Efficiency)"
            norm = "发射总粒子数归一化 (Per Emitted Primary)"
        elif any(w in t for w in ["吸收剂量", "剂量率", "dose", "flash", "gy", "water phantom", "水模"]):
            obs = ObservableType.ABSORBED_DOSE
            obs_desc = "靶区介质吸收剂量 / 剂量率 (Gy = J/kg)"
            norm = "靶区介质质量除法归一 (Gy = J/kg)"
        elif any(w in t for w in ["飞行时间", "tof", "time of flight", "time-of-flight"]):
            obs = ObservableType.TIME_OF_FLIGHT
            obs_desc = "快粒子飞行时间谱 (Time of Flight Spectrum)"
            norm = "每注入脉冲粒子数归一化"
        elif any(w in t for w in ["通量", "注量", "fluence", "flux", "深穿透"]):
            obs = ObservableType.FLUENCE
            obs_desc = "空间微观粒子注量 (Particle Fluence)"
            norm = "权重无偏统计归一化 (Sum of Weights / N)"
        elif any(w in t for w in ["能谱", "脉冲高度", "spectrum", "pulse height", "多道"]):
            obs = ObservableType.ENERGY_SPECTRUM
            obs_desc = "能量沉积响应谱 (Pulse Height Spectrum)"
            norm = "每初级事件归一化 (Per Primary)"
        elif any(w in t for w in ["反应产额", "活度", "yield", "activity", "产额"]):
            obs = ObservableType.REACTION_YIELD
            obs_desc = "核反应产额与核素活度 (Specific Activity / Yield)"
            norm = "每居里/贝克勒尔或每束流积分强度"
        else:
            obs = ObservableType.GENERAL_TRANSPORT
            obs_desc = "通用粒子输运物理量"
            norm = "每源粒子 (Per Event)"

        # 2. 辐射源空间角分布对称性 (Source Symmetry)
        is_collimated = bool(re.search(r"(?:准直|collimat|pencil|平行束|垂直|单向|打靶|束流|beam|proton beam|electron beam)", query_text, re.IGNORECASE))
        is_isotropic_point = bool(re.search(
            r"(?:点源|point\s*source|各向同性|isotrop|4\s*pi|4pi|cs-?137|co-?60|na-?22|am-?241|eu-?152|ba-?133)",
            query_text,
            re.IGNORECASE,
        ))

        if is_isotropic_point and not is_collimated:
            sym = SourceSymmetry.ISOTROPIC_4PI
            sym_desc = "各向同性放射性点源 (4π 球面度立体角测度)"
        elif is_collimated:
            sym = SourceSymmetry.DIRECTIONAL_BEAM
            sym_desc = "定向准直束流 / 笔束 (Directional Beam)"
        elif any(w in t for w in ["宇宙线", "天顶角", "cosmic", "2pi"]) or re.search(r"2\s*pi", t):
            sym = SourceSymmetry.COSMIC_2PI
            sym_desc = "上半球余弦天顶角分布 (Cosmic 2π)"
        elif any(w in t for w in ["相空间", "phase space", "iaea"]):
            sym = SourceSymmetry.PHASE_SPACE
            sym_desc = "预设外部相空间文件 (Phase Space File)"
        else:
            sym = SourceSymmetry.CUSTOM
            sym_desc = "自定义源项分布"

        # 3. 时间演化测度 (Timing Clock)
        if any(w in t for w in ["tof", "飞行时间", "全局时钟", "global time", "符合测量", "coincidence"]):
            clock = TimingClock.GLOBAL_LAB_CLOCK
        elif any(w in t for w in ["瞬态", "脉冲", "flash", "pulse", "微秒", "us", "ns", "纳秒"]):
            clock = TimingClock.TRANSIENT_PULSE
        else:
            clock = TimingClock.TIME_INDEPENDENT

        # 4. 减方差技巧声明 (Variance Reduction)
        vr = bool(re.search(r"(?:减方差|方差缩减|深穿透|重要抽样|分裂|轮盘赌|权窗|variance\s*reduction|weight\s*window|splitting|roulette|importance)", query_text, re.IGNORECASE))

        return PhysicsIntent(
            observable=obs,
            source_symmetry=sym,
            timing_clock=clock,
            variance_reduction=vr,
            observable_desc=obs_desc,
            source_desc=sym_desc,
            normalization_basis=norm,
            extraction_source="语义模式解析 (离线高泛化回退)",
        )
