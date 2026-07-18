---
memoc: true
type: state
scope: project-memory
created: 2026-05-21T07:03:24
updated: 2026-07-18T19:49:02+09:00
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
- Use the current `[EDEN-MAIN]` thread as the sole writer, verifier, stager, and committer. Role threads are read-only advisers unless the main thread explicitly delegates a bounded write.
- Route cross-role questions and reviews through the main thread so every decision uses the same Git snapshot and protected-file list.

## Agent Behavior Preferences

- Be factual and operational in memory docs.
- Keep logs concise; do not paste temporary command output unless it changes future work.
- Preserve user changes and avoid reverting unrelated work.
- State unverified parts honestly in the final answer and handoff.

## Project-Specific Rules

- When Codex performs implementation work, split commits by functional unit.
- Split every task into the smallest independently reviewable and reversible functional units. As a default, one implementation unit produces one commit, including its directly related tests.
- Never mix unrelated refactors, formatting, documentation, or binary asset changes into a functional commit. Stage explicit file paths only after reviewing the current status and diff.
- When adding or modifying code, leave comments that explain the relevant intent or constraint.
- Follow the repository's commit title style from history: `type(scope): short summary`, such as `feat(ui): add native tab navigation for character stats menu`.
- After C++ changes, Codex should run the normal local/Rider-compatible build so the user does not wait on first editor launch. Before building, check for `UnrealEditor.exe`; if it is running, close it first, then build. If a build fails because Live Coding/editor/game is active, close the editor/game and rerun the same build command. For `Project_Eden`, use the generated Rider/UE project-file engine path, currently `D:\Engine\Windows\Engine\Build\BatchFiles\Build.bat`, not `C:\Program Files\Epic Games\UE_5.7\...`; mixing engine paths can invalidate external plugin outputs and cause repeated `McpAutomationBridge` rebuilds. Do not use MCP-triggered rebuild commands. If the normal editor target emits MCP plugin log lines because the project has that plugin enabled, treat that as normal project build output, not as an MCP-triggered rebuild.
- For the current graduation slice, do not launch additional live dedicated/listen multi-client tests unless the user explicitly asks. Keep server integration functional and verify it with normal builds and local automation.
