# AGENTS.md

This is the Codex entry file for the project.

<!-- context-forge:managed:start -->
## Session Start
- [ ] Read `.memoc/session-summary.md`
- [ ] `.pending` exists? → review changed files → update memory if needed → delete it
- [ ] If `memoc` is not found in an existing shell, open a new terminal or load the local helper: PowerShell `. .\.memoc\env.ps1`; sh `. ./.memoc/env.sh`

## Before Opening More Files
- [ ] Run memoc commands in this order: `memoc search "<query>"` → `.\.memoc\bin\memoc.cmd search "<query>"` (Windows) or `.memoc/bin/memoc search "<query>"` (sh) → `npx @kevin0181/memoc search "<query>"`
- [ ] Open on demand: `02` status · `04` resume · `06` rules · `llms.txt` map
- [ ] If memory search is not enough, search project files with `memoc grep "<query>" --limit 5`
- [ ] Keep output small: `summary`, `search --limit`, `grep --limit`, `--snippets`

## Before Finishing _(update only applicable files; skip Q&A / throwaway exploration)_
- [ ] Code/config/deps changed → `02` (version, commands list, Last synced) + `session-summary.md` (status, changed, open tasks)
- [ ] Decision made → `03-decisions.md` (what & why) + `02`
- [ ] Work incomplete or risky → `04-handoff.md` (verified commands, unverified items, next steps)
- [ ] Rule/preference set → `06-project-rules.md`
- [ ] Wiki/systems work → read `skills/project-memory-maintainer/SKILL.md`
<!-- context-forge:managed:end -->
