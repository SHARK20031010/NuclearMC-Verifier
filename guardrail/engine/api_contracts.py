#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
api_contracts.py —— Geant4 语义级 API 契约与计算物理不变量检查器

解决核心痛点：
将原先分散在几十条孤立正则规则中的 API 契约与时序陷阱，提炼为一套完全与业务题目解耦的
Geant4 框架语义检查引擎。覆盖时序时钟、步进生命周期、过程追溯、边界条件、权重记账等
核心不可压缩盲区。
"""

import re
from typing import Dict, List, Any


def strip_cpp_comments(src: str) -> str:
    """剔除 C++ 单行与多行注释，保证检查只针对实际可执行代码。"""
    src = re.sub(r"/\*.*?\*/", " ", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


class APIContractChecker:
    def __init__(self):
        pass

    def check_all(self, code_text: str) -> List[Dict[str, Any]]:
        clean_code = strip_cpp_comments(code_text)
        issues = []
        issues.extend(self.check_process_provenance(clean_code))
        issues.extend(self.check_time_metrics(clean_code))
        issues.extend(self.check_stepping_secondary_lifecycle(clean_code))
        issues.extend(self.check_boundary_crossing_step_point(clean_code))
        issues.extend(self.check_variance_reduction_scoring(clean_code))
        issues.extend(self.check_container_event_lifecycle(clean_code))
        issues.extend(self.check_point_source_isotropy(clean_code))
        issues.extend(self.check_gaussian_broadening_fwhm(clean_code))
        issues.extend(self.check_disk_area_sampling(clean_code))
        issues.extend(self.check_photoneutron_physics(clean_code))
        return issues

    def check_process_provenance(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：次级核反应道溯源必须使用 track->GetCreatorProcess()。"""
        issues = []
        # 匹配试图通过 GetProcessDefinedStep 追溯次级核生成过程的代码
        if re.search(r"GetProcessDefinedStep", code):
            # 检查上下文是否在统计产物核或次级粒子
            is_secondary_context = bool(re.search(r"(?:isotope|nucleus|target|creator|parent|yield|reaction|secondary)", code, re.IGNORECASE))
            if is_secondary_context and not re.search(r"GetCreatorProcess", code):
                issues.append({
                    "contract": "PROCESS_PROVENANCE_CONTRACT",
                    "severity": "CRITICAL",
                    "api_used": "GetProcessDefinedStep()",
                    "api_expected": "track->GetCreatorProcess()",
                    "description": "Geant4 内核中，step->GetPostStepPoint()->GetProcessDefinedStep() 返回的是限制当前步长的推进过程（通常为能损过程 ionIoni 或 NoProcess），而非生成该粒子的物理反应道！用于追溯次级核生成反应时将导致反应道字典全空。",
                    "remedy": "改用次级粒子的 G4Track 指针查询创建过程：const G4VProcess* proc = track->GetCreatorProcess(); if (proc) G4String name = proc->GetProcessName();"
                })
        return issues

    def check_time_metrics(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：绝对飞行时间 (TOF) 与探测器到达时刻必须读取 GetGlobalTime()。"""
        issues = []
        is_tof_context = bool(re.search(r"(?:tof|flight|arrival|prompt|time_of_flight)", code, re.IGNORECASE))
        has_local_time = bool(re.search(r"GetLocalTime", code))
        has_global_time = bool(re.search(r"GetGlobalTime", code))

        if is_tof_context and has_local_time and not has_global_time:
            issues.append({
                "contract": "TIME_METRICS_CONTRACT",
                "severity": "CRITICAL",
                "api_used": "GetLocalTime()",
                "api_expected": "GetGlobalTime()",
                "description": "在飞行时间 (TOF) 或到达时间测量中，GetLocalTime() 仅记录当前径迹自生成以来的局部寿命，对于初级粒子发射到探测器的绝对飞行物理时间完全丢失。",
                "remedy": "改从步点读取绝对全局时钟：G4double tof = step->GetPostStepPoint()->GetGlobalTime();"
            })
        return issues

    def check_stepping_secondary_lifecycle(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：在 SteppingAction 中统计次级粒子空间产生顶点或产额，必须过滤首步。"""
        issues = []
        # 仅截取 UserSteppingAction 函数体内部代码（兼容 override / const 等修饰符）
        m_step = re.search(r"UserSteppingAction\s*\([^)]*\)[^{]*\{([\s\S]*?)\n\s*\}", code)
        if not m_step:
            return issues
        step_body = m_step.group(1)

        has_step1_check = bool(re.search(r"GetCurrentStepNumber\s*\(\s*\)\s*==\s*1", step_body))

        # 检查是否记录次级强子/粒子产生深度、顶点或累加计数
        is_secondary_tally = bool(
            re.search(r"(?:GetTrackID\s*\(\s*\)\s*>\s*1|secondary).*?(?:gVertices|vert|count|prod|yield|\+\+)", step_body, re.IGNORECASE | re.S) or
            re.search(r"(?:vertex|creation|birth|prod).*?(?:depth|z|pos|profile)", step_body, re.IGNORECASE | re.S)
        )

        if is_secondary_tally and not has_step1_check:
            issues.append({
                "contract": "STEPPING_SECONDARY_LIFECYCLE_CONTRACT",
                "severity": "HIGH",
                "api_expected": "track->GetCurrentStepNumber() == 1",
                "description": "在 UserSteppingAction 中统计次级粒子产生位置分布或产额时，次级粒子在靶中输运推进的每个步点均被反复记录，导致空间产生密度或计数虚高 1~2 个数量级。",
                "remedy": "在记录次级粒子产生位置前增加首步生命周期判定：if (track->GetCurrentStepNumber() == 1) { /* 仅在产生第一步记账 */ }"
            })
        return issues

    def check_boundary_crossing_step_point(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：几何边界跨越必须在 PostStepPoint 上读取 fGeomBoundary。"""
        issues = []
        if re.search(r"GetPreStepPoint\s*\(\s*\)->GetStepStatus\s*\(\s*\)\s*==\s*fGeomBoundary", code):
            issues.append({
                "contract": "GEOMETRY_BOUNDARY_CONTRACT",
                "severity": "HIGH",
                "api_used": "GetPreStepPoint()->GetStepStatus() == fGeomBoundary",
                "api_expected": "GetPostStepPoint()->GetStepStatus() == fGeomBoundary",
                "description": "步进的几何边界判定是沿粒子运动方向前瞻完成的。后步点 (PostStepPoint) 才能反映本步是否恰好停在物理体积边界上。PreStepPoint 只有在粒子刚越过边界后的下一步才可能标记，极易漏判跨界面粒子流。",
                "remedy": "将边界判断改写在后步点：if (step->GetPostStepPoint()->GetStepStatus() == fGeomBoundary) { ... }"
            })
        return issues

    def check_variance_reduction_scoring(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：使用重要性分裂/方差缩减时，计分器累加必须乘入粒子权重 GetWeight()。"""
        issues = []
        has_vr = bool(re.search(r"G4GeometrySampler|G4WeightWindow|importance|split|roulette", code, re.IGNORECASE))
        has_scoring = bool(re.search(r"(?:edep|flux|current|tally|dose|count)\s*\+=", code, re.IGNORECASE))
        has_weight = bool(re.search(r"GetWeight", code))

        if has_vr and has_scoring and not has_weight:
            issues.append({
                "contract": "VARIANCE_REDUCTION_WEIGHT_CONTRACT",
                "severity": "CRITICAL",
                "api_expected": "track->GetWeight()",
                "description": "代码中启用了几何重要性分裂或方差缩减机制，但计分累加直接使用事件计数或无权重物理量，未乘入粒子动态权重 GetWeight()，导致高重要度区域通量高估数倍到数十倍。",
                "remedy": "在每次计分累加时严格乘入权重：flux += 1.0 * step->GetTrack()->GetWeight(); 或 edep += step->GetTotalEnergyDeposit() * step->GetTrack()->GetWeight();"
            })
        return issues

    def check_container_event_lifecycle(self, code: str) -> List[Dict[str, Any]]:
        """API 契约：跨径迹去重容器必须在 Event 级别调用 clear()。"""
        issues = []
        # 检测全局/类成员容器 std::set 或 std::vector 用于 trackId 去重
        has_track_set = bool(re.search(r"std::(?:set|unordered_set)<(?:G4int|int)>\s*(?:g_)?\w*track", code, re.IGNORECASE))
        has_event_clear = bool(re.search(r"(?:BeginOfEventAction|EndOfEventAction).*?\.clear\s*\(", code, re.S))

        if has_track_set and not has_event_clear:
            issues.append({
                "contract": "CONTAINER_EVENT_LIFECYCLE_CONTRACT",
                "severity": "HIGH",
                "api_expected": "container.clear() in BeginOfEventAction",
                "description": "用于初级/次级径迹 ID 去重的容器未在每个事件的 BeginOfEventAction 中清空，导致后续事件中相同 TrackID 的有效粒子被当作历史重复粒子错误丢弃，统计量随事件数递减。",
                "remedy": "在 UserEventAction::BeginOfEventAction 中对径迹去重容器调用 .clear()。"
            })
        return issues

    def check_point_source_isotropy(self, code: str) -> List[Dict[str, Any]]:
        """几何物理契约：点源必须在 4pi 立体角内各向同性抽样。"""
        issues = []
        is_point_source = bool(re.search(r"(?:point|isotrop|点源).*?(?:source|放射源)", code, re.IGNORECASE))
        has_fixed_momentum = bool(re.search(r"SetParticleMomentumDirection\s*\(\s*G4ThreeVector\s*\(\s*0\s*,\s*0\s*,\s*1\s*\)\s*\)", code))
        has_random_dir = bool(re.search(r"G4RandomDirection|cosTheta|sinTheta", code))

        if is_point_source and has_fixed_momentum and not has_random_dir:
            issues.append({
                "contract": "SOURCE_GEOMETRY_CONTRACT",
                "severity": "CRITICAL",
                "expected": "4pi isotropic sampling",
                "description": "注释或需求声明为点辐射源，但粒子发射方向被硬编码为固定的单向平行束 (0,0,1)。点源在全立体角各向同性发射，固定单向束会导致探测器接收立体角人为放大数十倍，峰效率超几何物理上限。",
                "remedy": "调用 Geant4 内置各向同性抽样函数：particleGun->SetParticleMomentumDirection(G4RandomDirection());"
            })
        return issues

    def check_gaussian_broadening_fwhm(self, code: str) -> List[Dict[str, Any]]:
        """数学物理契约：高斯能量展宽 FWHM 与标准差 sigma 转换必须除以 2.355。"""
        issues = []
        has_broadening = bool(re.search(r"(?:G4RandGauss|CLHEP::RandGauss).*?(?:shoot|shootEngine)", code))
        has_fwhm = bool(re.search(r"(?:fwhm|resolution|sigma)", code, re.IGNORECASE))
        has_2355 = bool(re.search(r"2\.355", code))

        if has_broadening and has_fwhm and not has_2355:
            issues.append({
                "contract": "MATHEMATICAL_BROADENING_CONTRACT",
                "severity": "MEDIUM",
                "expected": "sigma = FWHM / 2.355",
                "description": "能谱高斯展宽抽样中混淆了半高宽 (FWHM) 与标准差 (sigma)。高斯分布下 FWHM = 2*sqrt(2*ln(2))*sigma ≈ 2.355*sigma。直接使用 FWHM 抽样会导致谱峰展宽人为虚高 2.355 倍。",
                "remedy": "在传入高斯抽样前对 FWHM 进行标准差归一化：G4double sigma = fwhm / 2.355; G4double e_smeared = G4RandGauss::shoot(e_true, sigma);"
            })
        return issues

    def check_disk_area_sampling(self, code: str) -> List[Dict[str, Any]]:
        """抽样拓扑契约：圆形面源径向均匀抽样必须开方 r = R * sqrt(xi)。"""
        issues = []
        has_disk = bool(re.search(r"(?:disk|circle|beam_spot|圆面源|束斑)", code, re.IGNORECASE))
        # 查找形如 r = R * G4UniformRand() 且无 sqrt 的模式
        has_linear_r = bool(re.search(r"(?:radius|r)\s*=\s*\w*\s*\*\s*G4UniformRand\s*\(\s*\)", code))
        has_sqrt = bool(re.search(r"sqrt\s*\(\s*G4UniformRand", code))

        if has_disk and has_linear_r and not has_sqrt:
            issues.append({
                "contract": "SAMPLING_JACOBIAN_CONTRACT",
                "severity": "HIGH",
                "expected": "r = R * std::sqrt(G4UniformRand())",
                "description": "平面圆形面源的微分面元为 dA = r*dr*dphi，径向累积概率密度为 P(r) = (r/R)^2。线性抽样 r = R*xi 遗漏了雅可比行列式开方，导致粒子在圆心密集聚拢、边缘稀疏，靶面光斑强度严重畸变。",
                "remedy": "修改径向抽样为面积均匀法则：G4double r = R * std::sqrt(G4UniformRand());"
            })
        return issues

    def check_photoneutron_physics(self, code: str) -> List[Dict[str, Any]]:
        """引擎完整性契约：高能光子打靶产生光核中子必须显式挂载 G4PhotonuclearPhysics。"""
        issues = []
        is_photoneutron = bool(re.search(r"(?:photo-?neutron|光核中子|gamma.*?(?:neutron|inelastic)|(n,gamma))", code, re.IGNORECASE))
        has_photonuc_phys = bool(re.search(r"G4PhotonuclearPhysics", code))

        if is_photoneutron and not has_photonuc_phys:
            issues.append({
                "contract": "PHYSICS_CONSTRUCTOR_OMISSION",
                "severity": "CRITICAL",
                "expected": "RegisterPhysics(new G4PhotonuclearPhysics())",
                "description": "Geant4 标准电磁物理包 (G4EmStandardPhysics) 不包含光核与轻子核诱发强子物理。代码涉及光致中子发射但物理列表中未挂载 G4PhotonuclearPhysics，光核中子产额将静默为 0。",
                "remedy": "在物理列表构造中显式注册光核物理构造器：RegisterPhysics(new G4PhotonuclearPhysics());"
            })
        return issues


if __name__ == "__main__":
    checker = APIContractChecker()
    sample = """
    void UserSteppingAction(const G4Step* step) {
        auto proc = step->GetPostStepPoint()->GetProcessDefinedStep()->GetProcessName();
        G4double t = step->GetPostStepPoint()->GetLocalTime();
    }
    """
    import json
    print(json.dumps(checker.check_all(sample), indent=2, ensure_ascii=False))
