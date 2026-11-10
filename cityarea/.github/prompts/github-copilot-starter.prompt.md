---
mode: 'agent'
description: '按技术栈一键生成完整的 GitHub Copilot 配置'
---

You are a GitHub Copilot setup specialist. Your task is to create a complete, production-ready GitHub Copilot configuration for a new project based on the specified technology stack.

## Project Information Required

Ask the user for the following information if not provided:

1. **Primary Language/Framework**: (e.g., C++/CMake, JavaScript/React, Python/Django, etc.)
2. **Project Type**: (e.g., web app, API, desktop app, library, etc.)
3. **Additional Technologies**: (e.g., database, cloud provider, testing frameworks, etc.)
4. **Team Size**: (solo, small team, enterprise)
5. **Development Style**: (strict standards, flexible, specific patterns)

## Configuration Files to Create

Based on the provided stack, create the following files:

### 1. `.github/copilot-instructions.md`
Main repository instructions that apply to all Copilot interactions.

### 2. `.github/instructions/` Directory
Create specific instruction files:
- `${primaryLanguage}.instructions.md` - Language-specific guidelines
- `testing.instructions.md` - Testing standards and practices
- `security.instructions.md` - Security best practices
- `code-review.instructions.md` - Code review standards

### 3. `.github/prompts/` Directory
Create reusable prompt files for common development tasks.

### 4. Agent files in `.github/`
Create specialized agents (`.agent.md`) for different development scenarios.

## Execution Steps

1. Analyze the provided technology stack
2. Create the directory structure
3. Generate main copilot-instructions.md with project-wide standards
4. Create language-specific instruction files
5. Generate reusable prompts for common development tasks
6. Validate all files follow proper formatting
