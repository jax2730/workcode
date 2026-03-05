# CLAUDE.md — KarstResConverter Project

## Project Overview

C++17 CMake project: a map/GIS data processing tool with ImGui + DirectX 11 GUI.
Handles karst data conversion, road mesh generation, city/line/mask data processing, grid-based map selection.

**Build:** `cmake -B build && cmake --build build`
**Run:** `build/bin/Debug/KarstResConverter.exe`

## Tech Stack
- C++17, CMake 3.10+, Dear ImGui (DX11 backend), FreeImage, nlohmann/json, Win32

## Skills System

This project uses two skills libraries. **You MUST check for applicable skills before any task.**

### Superpowers Skills (in `superpowers-main/skills/`)

These are your primary development workflow skills. Use the Skill tool to invoke them.

- **brainstorming** — REQUIRED before any creative/feature work
- **writing-plans** — Create implementation plans from specs
- **executing-plans** — Execute tasks from plans
- **test-driven-development** — Write tests before code (RED-GREEN-REFACTOR)
- **systematic-debugging** — Find root cause before fixing bugs
- **verification-before-completion** — Verify before declaring done
- **subagent-driven-development** — Dispatch parallel implementation tasks
- **requesting-code-review** — Pre-review checklist
- **receiving-code-review** — Respond to code review feedback  
- **dispatching-parallel-agents** — Concurrent subagent workflows
- **using-git-worktrees** — Parallel development branches
- **finishing-a-development-branch** — Merge/PR decision workflow
- **writing-skills** — Create new skills following best practices
- **using-superpowers** — How to find and use skills

### Context Engineering Skills (in `Agent-Skills-for-Context-Engineering-main/skills/`)

Use when building, optimizing, or debugging AI agent systems:

- **context-fundamentals** — Context windows, attention mechanics
- **context-degradation** — Lost-in-middle, context poisoning
- **context-compression** — Compression strategies for long sessions
- **context-optimization** — Compaction, masking, caching
- **multi-agent-patterns** — Supervisor, peer-to-peer, hierarchical architectures
- **memory-systems** — Short/long-term memory, knowledge graphs
- **tool-design** — Build tools agents can use effectively
- **filesystem-context** — Filesystem for dynamic context discovery
- **hosted-agents** — Background coding agents in sandboxed environments
- **evaluation** — Evaluation frameworks for agent systems
- **advanced-evaluation** — LLM-as-judge, pairwise comparison
- **project-development** — LLM project development methodology
- **bdi-mental-states** — BDI cognitive architecture

## Workflow

1. Before new features → brainstorming → writing-plans → executing-plans
2. Before coding → test-driven-development
3. When debugging → systematic-debugging
4. Before completion → verification-before-completion
5. Save plans to `docs/plans/YYYY-MM-DD-<topic>.md`

## Conventions

- Chinese comments acceptable (existing codebase)
- Follow ImGui patterns from `WindowTool.cpp` for new UI
- Use `std::filesystem` for paths, `nlohmann::json` for JSON
- RAII for DirectX resources
- `#define _CRT_SECURE_NO_WARNINGS` at top of translation units
