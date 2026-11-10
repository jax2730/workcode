---
description: 'Bridge external skill repositories for Copilot task execution'
applyTo: '**'
---

# Copilot Skill Bridge

Use this bridge when a request matches one of the external skill repositories in this workspace:

- `ui-main/skills/shadcn` (UI component and shadcn workflow rules)
- `superpowers-main/skills` (planning, debugging, TDD, review process skills)

## Skill Selection Rules

1. Match the user intent to the closest skill domain first.
2. Read the corresponding `SKILL.md` before implementation.
3. Treat skill checklists as execution constraints.
4. If multiple skills apply, prioritize process skills before implementation skills.

## Trigger Map

- shadcn/ui components, registry, preset, `components.json`, UI composition
  - Read: `ui-main/skills/shadcn/SKILL.md`
- planning, step breakdown, implementation plan
  - Read: `superpowers-main/skills/writing-plans/SKILL.md`
- structured debugging and root-cause analysis
  - Read: `superpowers-main/skills/systematic-debugging/SKILL.md`
- test-first or TDD requests
  - Read: `superpowers-main/skills/test-driven-development/SKILL.md`
- code review requests
  - Read: `superpowers-main/skills/requesting-code-review/SKILL.md`
- branch completion and handoff
  - Read: `superpowers-main/skills/finishing-a-development-branch/SKILL.md`

## Execution Notes

- These repositories are local workspace sources, not plugin-installed runtime skills.
- If a skill requires unavailable platform tooling, follow the same process manually and document assumptions.
- Keep implementation scoped to the current repository and avoid cross-repo edits unless explicitly requested.