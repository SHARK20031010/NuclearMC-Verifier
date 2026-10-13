#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tests/test_cross_file_linking.py
自动化验证跨文件 PST 全局图分析落地与三大语义纽带：
1. 跨模块调用分发与权重流连接 (Call Graph & WeightFlow)
2. 几何实体与死层作用域纽带 (Volume Scope & Dead Layer Containment)
3. Geant4 生命周期层级与跨事件重置 (Lifecycle Hierarchy & Container Reset)
4. 单文件向上自动探测归属 CMake 工程 (Parent Project Auto-Discovery)
"""

import os
import shutil
import tempfile
from pathlib import Path
import pytest

from guardrail.ir.cmake_parser import CMakeParser
from guardrail.ir.pst_linker import PSTLinker
from guardrail.ir.pst_verifier import PSTVerifier
from guardrail.ir.pst_schema import PSTEdgeType, PhysicalAction, ConservationDomain
from guardrail.companion import MCCompanion


@pytest.fixture
def mock_geant4_cmake_project():
    """创建一个标准 5 模块 Geant4 CMake 模拟工程目录"""
    temp_dir = tempfile.mkdtemp(prefix="g4_pst_proj_")
    proj_path = Path(temp_dir)
    src_path = proj_path / "src"
    src_path.mkdir(parents=True, exist_ok=True)

    # 1. CMakeLists.txt
    cmake_text = """
cmake_minimum_required(VERSION 3.16)
project(SimShield)

find_package(Geant4 REQUIRED)
include(${Geant4_USE_FILE})

file(GLOB_RECURSE SOURCES "src/*.cc")
add_executable(SimShield SimShield.cc ${SOURCES})
target_link_libraries(SimShield ${Geant4_LIBRARIES})
"""
    (proj_path / "CMakeLists.txt").write_text(cmake_text, encoding="utf-8")

    # 2. DetectorConstruction.cc (定义敏感靶与死层)
    detector_text = """
#include "G4LogicalVolume.hh"
#include "G4Box.hh"

void DetectorConstruction::Construct() {
    auto solidDet = new G4Box("SolidDet", 10, 10, 10);
    auto logicDetector = new G4LogicalVolume(solidDet, mat, "Detector");
    
    auto solidDead = new G4Box("SolidDead", 10, 10, 0.5);
    auto logicDead = new G4LogicalVolume(solidDead, mat, "DeadLayer");
}
"""
    (src_path / "DetectorConstruction.cc").write_text(detector_text, encoding="utf-8")

    # 3. PrimaryGeneratorAction.cc (4pi 各向同性点放射源)
    primary_text = """
#include "G4ParticleGun.hh"
#include "G4RandomDirection.hh"

void PrimaryGeneratorAction::GeneratePrimaries(G4Event* anEvent) {
    // 4pi 各向同性发射 Cs-137 点源
    fParticleGun->SetParticleMomentumDirection(G4RandomDirection());
    fParticleGun->GeneratePrimaryVertex(anEvent);
}
"""
    (src_path / "PrimaryGeneratorAction.cc").write_text(primary_text, encoding="utf-8")

    # 4. SensitiveDetector.cc (计分抓取能量与权重)
    sd_text = """
#include "G4Step.hh"

G4bool SensitiveDetector::ProcessHits(G4Step* step, G4TouchableHistory*) {
    G4double edep = step->GetTotalEnergyDeposit();
    G4double weight = step->GetTrack()->GetWeight();
    if (edep > 0.) {
        fHitsCollection->insert(new MyHit(edep, weight));
    }
    return true;
}
"""
    (src_path / "SensitiveDetector.cc").write_text(sd_text, encoding="utf-8")

    # 5. RunAction.cc (Run 级累加与重置)
    run_text = """
#include "G4Run.hh"

void RunAction::BeginOfRunAction(const G4Run*) {
    fRunFlux = 0.0;
}

void RunAction::AddFlux(G4double flux, G4double weight) {
    fRunFlux += flux * weight;
}
"""
    (src_path / "RunAction.cc").write_text(run_text, encoding="utf-8")

    # 6. EventAction.cc (Event 级重置)
    event_text = """
#include "G4Event.hh"

void EventAction::BeginOfEventAction(const G4Event*) {
    fEventEdep = 0.0;
}
"""
    (src_path / "EventAction.cc").write_text(event_text, encoding="utf-8")

    # 7. SteppingAction.cc (Step 级：调用 RunAction::AddFlux 传递权重)
    stepping_text = """
#include "G4Step.hh"

void SteppingAction::UserSteppingAction(const G4Step* step) {
    G4double w = step->GetTrack()->GetWeight();
    fRunAction->AddFlux(1.0, w);
}
"""
    (src_path / "SteppingAction.cc").write_text(stepping_text, encoding="utf-8")

    yield proj_path

    # 清理
    shutil.rmtree(temp_dir, ignore_errors=True)


def test_cross_file_linking_end_to_end(mock_geant4_cmake_project):
    """测试多模块工程全量编译与三大纽带跨文件拓扑缝合"""
    parser = CMakeParser()
    proj_info = parser.parse_project(str(mock_geant4_cmake_project))

    assert len(proj_info.all_source_files) == 6

    linker = PSTLinker()
    global_graph = linker.link_project(proj_info)

    # 1. 验证全局图节点合并
    assert len(global_graph.nodes) > 0
    # 2. 验证跨模块边生成
    cross_weight_edges = [e for e in global_graph.edges if e.edge_type == PSTEdgeType.WEIGHT_FLOW]
    assert len(cross_weight_edges) > 0

    containment_edges = [e for e in global_graph.edges if e.edge_type == PSTEdgeType.VOLUME_CONTAINMENT]
    assert len(containment_edges) > 0

    reset_edges = [e for e in global_graph.edges if e.edge_type == PSTEdgeType.LIFECYCLE_RESET]
    assert len(reset_edges) > 0

    hierarchy_edges = [e for e in global_graph.edges if e.edge_type == PSTEdgeType.LIFECYCLE_HIERARCHY]
    assert len(hierarchy_edges) > 0

    # 3. 验证全局守恒断言 100% 绿灯通过
    verifier = PSTVerifier()
    report = verifier.verify(global_graph)
    assert report.passed is True
    assert report.violations_count == 0


def test_parent_project_auto_discovery(mock_geant4_cmake_project):
    """测试从子目录单文件向上自动探测外层 CMake 工程"""
    stepping_file = mock_geant4_cmake_project / "src" / "SteppingAction.cc"

    companion = MCCompanion()
    envelope = companion.inspect(str(stepping_file))

    # 验证自动切换为跨文件全局图模式
    assert envelope.is_project is True
    assert envelope.enclosing_project_detected is True
    assert len(envelope.source_files) == 6
    assert envelope.passed is True
    assert envelope.cross_edges_count > 0


def test_detects_inter_event_stale_tally_leak(mock_geant4_cmake_project):
    """测试跨文件捕获事件级累加器未在 BeginOfEventAction 重置的物理缺陷"""
    event_file = mock_geant4_cmake_project / "src" / "EventAction.cc"
    # 故意移除 BeginOfEventAction 中的重置语句，增加事件累加
    buggy_event_text = """
#include "G4Event.hh"

void EventAction::BeginOfEventAction(const G4Event*) {
    // 缺陷：忘记清零 fEventEdep，导致跨事件累积残留污染！
}

void EventAction::EndOfEventAction(const G4Event*) {
    fEventEdep += 10.0;
}
"""
    event_file.write_text(buggy_event_text, encoding="utf-8")

    companion = MCCompanion()
    envelope = companion.inspect(str(mock_geant4_cmake_project))

    assert envelope.passed is False
    assert any(v.rule_id == "PST-INV-LIFECYCLE-03" for v in envelope.violations)
    viol = next(v for v in envelope.violations if v.rule_id == "PST-INV-LIFECYCLE-03")
    assert "BeginOfEventAction" in viol.description
