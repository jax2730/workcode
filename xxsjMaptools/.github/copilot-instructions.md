# Copilot Instructions — KarstResConverter

## Project Overview

This is a **C++17 CMake project** (KarstResConverter) — a map/GIS data processing tool using ImGui + DirectX 11 for the GUI layer. It handles karst data conversion, road mesh generation, city/line/mask data processing, and grid-based map selection.

### Tech Stack
- **Language:** C++17
- **Build System:** CMake 3.10+
- **GUI:** Dear ImGui (DirectX 11 backend)
- **Image Processing:** FreeImage
- **JSON:** nlohmann/json (header-only)
- **Platform:** Windows (Win32 + DX11)

### Key Files
- `main.cpp` — Application entry, ImGui window loop, grid selection UI
- `WindowTool.cpp/.h` — GUI tool windows and helpers
- `ConvertKarstData.cpp` — Karst data format conversion
- `CreateRoadMesh.cpp` — Road mesh generation
- `ProcCityData.cpp` — City data processing
- `ProcLineData.cpp` — Line data processing  
- `ProcMaskData.cpp` — Mask data processing
- `Task.h` / `KarstDataHelper.h` — Shared types and helpers
- `GIS/` — Node.js GIS coordinate conversion utilities

## Agent Skills System

This project has two skills libraries available for structured development workflows:

### Superpowers Skills (Development Workflow)
Located in `superpowers-main/skills/`. These enforce disciplined development practices:

| Skill | File | When to Use |
|-------|------|-------------|
| brainstorming | `superpowers-main/skills/brainstorming/SKILL.md` | Before any creative work — creating features, building components |
| writing-plans | `superpowers-main/skills/writing-plans/SKILL.md` | When you have a spec and need to create implementation plan |
| executing-plans | `superpowers-main/skills/executing-plans/SKILL.md` | When implementing tasks from a plan |
| test-driven-development | `superpowers-main/skills/test-driven-development/SKILL.md` | Before writing any implementation code |
| systematic-debugging | `superpowers-main/skills/systematic-debugging/SKILL.md` | When encountering any bug or unexpected behavior |
| verification-before-completion | `superpowers-main/skills/verification-before-completion/SKILL.md` | Before declaring any fix or feature complete |
| subagent-driven-development | `superpowers-main/skills/subagent-driven-development/SKILL.md` | For dispatching parallel implementation tasks |
| requesting-code-review | `superpowers-main/skills/requesting-code-review/SKILL.md` | After completing implementation tasks |
| receiving-code-review | `superpowers-main/skills/receiving-code-review/SKILL.md` | When responding to code review feedback |
| using-git-worktrees | `superpowers-main/skills/using-git-worktrees/SKILL.md` | For parallel development branches |
| finishing-a-development-branch | `superpowers-main/skills/finishing-a-development-branch/SKILL.md` | When tasks are complete, merge/PR decision |
| writing-skills | `superpowers-main/skills/writing-skills/SKILL.md` | When creating new skills |

### Context Engineering Skills (Agent Architecture)
Located in `Agent-Skills-for-Context-Engineering-main/skills/`. These cover AI agent system design:

| Skill | File | When to Use |
|-------|------|-------------|
| context-fundamentals | `Agent-Skills-for-Context-Engineering-main/skills/context-fundamentals/SKILL.md` | Understanding context windows and attention |
| context-degradation | `Agent-Skills-for-Context-Engineering-main/skills/context-degradation/SKILL.md` | Diagnosing context-related failures |
| context-compression | `Agent-Skills-for-Context-Engineering-main/skills/context-compression/SKILL.md` | Reducing token usage, summarizing conversations |
| context-optimization | `Agent-Skills-for-Context-Engineering-main/skills/context-optimization/SKILL.md` | Optimizing context window usage |
| multi-agent-patterns | `Agent-Skills-for-Context-Engineering-main/skills/multi-agent-patterns/SKILL.md` | Designing multi-agent systems |
| memory-systems | `Agent-Skills-for-Context-Engineering-main/skills/memory-systems/SKILL.md` | Implementing agent memory, knowledge graphs |
| tool-design | `Agent-Skills-for-Context-Engineering-main/skills/tool-design/SKILL.md` | Designing agent tools and MCP tools |
| filesystem-context | `Agent-Skills-for-Context-Engineering-main/skills/filesystem-context/SKILL.md` | File-based context management |
| hosted-agents | `Agent-Skills-for-Context-Engineering-main/skills/hosted-agents/SKILL.md` | Building background/hosted agent systems |
| evaluation | `Agent-Skills-for-Context-Engineering-main/skills/evaluation/SKILL.md` | Building test/evaluation frameworks |
| advanced-evaluation | `Agent-Skills-for-Context-Engineering-main/skills/advanced-evaluation/SKILL.md` | LLM-as-judge techniques |
| project-development | `Agent-Skills-for-Context-Engineering-main/skills/project-development/SKILL.md` | LLM project development methodology |
| bdi-mental-states | `Agent-Skills-for-Context-Engineering-main/skills/bdi-mental-states/SKILL.md` | BDI cognitive architecture for agents |

## Workflow Guidelines

1. **Before implementing a new feature:** Read the brainstorming skill, explore requirements, then create a plan.
2. **Before writing code:** Read the TDD skill. Write tests first when applicable.
3. **When debugging:** Read the systematic-debugging skill. Find root cause before fixing.
4. **Before declaring done:** Read the verification-before-completion skill. Verify everything works.
5. **Save plans to:** `docs/plans/YYYY-MM-DD-<topic>.md`

## Coding Conventions

- Use `#define _CRT_SECURE_NO_WARNINGS` for legacy C function usage
- ImGui patterns: follow existing `WindowTool.cpp` patterns for new UI windows
- Use `std::filesystem` for path handling
- Use `nlohmann::json` for JSON serialization
- Chinese comments are acceptable (existing codebase style)
- Prefer RAII patterns for DirectX resource management
