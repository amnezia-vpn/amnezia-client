# Dynamic Context Guide

Behavioral rules are defined in `AGENTS.md`.
This file describes where dynamic project knowledge lives.

## Dynamic Artifacts
- `conventions.md`: generated coding conventions.
- `structure.md`: generated architecture and file map.
- `knowledge_graph.md` / `knowledge_graph.json`: generated dependency and relationship context.
- `module_registry.md`: generated module responsibility summaries.
- `decisions/log.md`: architectural decision history.
- `memory/`: reports, findings, errors, and task traces.

## Intended Ownership
- `ag init` seeds this directory structure.
- `ag refresh` updates generated knowledge artifacts in `.antigravity/`.
- Human-authored behavior policy should stay in `AGENTS.md`, not here.
