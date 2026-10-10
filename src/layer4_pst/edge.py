"""
Layer 4: Physics Semantic Tree (PST) Edge Definition
Represents physical conservation and dataflow couplings between nodes.
"""

from typing import Literal, Dict, Any, Optional
from pydantic import BaseModel


EdgeType = Literal["DATA_DEPENDENCY", "WEIGHT_COUPLING", "LIFECYCLE_CONTAINER", "CAUSALITY_FLOW"]


class PSTEdge(BaseModel):
    """A typed directed edge in the Physics Semantic Tree."""
    source_id: str
    target_id: str
    edge_type: EdgeType
    description: str = ""
    carrier_quantity: Optional[str] = None
    is_satisfied: bool = True

    def to_dict(self) -> Dict[str, Any]:
        return self.model_dump()
