#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
lifecycle_core.py —— 步进生命周期与边界时序总线 (Stepping Lifecycle & Boundary Core)
覆盖 Geant4 底层粒子输运微步态机：径迹诞生、首步过滤、过程追溯、边界跨越、微步长限制与多敏感体积解耦。
"""

import re
from typing import Dict, List, Any


def strip_cpp_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class LifecycleCore:
    """生命周期时序总线：拦截产生顶点全步点重复累加、过程溯源误读、边界判定时点错位、初生动能读错后步点等引擎陷阱。"""

    def audit(self, code_text: str) -> List[Dict[str, Any]]:
        clean = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self._check_secondary_production_step1_filter(clean))
        issues.extend(self._check_process_provenance_method(clean))
        issues.extend(self._check_boundary_crossing_step_point(clean))
        issues.extend(self._check_container_event_lifecycle(clean))
        issues.extend(self._check_dead_layer_energy_leakage(clean))
        issues.extend(self._check_primary_kinetic_energy_timing(clean))
        issues.extend(self._check_multi_cell_volume_decoupling(clean))
        issues.extend(self._check_nanoparticle_interface_step_limit(clean))
        return issues

    def _check_secondary_production_step1_filter(self, clean: str) -> List[Dict[str, Any]]:
        """步进生命周期契约：在 SteppingAction 中统计次级粒子产生位置/产额，必须过滤首步 StepNumber == 1。"""
        issues = []
        m_step = re.search(r"UserSteppingAction\s*\([^)]*\)[^{]*\{([\s\S]*?)\n\s*\}", clean)
        if not m_step:
            return issues
        body = m_step.group(1)

        has_step1_check = bool(re.search(r"GetCurrentStepNumber\s*\(\s*\)\s*==\s*1|GetStepNumber\s*\(\s*\)\s*==\s*1", body))

        is_secondary_tally = bool(
            re.search(r"(?:GetParentID\s*\(\s*\)\s*>\s*0|ParentID\s*!=\s*0|GetTrackID\s*\(\s*\)\s*>\s*1)", body) and
            re.search(r"(?:vertex|vert|count|prod|yield|birth|gSecondary|PromptGammas|\+\+|\+=)", body, re.IGNORECASE)
        )

        if is_secondary_tally and not has_step1_check:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "STEPPING_SECONDARY_LIFECYCLE_CONTRACT",
                "severity": "CRITICAL",
                "defect_name": "次级粒子空间产生顶点未过滤产生首步 (全步点重复累加虚高)",
                "description": "在 UserSteppingAction 中对次级粒子（如次级强子、散裂中子、核退激 4.44 MeV 瞬发伽马）进行空间产生深度分布或产额统计时，未限制 GetCurrentStepNumber() == 1。次级粒子诞生后在介质中向前输运的几十到几百个连续步点全部被无差别当作『产生点』累加，导致空间产生密度分布严重畸变，产额虚高 1~2 个数量级。",
                "remedy": "增加首步产生生命周期判定：if (track->GetCurrentStepNumber() == 1) { /* 仅在诞生首步记录顶点与统计产额 */ }"
            })
        return issues

    def _check_process_provenance_method(self, clean: str) -> List[Dict[str, Any]]:
        """物理过程追溯契约：追溯生成次级核/粒子的物理反应道，必须调用 track->GetCreatorProcess()。"""
        issues = []
        if re.search(r"GetProcessDefinedStep", clean):
            is_secondary_context = bool(re.search(r"(?:isotope|nucleus|target|creator|parent|yield|reaction|secondary|残余核|产物核)", clean, re.IGNORECASE))
            has_creator_proc = bool(re.search(r"GetCreatorProcess", clean))

            if is_secondary_context and not has_creator_proc:
                issues.append({
                    "core": "LIFECYCLE_CORE",
                    "contract": "PROCESS_PROVENANCE_CONTRACT",
                    "severity": "CRITICAL",
                    "defect_name": "产物核/次级粒子生成物理过程溯源误用 GetProcessDefinedStep()",
                    "description": "代码试图通过 step->GetPostStepPoint()->GetProcessDefinedStep() 查询生成某个残余核或次级粒子的物理核反应道。在 Geant4 输运内核中，GetProcessDefinedStep() 返回的是『限制并推进当前输运步的物理过程』（如电离 ionIoni），而非『创造该粒子的母体物理过程』！用于追溯核反应道时将导致产物核输出完全为空或误识别为连续能损。",
                    "remedy": "改用次级粒子的径迹指针查询创建过程：const G4VProcess* proc = track->GetCreatorProcess(); if (proc) G4String name = proc->GetProcessName();"
                })
        return issues

    def _check_boundary_crossing_step_point(self, clean: str) -> List[Dict[str, Any]]:
        """几何边界跨越契约：出射穿透边界判定必须且只能在 PostStepPoint 上读取 fGeomBoundary。"""
        issues = []
        has_pre_boundary = bool(re.search(r"GetPreStepPoint\s*\(\s*\)->GetStepStatus\s*\(\s*\)\s*==\s*fGeomBoundary", clean))
        if has_pre_boundary:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "GEOMETRY_BOUNDARY_CONTRACT",
                "severity": "HIGH",
                "defect_name": "几何边界跨越误在 PreStepPoint 上判定导致漏粒子",
                "description": "Geant4 导航仪在计算几何交点时，是将步长截断在几何体外表面，并将 fGeomBoundary 状态严格标记在当前步的后步点 (PostStepPoint)。PreStepPoint 只有在粒子进入下一体积的下一步时才可能反映旧状态，极易漏判跨界面出射粒子流（漏计率可超 90%）。",
                "remedy": "将边界判断严格改写在后步点：if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) { /* 成功出射 */ }"
            })
        return issues

    def _check_container_event_lifecycle(self, clean: str) -> List[Dict[str, Any]]:
        """跨事件生命周期契约：径迹去重容器必须在 Event 级别调用 clear()。"""
        issues = []
        has_track_set = bool(re.search(r"std::(?:set|unordered_set)<(?:G4int|int)>\s*(?:g_)?\w*track", clean, re.IGNORECASE))
        has_event_clear = bool(re.search(r"(?:BeginOfEventAction|EndOfEventAction)[\s\S]*?\.clear\s*\(", clean))

        if has_track_set and not has_event_clear:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "CONTAINER_EVENT_LIFECYCLE_CONTRACT",
                "severity": "HIGH",
                "defect_name": "径迹 ID 去重容器跨事件未清空导致后续事件粒子丢失",
                "description": "用于记录已被击中或统计过的粒子 TrackID 的去重容器（如 std::set<int>）未在每个事件的 BeginOfEventAction 中调用 clear() 清空。不同事件中的粒子 TrackID 均从 1 开始重新分配，跨事件未清空会导致后续事件中 TrackID 相同的有效新粒子被当作历史重复粒子被静默忽略。",
                "remedy": "在 UserEventAction::BeginOfEventAction 中显式清空去重容器：fTrackSet.clear();"
            })
        return issues

    def _check_dead_layer_energy_leakage(self, clean: str) -> List[Dict[str, Any]]:
        """半导体死层隔离契约：探测器前表面死层沉积的能量绝不能并入晶体灵敏区有效电脉冲。"""
        issues = []
        is_dead_layer = bool(re.search(r"(?:dead_layer|eDead|deadLayer|死层)", clean, re.IGNORECASE))
        has_pulse_leakage = bool(re.search(r"(?:eCry|e_active|edep)\s*\+\s*(?:eDead|deadLayer)", clean, re.IGNORECASE) or
                                re.search(r"eCry\s*\+\s*eDead\s*>\s*0", clean))

        if is_dead_layer and has_pulse_leakage:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "DETECTOR_DEAD_LAYER_ISOLATION_CONTRACT",
                "severity": "HIGH",
                "defect_name": "半导体探测器死层沉积能量并入有效电信号",
                "description": "探测器表面钝化层或死层（Dead Layer）中的电子-空穴对复合严重，不会形成有效感应电荷输出。代码中将死层能量与晶体活性区能量相加形成输出脉冲，导致低能特征 X 射线或低穿透粒子能量被虚假并入全能峰。",
                "remedy": "严格剥离死层沉积能量：输出脉冲幅度仅计灵敏区沉积 eCry，死层能量 eDead 仅用于材料吸收记账，不得参与脉冲谱输出。"
            })
        return issues

    def _check_primary_kinetic_energy_timing(self, clean: str) -> List[Dict[str, Any]]:
        """初生粒子动能抓取时序契约：初生能量必须在 PreStepPoint 读取，严禁在首步 PostStepPoint 读取。"""
        issues = []
        # 检测在首步从 PostStepPoint 获取初生动能导致分箱下坠
        has_post_step_ek = bool(re.search(r"GetCurrentStepNumber\s*\(\s*\)\s*==\s*1[\s\S]*?GetPostStepPoint\s*\(\s*\)->GetKineticEnergy\s*\(\s*\)", clean) or
                                re.search(r"GetPostStepPoint\s*\(\s*\)->GetKineticEnergy\s*\(\s*\)[\s\S]*?(?:spectrum|hist|谱)", clean))
        has_pre_step_ek = bool(re.search(r"GetPreStepPoint\s*\(\s*\)->GetKineticEnergy\s*\(\s*\)|GetVertexKineticEnergy", clean))

        if has_post_step_ek and not has_pre_step_ek:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "PRIMARY_KINETIC_ENERGY_TIMING_CONTRACT",
                "severity": "HIGH",
                "defect_name": "初级粒子初生动能误在 PostStepPoint 读取导致能谱分箱下坠",
                "description": "在首步推进中，带电粒子或光子已经发生了初级电离能损或轫致辐射。在 PostStepPoint 读取到的能量是经历首步能损之后的剩余动能，而非粒子诞生时刻的真初生能量，导致出射能谱整体向下漂移一个步长的能量。",
                "remedy": "改在 PreStepPoint 读取初生动能，或直接调用 track->GetVertexKineticEnergy()。"
            })
        return issues

    def _check_multi_cell_volume_decoupling(self, clean: str) -> List[Dict[str, Any]]:
        """多敏感靶区计分解耦契约：细胞群多细胞核单事件击中必须按 CopyNo 独立解耦累加。"""
        issues = []
        is_multi_cell = bool(re.search(r"(?:cells|multi_cell|多细胞|细胞群|cell_)\b", clean, re.IGNORECASE))
        has_copy_no = bool(re.search(r"GetCopyNo|copyNo|replica|map<int", clean))

        if is_multi_cell and not has_copy_no:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "MULTI_SENSITIVE_VOLUME_DECOUPLING_CONTRACT",
                "severity": "HIGH",
                "defect_name": "多细胞核敏感体积共享单一累加变量导致击中数混淆",
                "description": "在模拟微米级细胞群或阵列微结构时，多个独立的细胞核几何体被实例化放置。若计分器使用单一全局变量累加，会导致所有细胞核接收的能量被混在一起，单靶击中与多靶击中的泊松统计分布彻底失效。",
                "remedy": "在 SteppingAction 中通过 preStep->GetTouchableHandle()->GetCopyNo() 获取各微敏感体积的唯一编号，使用 std::map<int, double> 分别独立记录各细胞核的沉积能量与击中数。"
            })
        return issues

    def _check_nanoparticle_interface_step_limit(self, clean: str) -> List[Dict[str, Any]]:
        """纳米界面连续能损跳步契约：纳米金颗粒微界面必须配置微步长限制器 (StepLimiter)。"""
        issues = []
        is_nanoparticle = bool(re.search(r"(?:nano|nanoparticle|\bgold\b|\bAu\b|纳米|增敏).*?(?:nanoparticle|纳米颗粒|界面|interface)", clean, re.IGNORECASE))
        has_step_limiter = bool(re.search(r"G4UserLimits|G4StepLimiter|StepLimiter|maxStep", clean))

        if is_nanoparticle and not has_step_limiter:
            issues.append({
                "core": "LIFECYCLE_CORE",
                "contract": "NANOPARTICLE_INTERFACE_STEP_LIMIT_CONTRACT",
                "severity": "HIGH",
                "defect_name": "纳米界面缺少微步长限制器导致次级电子跳步",
                "description": "纳米颗粒（如 50 nm 金颗粒）界面附近的剂量增强效应由几十 eV 到几 keV 的俄歇电子和低能光电子主导。Geant4 默认的多重散射与连续能损步长在水介质中可达数微米甚至毫米，若未在纳米颗粒及其邻域设置纳米级最大步长限制，低能次级电子将跨步越过界面，造成界面剂量增强比 (DEF) 严重低估数倍。",
                "remedy": "在纳米颗粒逻辑体积挂载微步长限制器：logicNano->SetUserLimits(new G4UserLimits(10.0 * nm)); 并在物理列表中注册 G4StepLimiterPhysics。"
            })
        return issues
