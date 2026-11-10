# Project Skills for Codex

This project has a local skill library in `skills/`. Treat that directory as the canonical source for project-specific skills.

The local `skills/` directory is also exposed for Codex discovery through these Windows junctions:

- `C:\Users\Pixel\.codex\skills\llm-dialog-project`
- `C:\Users\Pixel\.agents\skills\llm-dialog-project`

If native skill discovery has not refreshed yet, load skills directly from this repository with:

```powershell
Get-Content -Raw "skills\<skill-name>\SKILL.md"
```

For the collection overview skill, read `skills\SKILL.md`.

Do not use `openskills read` in this project. These skills are plain Codex-compatible `SKILL.md` folders.

## Skill Loading Rules

- Before meaningful work, check whether a skill below applies.
- If a skill applies, read only that skill's `SKILL.md` first.
- Load referenced files such as `references/`, `rules/`, `scripts/`, or `assets/` only when the skill says they are relevant.
- Prefer project-local skills over generic memory when the task overlaps with a listed skill.
- Announce the skill briefly when using it, then follow its workflow.
- Keep unrelated skills out of context.

## Available Skills

### Context Engineering

- `context-engineering-collection`: overview of the local context engineering skill collection at `skills/SKILL.md`.
- `context-fundamentals`: use for explaining or designing effective LLM context.
- `context-degradation`: use for diagnosing lost-in-middle, context poisoning, distraction, or context rot.
- `context-compression`: use for summarizing, compacting, or preserving long-running agent sessions.
- `context-optimization`: use for improving token efficiency, context partitioning, or retrieval strategy.
- `filesystem-context`: use for file-system-as-memory patterns and durable context handoff.
- `memory-systems`: use for agent memory, vector stores, knowledge graphs, or persistence design.
- `multi-agent-patterns`: use for orchestrators, supervisors, swarms, and sub-agent architectures.
- `tool-design`: use for designing deterministic tools and agent tool contracts.
- `hosted-agents`: use for hosted/background coding agents and sandbox infrastructure.
- `evaluation`: use for evaluating agent behavior or LLM pipelines.
- `advanced-evaluation`: use for production-grade evaluation systems, rubrics, bias checks, and metrics.
- `project-development`: use for planning LLM-powered applications and staged processing pipelines.
- `bdi-mental-states`: use for belief-desire-intention and cognitive architecture modeling.

### Development Workflow

- `using-superpowers`: use as the entry point for the Superpowers workflow library.
- `brainstorming`: use before open-ended design, feature planning, or UX/product exploration.
- `systematic-debugging`: use for bugs, test failures, regressions, or unexplained behavior.
- `test-driven-development`: use before implementing features or bug fixes that need tests.
- `verification-before-completion`: use before claiming a fix or implementation is complete.
- `writing-plans`: use to turn an approved spec into a concrete implementation plan.
- `executing-plans`: use when executing an existing implementation plan.
- `subagent-driven-development`: use when a plan can be split into independent agent-sized tasks.
- `dispatching-parallel-agents`: use when independent tasks can run in parallel.
- `requesting-code-review`: use before asking for or performing a review of completed work.
- `receiving-code-review`: use when applying review feedback.
- `using-git-worktrees`: use when isolated feature work or parallel branches are needed.
- `finishing-a-development-branch`: use when implementation is complete and the branch needs final handling.
- `writing-skills`: use when creating or improving skills.

### UI

- `shadcn`: use for shadcn/ui projects, component registries, presets, `components.json`, component installation, styling, debugging, and composition.

### Documentation

- `mini-wiki`: use to generate or update a professional project wiki in `.mini-wiki/`, including architecture docs, module/API docs, Mermaid diagrams, source references, incremental updates, and mini-wiki plugin management.

## Folder Selection Note

In the Codex UI, `@` may feel like it only selects one folder at a time. You do not need to rely on `@` for this project:

- Start Codex from the repository root `E:\C++pandan\workcode\LLM&Dialog` whenever possible.
- If you want to point at several folders, mention them as plain paths in the prompt, for example `ui-main`, `Agent-Skills-for-Context-Engineering-main`, and `superpowers-main`.
- You can also insert multiple folder mentions one by one with `@`.
- The configured skills now live under `skills/`, so future prompts can simply say "use the project skills" or name a skill such as `shadcn` or `systematic-debugging`.


<claude-mem-context>
# Memory Context

# [LLM&Dialog] recent context, 2026-05-08 2:49pm GMT+8

No previous sessions found.
</claude-mem-context>