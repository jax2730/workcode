# Copilot Instructions — cityarea

## Project Overview

This is a Python-based city GIS data processing project.

- Main processing modules: `admin_process/`, `city_process/`, `overpass_query/`
- Data and artifacts: `citydata/`, `picture/`, `test/`
- Utility/config files: `constants.py`, `README.md`, `.vscode/`, `.claude/`

## Primary Conventions

- Keep changes minimal and task-focused.
- Prefer reusable functions over ad-hoc scripts when logic is shared.
- Preserve existing Chinese/English comment style.
- Do not commit API keys, tokens, or local machine secrets.
- When changing behavior, validate with the smallest relevant script/test first.

## Skills & Local References

When tasks match the domain below, consult local resources in this workspace first:

1. `Agent-Skills-for-Context-Engineering-main/skills/`
	- Use for context engineering, evaluation, memory, multi-agent patterns, and tool design.
2. `.github/*.instructions.md`
	- Use project-local behavior rules for implementation and review.

## Execution Preference

- Default workflow: inspect -> plan -> implement -> verify.
- Avoid unrelated refactors.
- Keep file paths and naming conventions consistent with existing repository style.
