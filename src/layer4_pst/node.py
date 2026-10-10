"""
Layer 4: Physics Semantic Tree (PST) Node Definition
"""

from typing import List, Dict, Any, Optional
from pydantic import BaseModel, Field
from ..layer1_ast.parser import ASTSpan
from ..layer3_semantic.schema import ActionType, ConservationDomain, LifecycleTier


class PSTNode(BaseModel):
    """A typed physical operation node in the Physics Semantic Tree."""
    node_id: str
    action: ActionType
    physical_operation: str
    physical_quantity: str
    conservation_domain: ConservationDomain
    expected_invariant: str
    code_evidence: str
    confidence: float = 1.0

    observable: Optional[str] = None
    unit_dimension: Optional[str] = None
    domain_scope: Optional[str] = None
    lifecycle_tier: LifecycleTier = "StepLevel"

    ast_span: Optional[ASTSpan] = None
    guard_conditions: List[str] = Field(default_factory=list)

    def to_dict(self) -> Dict[str, Any]:
        return self.model_dump()
