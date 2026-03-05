---
mode: 'agent'
description: '代码重构：提取函数、消除代码异味、改善可维护性'
---

# Refactor

Improve code structure and readability without changing external behavior. Refactoring is gradual evolution, not revolution.

## Refactoring Principles

1. **Behavior is preserved** - Refactoring doesn't change what the code does, only how
2. **Small steps** - Make tiny changes, test after each
3. **Version control is your friend** - Commit before and after each safe state
4. **One thing at a time** - Don't mix refactoring with feature changes

## Common Code Smells to Fix

- **Long Method/Function** — Break into focused functions
- **Duplicated Code** — Extract common logic
- **Large Class/Module** — Single responsibility per class
- **Long Parameter List** — Group related parameters into structs
- **Magic Numbers/Strings** — Use named constants
- **Nested Conditionals** — Use guard clauses / early returns
- **Dead Code** — Remove unused functions, imports, commented code
- **Feature Envy** — Move logic to the object that owns the data

## Safe Refactoring Process

1. **PREPARE** - Ensure tests exist, commit current state
2. **IDENTIFY** - Find the code smell, understand what the code does
3. **REFACTOR** - Make one small change, verify it works, repeat
4. **VERIFY** - All tests pass, performance unchanged
5. **CLEAN UP** - Update comments and documentation

## Refactoring Checklist

- Functions are small (< 50 lines) and do one thing
- No duplicated code
- Descriptive names for variables, functions, classes
- No magic numbers/strings
- Dead code removed
- Related code is together
- Clear module boundaries
- No circular dependencies
