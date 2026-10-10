"""
Layer 3: Physical Operation Semantic Schema
Enforces strict Pydantic schema for structured physical semantic annotations.
"""

from typing import Optional, List, Literal
from pydantic import BaseModel, Field


ActionType = Literal["Define", "Sample", "Fetch", "Accumulate", "Convert"]
ConservationDomain = Literal["energy_momentum", "quantum_numbers", "phase_space", "variance_weight", "causality_clock", "dimension_linearity", "geometry_topology"]
LifecycleTier = Literal["StepLevel", "EventLevel", "RunLevel", "Initialization"]


class PhysicalSemanticAnnotation(BaseModel):
    """Structured physical semantic description of a code operation."""
    action: ActionType
    physical_operation: str               # e.g., 'sample_direction', 'accumulate_fluence', 'fetch_weight'
    physical_quantity: str                # e.g., 'solid_angle', 'absorbed_dose', 'track_weight', 'energy'
    conservation_domain: ConservationDomain
    expected_invariant: str               # e.g., '4pi isotropic', 'fair_game_weight_coupling', 'step_1_filter'
    code_evidence: str                    # verbatim code snippet
    confidence: float = Field(ge=0.0, le=1.0, default=1.0)
    
    # Metadata fields for graph construction
    observable: Optional[str] = None      # e.g., 'Fluence', 'AbsorbedDose', 'SecondaryYield'
    unit_dimension: Optional[str] = None  # e.g., '[L]^-2', '[M][L]^2[T]^-2', 'dimensionless'
    domain_scope: Optional[str] = None    # target volume / region name
    lifecycle_tier: LifecycleTier = "StepLevel"
    guard_conditions: List[str] = Field(default_factory=list)
