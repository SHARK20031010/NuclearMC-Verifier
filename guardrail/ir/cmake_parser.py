#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
guardrail/ir/cmake_parser.py —— CMakeLists.txt 与 compile_commands.json 工程解析器

用于提取 C++ 蒙卡仿真工程的：
1. 编译目标 (Executable Targets)
2. 真实编译单元源文件列表 (Source Files, 避免扫描废弃测试脚本)
3. 编译宏定义 (Definitions, 如 -DG4MULTITHREADED)
4. 头文件包含路径 (Include Directories)
"""

from __future__ import annotations

import os
import re
import json
import glob
from pathlib import Path
from typing import Dict, List, Optional, Any, Set
from pydantic import BaseModel, Field


class CMakeTarget(BaseModel):
    """CMake 可执行文件构建目标"""
    target_name: str
    sources: List[str] = Field(default_factory=list)
    include_dirs: List[str] = Field(default_factory=list)
    definitions: List[str] = Field(default_factory=list)


class CMakeProjectInfo(BaseModel):
    """CMake 蒙卡仿真工程元数据"""
    project_dir: str
    project_name: Optional[str] = None
    cmake_file: Optional[str] = None
    compile_commands_path: Optional[str] = None
    targets: List[CMakeTarget] = Field(default_factory=list)
    all_source_files: List[str] = Field(default_factory=list)
    include_dirs: List[str] = Field(default_factory=list)
    definitions: List[str] = Field(default_factory=list)


class CMakeParser:
    """CMake 构建系统分析引擎"""

    def parse_project(self, project_path: str) -> CMakeProjectInfo:
        """分析指定目录或指定 CMakeLists.txt 文件"""
        p = Path(project_path).resolve()
        if p.is_file():
            if p.name == "CMakeLists.txt":
                project_dir = p.parent
                cmake_file = p
            elif p.suffix in (".cc", ".cpp"):
                # 单文件作为单一 target
                return CMakeProjectInfo(
                    project_dir=str(p.parent),
                    project_name=p.stem,
                    targets=[CMakeTarget(target_name=p.stem, sources=[str(p)])],
                    all_source_files=[str(p)],
                )
            else:
                project_dir = p.parent
                cmake_file = project_dir / "CMakeLists.txt"
        else:
            project_dir = p
            cmake_file = project_dir / "CMakeLists.txt"

        # 1. 优先尝试加载标准 compile_commands.json
        comp_db = self._find_compile_commands(project_dir)
        if comp_db:
            info = self._parse_compile_commands(comp_db, project_dir)
            if info and info.all_source_files:
                return info

        # 2. 如果存在 CMakeLists.txt，解析其内容
        if cmake_file.exists():
            return self._parse_cmake_file(cmake_file, project_dir)

        # 3. 兜底策略：搜寻当前目录或 src/ 下的直接 .cc / .cpp
        direct_sources = (
            list(project_dir.glob("*.cc"))
            + list(project_dir.glob("*.cpp"))
            + list((project_dir / "src").glob("*.cc"))
            + list((project_dir / "src").glob("*.cpp"))
        )
        filtered = sorted(list(set(str(f.resolve()) for f in direct_sources if f.is_file())))
        if not filtered:
            raise FileNotFoundError(f"未在工程目录 {project_dir} 找到 CMakeLists.txt 或直接 C++ 源码文件。")

        return CMakeProjectInfo(
            project_dir=str(project_dir),
            project_name=project_dir.name,
            targets=[CMakeTarget(target_name=project_dir.name, sources=filtered)],
            all_source_files=filtered,
        )

    def find_enclosing_project(self, file_or_dir_path: str) -> Optional[CMakeProjectInfo]:
        """
        向上智能探测单文件或子目录所归属的 CMake 工程。
        支持从深层子目录 (如 my_project/src/SteppingAction.cc) 自动定位根级 CMakeLists.txt，
        解析整个多文件工程，避免单文件孤立核验导致的跨模块误报。
        """
        try:
            p = Path(file_or_dir_path).resolve()
        except Exception:
            return None

        if not p.exists():
            return None

        start_dir = p if p.is_dir() else p.parent
        curr = start_dir
        for _ in range(5):
            cmake_candidate = curr / "CMakeLists.txt"
            if cmake_candidate.is_file():
                try:
                    info = self.parse_project(str(curr))
                    if info and info.all_source_files:
                        p_str = str(p)
                        # 如果目标是具体文件，验证其属于工程源码或处于工程目录树下
                        if p.is_file():
                            if p_str in info.all_source_files or p_str.startswith(str(curr)):
                                return info
                        else:
                            return info
                except Exception:
                    pass
            if curr.parent == curr:
                break
            curr = curr.parent

        return None

    def _find_compile_commands(self, project_dir: Path) -> Optional[Path]:
        """寻找 compile_commands.json"""
        candidates = [
            project_dir / "compile_commands.json",
            project_dir / "build" / "compile_commands.json",
            project_dir / "build_bins" / "compile_commands.json",
        ]
        for c in candidates:
            if c.exists():
                return c
        # 搜寻 build* 文件夹
        for p in project_dir.glob("build*/compile_commands.json"):
            if p.exists():
                return p
        return None

    def _parse_compile_commands(self, comp_db_path: Path, project_dir: Path) -> Optional[CMakeProjectInfo]:
        """解析编译数据库 compile_commands.json"""
        try:
            with open(comp_db_path, "r", encoding="utf-8") as f:
                entries = json.load(f)
            sources: List[str] = []
            includes: Set[str] = set()
            definitions: Set[str] = set()

            for item in entries:
                file_path = item.get("file")
                if not file_path:
                    continue
                abs_file = Path(file_path)
                if not abs_file.is_absolute():
                    abs_file = Path(item.get("directory", project_dir)) / abs_file
                if abs_file.suffix in (".cc", ".cpp", ".cxx"):
                    sources.append(str(abs_file.resolve()))

                # 提取编译参数中的 -I 与 -D
                cmd = item.get("command", "") or " ".join(item.get("arguments", []))
                for m in re.finditer(r"-I\s*(\S+)", cmd):
                    includes.add(m.group(1))
                for m in re.finditer(r"-D\s*(\S+)", cmd):
                    definitions.add(m.group(1))

            unique_sources = sorted(list(set(sources)))
            return CMakeProjectInfo(
                project_dir=str(project_dir),
                project_name=project_dir.name,
                compile_commands_path=str(comp_db_path),
                targets=[CMakeTarget(
                    target_name=project_dir.name,
                    sources=unique_sources,
                    include_dirs=sorted(list(includes)),
                    definitions=sorted(list(definitions)),
                )],
                all_source_files=unique_sources,
                include_dirs=sorted(list(includes)),
                definitions=sorted(list(definitions)),
            )
        except Exception:
            return None

    def _parse_cmake_file(self, cmake_file: Path, project_dir: Path) -> CMakeProjectInfo:
        """轻量化递归分析 CMakeLists.txt 内容"""
        text = cmake_file.read_text(encoding="utf-8", errors="replace")
        # 移除单行注释
        text = re.sub(r"#.*", "", text)

        # 1. 项目名称
        proj_name = project_dir.name
        m_proj = re.search(r"project\s*\(\s*([a-zA-Z0-9_\-]+)", text, re.IGNORECASE)
        if m_proj:
            proj_name = m_proj.group(1)

        # 预先替换 CMake 内置常用路径变量
        p_dir_str = str(project_dir)
        text = text.replace("${PROJECT_SOURCE_DIR}", p_dir_str)
        text = text.replace("${CMAKE_CURRENT_SOURCE_DIR}", p_dir_str)
        text = text.replace("${CMAKE_SOURCE_DIR}", p_dir_str)
        text = text.replace("${PROJECT_NAME}", proj_name)
        text = text.replace("${PROJECT_BINARY_DIR}", str(project_dir / "build"))
        text = text.replace("${CMAKE_BINARY_DIR}", str(project_dir / "build"))

        # 2. 提取 set(...) 变量赋值
        variables: Dict[str, List[str]] = {}
        for m in re.finditer(r"set\s*\(\s*([a-zA-Z0-9_\-]+)\s+([^)]+)\)", text, re.IGNORECASE):
            var_name = m.group(1)
            raw_vals = m.group(2).split()
            variables[var_name] = [v.strip("\"'") for v in raw_vals]

        # 3. 提取 file(GLOB ...)
        for m in re.finditer(r"file\s*\(\s*GLOB(?:_RECURSE)?\s+([a-zA-Z0-9_\-]+)\s+([^)]+)\)", text, re.IGNORECASE):
            var_name = m.group(1)
            patterns = m.group(2).split()
            matched_files: List[str] = []
            for pat in patterns:
                pat = pat.strip("\"'")
                # 如果是相对路径
                full_pat = str(project_dir / pat) if not os.path.isabs(pat) else pat
                matched_files.extend(glob.glob(full_pat, recursive=True))
            variables[var_name] = matched_files

        # 4. 提取宏定义 (add_definitions / target_compile_definitions)
        definitions: Set[str] = set()
        for m in re.finditer(r"(?:add_definitions|target_compile_definitions)\s*\([^)]*?(-D[a-zA-Z0-9_\=]+|G4[a-zA-Z0-9_]+)", text, re.IGNORECASE):
            definitions.add(m.group(1))

        # 5. 提取包含目录 (include_directories / target_include_directories)
        include_dirs: Set[str] = set()
        for m in re.finditer(r"(?:include_directories|target_include_directories)\s*\([^)]*?\s+([^)]+)\)", text, re.IGNORECASE):
            for part in m.group(1).split():
                clean_dir = part.strip("\"'")
                if clean_dir and not clean_dir.startswith(("-", "$<", "PRIVATE", "PUBLIC", "INTERFACE")):
                    p_inc = project_dir / clean_dir if not os.path.isabs(clean_dir) else Path(clean_dir)
                    if p_inc.exists():
                        include_dirs.add(str(p_inc.resolve()))

        # 常规 include 目录
        if (project_dir / "include").exists():
            include_dirs.add(str((project_dir / "include").resolve()))

        # 6. 提取 add_executable(target_name src1 src2 ...)
        targets: List[CMakeTarget] = []
        all_sources: Set[str] = set()

        for m in re.finditer(r"add_executable\s*\(\s*([a-zA-Z0-9_\-]+)\s+([^)]+)\)", text, re.IGNORECASE):
            t_name = m.group(1)
            raw_srcs = m.group(2).split()
            resolved_srcs: List[str] = []

            for s in raw_srcs:
                s = s.strip("\"'")
                # 检查是否为变量引用 ${VAR}
                m_var = re.match(r"\$\{([a-zA-Z0-9_\-]+)\}", s)
                if m_var and m_var.group(1) in variables:
                    for v in variables[m_var.group(1)]:
                        p_s = self._resolve_source_path(v, project_dir)
                        if p_s and Path(p_s).suffix in (".cc", ".cpp", ".cxx"):
                            resolved_srcs.append(p_s)
                elif s.endswith((".cc", ".cpp", ".cxx")):
                    resolved_srcs.append(self._resolve_source_path(s, project_dir))

            valid_srcs = [s for s in resolved_srcs if s and Path(s).exists()]
            all_sources.update(valid_srcs)
            targets.append(CMakeTarget(
                target_name=t_name,
                sources=valid_srcs,
                include_dirs=sorted(list(include_dirs)),
                definitions=sorted(list(definitions)),
            ))

        # 若未找到明确的 add_executable，自动搜寻 src/ 和当前目录中的 C++ 文件
        if not all_sources:
            for s in project_dir.glob("*.cc"):
                all_sources.add(str(s.resolve()))
            for s in (project_dir / "src").glob("*.cc"):
                all_sources.add(str(s.resolve()))
            targets.append(CMakeTarget(
                target_name=proj_name,
                sources=sorted(list(all_sources)),
                include_dirs=sorted(list(include_dirs)),
                definitions=sorted(list(definitions)),
            ))

        sorted_sources = sorted(list(all_sources))
        return CMakeProjectInfo(
            project_dir=str(project_dir),
            project_name=proj_name,
            cmake_file=str(cmake_file),
            targets=targets,
            all_source_files=sorted_sources,
            include_dirs=sorted(list(include_dirs)),
            definitions=sorted(list(definitions)),
        )

    def _resolve_source_path(self, raw_path: str, project_dir: Path) -> str:
        """解析相对或绝对源码文件路径"""
        p = Path(raw_path)
        if p.is_absolute():
            return str(p)
        # 尝试直接相对 project_dir
        if (project_dir / p).exists():
            return str((project_dir / p).resolve())
        # 尝试相对 src/
        if (project_dir / "src" / p).exists():
            return str((project_dir / "src" / p).resolve())
        return str((project_dir / p).resolve())
