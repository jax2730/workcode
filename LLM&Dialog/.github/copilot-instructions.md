# Project Copilot Instructions

## Scope

These instructions apply to the whole repository and are optimized for this workspace's mixed stack (Python + C++ + docs + agent skills).

## Priority

1. Follow repository root AGENTS.md and existing project conventions first.
2. Then apply task-specific files in `.github/instructions/*.instructions.md`.
3. For shadcn UI and engineering workflow requests, apply `.github/instructions/copilot-skill-bridge.instructions.md` and read mapped skill `SKILL.md` files from `ui-main/skills` and `superpowers-main/skills`.
4. Keep changes minimal and avoid unrelated refactors.

## Working style

- Prefer reading existing code patterns before implementing new code.
- For non-trivial tasks, propose or maintain a short actionable plan.
- Validate changes with the smallest relevant checks first, then broader checks if needed.
- Do not modify generated/binary/build artifacts unless explicitly requested.

## Python

- Follow PEP8, add type hints where practical, and keep functions focused.
- Preserve existing public function signatures unless change is required.
- Use clear error messages and avoid broad `except` unless justified.

## C++

- Respect current style and project structure.
- Keep headers/sources consistent and avoid introducing style drift.

## Documentation

- Keep docs aligned with actual behavior and API flow.
- When API behavior changes, update examples and sequence descriptions.

## Safety

- Never assume external service availability; surface clear failure guidance.
- Prefer deterministic, testable changes over speculative rewrites.
