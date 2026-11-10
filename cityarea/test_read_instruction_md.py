from pathlib import Path

WORKSPACE_ROOT = Path(__file__).resolve().parent

INSTRUCTION_FILES = [
    ".github/copilot-instructions.md",
    ".github/cityarea.instructions.md",
    ".claude/CLAUDE.md",
]


def check_markdown_files() -> int:
    failed = []

    for relative_path in INSTRUCTION_FILES:
        full_path = WORKSPACE_ROOT / relative_path
        if not full_path.exists():
            failed.append((relative_path, "not found"))
            continue

        try:
            content = full_path.read_text(encoding="utf-8")
        except Exception as exc:
            failed.append((relative_path, f"read failed: {exc}"))
            continue

        if not content.strip():
            failed.append((relative_path, "empty content"))
            continue

        print(f"OK: {relative_path} ({len(content)} chars)")

    if failed:
        print("\nFAILED:")
        for path, reason in failed:
            print(f"- {path}: {reason}")
        return 1

    print("\nAll markdown instruction files are readable.")
    return 0


if __name__ == "__main__":
    raise SystemExit(check_markdown_files())
