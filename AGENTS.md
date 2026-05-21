# AGENTS.md

This is the Codex entry file for the project.

<!-- memoc:managed:start -->
## Session Start
- [ ] Read `.memoc/session-summary.md`
- [ ] `.pending` exists? Review changed files, update memory if needed, then delete it.
- [ ] If `memoc` is not found, use the project-local wrapper for the rest of the session: Windows `.\.memoc\bin\memoc.cmd <command>`; sh `.memoc/bin/memoc <command>`

## Before Opening More Files
- [ ] Search memory first: `memoc search "<query>" --limit 5`, or wrapper fallback above if PATH fails
- [ ] Open on demand: `02` status, `04` resume, `06` rules, `llms.txt` map
- [ ] If memory search is not enough, search project files with `memoc grep "<query>" --limit 5` (or wrapper fallback)
- [ ] Keep output small: `summary`, `search --limit`, `grep --limit`, `--snippets`

## Before Finishing _(update only applicable files; skip Q&A / throwaway exploration)_
- [ ] Code/config/deps changed? Update `02` + `session-summary.md`
- [ ] Decision made? Update `03-decisions.md` + `02`
- [ ] Work incomplete or risky? Update `04-handoff.md`
- [ ] Rule/preference set? Update `06-project-rules.md`
- [ ] Wiki/systems work? Read `skills/project-memory-maintainer/SKILL.md`
<!-- memoc:managed:end -->
