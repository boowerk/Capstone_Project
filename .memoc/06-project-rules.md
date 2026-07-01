---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-05-21T07:03:24
status: active
tags:
  - memoc
  - memoc/state
---
# Project Rules

Durable user and project preferences live here. Update when the user gives a rule that should persist across sessions.

## Operating Rules

- Keep `AGENTS.md` and `CLAUDE.md` as short entry files; durable context belongs under `.memoc/`.
- Do not track generated output folders such as `out/`, `.next/`, `dist/`, `build/` unless the user explicitly asks.
- Update `.memoc/04-handoff.md` after substantial work so the next agent can resume quickly.
- Use `.memoc/05-done-checklist.md` before saying substantial work is complete.

## Agent Behavior Preferences

- Be factual and operational in memory docs.
- Keep logs concise; do not paste temporary command output unless it changes future work.
- Preserve user changes and avoid reverting unrelated work.
- State unverified parts honestly in the final answer and handoff.

## Project-Specific Rules

- When Codex performs implementation work, split commits by functional unit.
- Follow the repository's commit title style from history: `type(scope): short summary`, such as `feat(ui): add native tab navigation for character stats menu`.
- After C++ changes, Codex should run the normal local/Rider-compatible build so the user does not wait on first editor launch. Before building, check for `UnrealEditor.exe`; if it is running, close it first, then build. If a build fails because Live Coding/editor/game is active, close the editor/game and rerun the same build command. For `Project_Eden`, use the generated Rider/UE project-file engine path, currently `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat`, not `C:\Program Files\Epic Games\UE_5.7\...`; mixing engine paths can invalidate external plugin outputs and cause repeated `McpAutomationBridge` rebuilds. Do not use MCP-triggered rebuild commands. If the normal editor target emits MCP plugin log lines because the project has that plugin enabled, treat that as normal project build output, not as an MCP-triggered rebuild.
