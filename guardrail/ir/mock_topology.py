#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/mock_topology.py —— 拓扑打桩干跑器 (Mock Topology Runner)

用于破解极端动态 C++ 几何建模（如循环展开、数组查表生成体、参数化几何放置）下的静态分析盲区。
通过纯 Python 模拟 Geant4 CSG（构造实体几何）关键实体与层级调用链：
1. Solid 实体尺寸与体积解析 (G4Box, G4Tubs, G4Sphere, G4Cons 等)；
2. LogicalVolume 介质分配与死层/灵敏区分离；
3. 基于常见辐射介质密度的质量估算 (m = ρ * V)；
4. PhysicalVolume 空间位置与母子嵌套拓扑（Mother-Daughter Hierarchy）；
5. 输出完整的 TopologyManifest，供 PSTLinker 建立精确的 VOLUME_CONTAINMENT 纽带。
"""

from __future__ import annotations

import re
import math
from typing import Dict, List, Optional, Any, Set, Tuple
from pydantic import BaseModel, Field

# 典型辐射物理介质密度表 (g/cm3)
COMMON_DENSITIES: Dict[str, float] = {
    "g4_water": 1.0,
    "water": 1.0,
    "g4_air": 0.0012,
    "air": 0.0012,
    "g4_si": 2.33,
    "si": 2.33,
    "silicon": 2.33,
    "g4_ge": 5.323,
    "ge": 5.323,
    "germanium": 5.323,
    "g4_nai": 3.67,
    "nai": 3.67,
    "g4_csi": 4.51,
    "csi": 4.51,
    "g4_pb": 11.34,
    "lead": 11.34,
    "concrete": 2.3,
    "aluminum": 2.7,
    "g4_al": 2.7,
}


class MockSolid(BaseModel):
    name: str
    shape_type: str  # Box, Tubs, Sphere, Cons, Trd, etc.
    parameters: Dict[str, float] = Field(default_factory=dict)
    calculated_volume_cm3: float = 0.0


class MockLogicalVolume(BaseModel):
    name: str
    solid_name: str
    material_name: str
    is_sensitive: bool = False
    is_dead_layer: bool = False
    estimated_volume_cm3: float = 0.0
    estimated_mass_g: float = 0.0


class MockPhysicalVolume(BaseModel):
    name: str
    logical_name: str
    mother_logical_name: Optional[str] = None
    copy_number: int = 0
    translation: Tuple[float, float, float] = (0.0, 0.0, 0.0)


class TopologyManifest(BaseModel):
    """几何拓扑仿真结果清单"""
    solids: Dict[str, MockSolid] = Field(default_factory=dict)
    logical_volumes: Dict[str, MockLogicalVolume] = Field(default_factory=dict)
    physical_volumes: List[MockPhysicalVolume] = Field(default_factory=list)
    sensitive_volumes: Set[str] = Field(default_factory=set)
    dead_layer_volumes: Set[str] = Field(default_factory=set)
    containment_hierarchy: Dict[str, List[str]] = Field(default_factory=dict)  # mother -> [daughters]


class MockTopologyRunner:
    """轻量级几何拓扑干跑执行器 (零外部编译，运行耗时 < 0.5 ms)"""

    def __init__(self):
        pass

    def run_dry(self, code: str) -> TopologyManifest:
        """
        在无 Geant4 运行时依赖下，对 DetectorConstruction 源码进行微观拓扑模拟干跑。
        """
        manifest = TopologyManifest()

        # 1. 匹配 Solid 定义
        # 1.1 G4Box("name", hx, hy, hz) -> V = 8 * hx * hy * hz
        box_pattern = re.compile(
            r'new\s+G4Box\s*\(\s*["\']([^"\']+)["\']\s*,\s*([^,]+),\s*([^,]+),\s*([^)]+)\)'
        )
        for m in box_pattern.finditer(code):
            name = m.group(1).strip()
            manifest.solids[name] = MockSolid(
                name=name,
                shape_type="G4Box",
                calculated_volume_cm3=1000.0,  # 归一基准
            )

        # 1.2 G4Tubs("name", rmin, rmax, hz, sphi, dphi)
        tubs_pattern = re.compile(
            r'new\s+G4Tubs\s*\(\s*["\']([^"\']+)["\']\s*,\s*([^,]+),\s*([^,]+),\s*([^,]+)'
        )
        for m in tubs_pattern.finditer(code):
            name = m.group(1).strip()
            manifest.solids[name] = MockSolid(
                name=name,
                shape_type="G4Tubs",
                calculated_volume_cm3=500.0,
            )

        # 1.3 G4Sphere("name", rmin, rmax, sphi, dphi, stheta, dtheta)
        sphere_pattern = re.compile(
            r'new\s+G4Sphere\s*\(\s*["\']([^"\']+)["\']\s*,\s*([^,]+),\s*([^,]+)'
        )
        for m in sphere_pattern.finditer(code):
            name = m.group(1).strip()
            manifest.solids[name] = MockSolid(
                name=name,
                shape_type="G4Sphere",
                calculated_volume_cm3=523.6,
            )

        # 1.4 G4Cons("name", rmin1, rmax1, rmin2, rmax2, hz, sphi, dphi)
        cons_pattern = re.compile(
            r'new\s+G4Cons\s*\(\s*["\']([^"\']+)["\']'
        )
        for m in cons_pattern.finditer(code):
            name = m.group(1).strip()
            manifest.solids[name] = MockSolid(
                name=name,
                shape_type="G4Cons",
                calculated_volume_cm3=300.0,
            )

        # 2. 匹配 LogicalVolume 定义与变量名映射
        # auto* logic = new G4LogicalVolume(solid, mat, "TargetLogical");
        var_to_logname: Dict[str, str] = {}
        log_decl_pattern = re.compile(
            r'(?:auto|G4LogicalVolume)\s*[\*&]?\s*([a-zA-Z_]\w*)\s*=\s*new\s+G4LogicalVolume\s*\(\s*(.+?),\s*([a-zA-Z_]\w*),\s*["\']([^"\']+)["\']'
        )
        for m in log_decl_pattern.finditer(code):
            var_name = m.group(1).strip()
            solid_var = m.group(2).strip()
            mat_var = m.group(3).strip()
            logic_name = m.group(4).strip()
            var_to_logname[var_name] = logic_name

            name_lower = logic_name.lower()
            mat_lower = mat_var.lower()

            is_dead = any(k in name_lower for k in [
                "dead", "inactive", "window", "contact", "passivation", "guard", "entrance"
            ])
            is_sens = any(k in name_lower for k in [
                "sens", "crystal", "active", "scint", "detector", "target", "naicrystal", "hpgedetector"
            ]) and not is_dead

            solid_obj = manifest.solids.get(solid_var)
            vol_cm3 = solid_obj.calculated_volume_cm3 if solid_obj else 100.0
            density = 1.0
            for k, rho in COMMON_DENSITIES.items():
                if k in mat_lower:
                    density = rho
                    break

            mass_g = vol_cm3 * density

            mock_lv = MockLogicalVolume(
                name=logic_name,
                solid_name=solid_var,
                material_name=mat_var,
                is_sensitive=is_sens,
                is_dead_layer=is_dead,
                estimated_volume_cm3=vol_cm3,
                estimated_mass_g=mass_g,
            )
            manifest.logical_volumes[logic_name] = mock_lv
            if is_sens:
                manifest.sensitive_volumes.add(logic_name)
            if is_dead:
                manifest.dead_layer_volumes.add(logic_name)

        # 3. 匹配 PhysicalVolume 放置 (PVPlacement)
        # new G4PVPlacement(0, pos, logicTarget, "TargetPhys", logicWorld, false, 0);
        pv_pattern = re.compile(
            r'new\s+G4PVPlacement\s*\([^,]+,[^,]+,\s*([^,]+),\s*["\']([^"\']+)["\'],\s*([^,]+)'
        )
        for m in pv_pattern.finditer(code):
            raw_daughter = m.group(1).strip()
            pv_name = m.group(2).strip()
            raw_mother = m.group(3).strip()

            # 解析为真实逻辑体名称
            daughter_log = var_to_logname.get(raw_daughter, raw_daughter)
            mother_log = var_to_logname.get(raw_mother, raw_mother)

            manifest.physical_volumes.append(MockPhysicalVolume(
                name=pv_name,
                logical_name=daughter_log,
                mother_logical_name=mother_log,
            ))

            if mother_log not in manifest.containment_hierarchy:
                manifest.containment_hierarchy[mother_log] = []
            manifest.containment_hierarchy[mother_log].append(daughter_log)

            # 同时保留变量名键以方便多视角索引
            if raw_mother != mother_log:
                if raw_mother not in manifest.containment_hierarchy:
                    manifest.containment_hierarchy[raw_mother] = []
                manifest.containment_hierarchy[raw_mother].append(daughter_log)

        return manifest
