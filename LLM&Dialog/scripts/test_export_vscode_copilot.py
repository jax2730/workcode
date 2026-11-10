from __future__ import annotations

import json
import os
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import export_vscode_copilot as exporter


SESSION_ID = "4fd7988d-65c8-448f-aa4d-46a937cd258e"


class CopilotExportContractTests(unittest.TestCase):
    def setUp(self) -> None:
        self.cache = (
            Path(os.environ["APPDATA"])
            / "Code"
            / "User"
            / "workspaceStorage"
            / "7bcf7c1a744b81d162804b819bedb9c1"
            / "chatSessions"
            / f"{SESSION_ID}.jsonl"
        )
        if not self.cache.exists():
            self.skipTest(f"VS Code cache not found: {self.cache}")
        self.metadata, self.requests, _ = exporter.parse_session(self.cache)

    def test_first_turn_preserves_rendered_context_and_tool_rounds(self) -> None:
        turn = exporter.build_canonical_turn(SESSION_ID, 1, self.requests[0])

        self.assertEqual(turn["requestId"], "request_ac03882a-d726-420c-ae8e-f5400f484d10")
        self.assertIn("Persona API 完整测试指南", turn["user"]["renderedText"])
        self.assertGreaterEqual(len(turn["user"]["attachments"]), 2)
        self.assertGreaterEqual(len(turn["assistant"]["toolRounds"]), 3)
        self.assertEqual(
            turn["assistant"]["toolRounds"][0]["toolCalls"][0]["name"],
            "read_file",
        )
        self.assertIn("persona_api.py", json.dumps(turn, ensure_ascii=False))

    def test_session_request_count_matches_incremental_jsonl_state(self) -> None:
        self.assertEqual(len(self.requests), 133)

    def test_ui_visible_text_matches_reference_style_for_first_turn(self) -> None:
        text = exporter.ui_visible_response_text(self.requests[0])

        self.assertIn("我来阅读文档和代码", text)
        self.assertIn("Persona API 的接口逻辑", text)
        self.assertIn("views.py", text)
        self.assertIn("parse_request_data()", text)
        self.assertNotIn("copilot_readFile", text)
        self.assertNotIn("toolInvocationSerialized", text)


if __name__ == "__main__":
    unittest.main()
