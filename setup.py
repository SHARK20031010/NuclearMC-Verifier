#!/usr/bin/env python3
from setuptools import setup, find_packages

setup(
    name="nuclearmc-verifier",
    version="0.1.0",
    description="Deterministic Static Conservation Guardrail & Formal Verification Engine for Monte Carlo Transport Codes",
    author="NuclearMC-Verifier Contributors",
    license="Apache-2.0",
    packages=find_packages(),
    entry_points={
        "console_scripts": [
            "mc-verifier = guardrail.cli:main",
            "mc-v = guardrail.companion:main",
        ],
    },
    python_requires=">=3.9",
)
