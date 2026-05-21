# Agent Workflow

Shared protocol for any coding agent.

## Entry Routine

1. Read the entry-file managed block.
2. Read `.memoc/session-summary.md` only.
3. Search before opening broad docs.
4. Work from the smallest relevant file set.
5. Update memory only when durable context changed.

## Memory Update Triggers

| Trigger | Update |
| --- | --- |
| User creates or changes a requirement | `02-current-project-state.md`, `06-project-rules.md`, `log.md` |
| Code, config, data, or assets changed | `02-current-project-state.md`, relevant `systems/*.md`, `log.md` |
| Architecture or system behavior changed | relevant `systems/*.md`, `03-decisions.md` |
| A decision should affect future agents | `03-decisions.md`, `02-current-project-state.md` |
| Work is substantial enough to resume later | `04-handoff.md`, `02-current-project-state.md`, `log.md` |
| Durable knowledge was learned | `wiki/*.md`, `wiki/index.md` |

## Usually No Update Needed

- Pure Q&A with no durable outcome.
- Tiny typo-only edits.
- Temporary exploration that finds nothing actionable.

## Documentation Shape

- Entry files: protocol only.
- `session-summary.md`: latest snapshot, max 3 bullets per section.
- `02-current-project-state.md`: current status, tasks, commands, recent notes.
- `04-handoff.md`: resume context, blockers, verified/unverified checks.
- `03-decisions.md`: append durable decisions only.
- `log.md`: full history; keep bulky completed work here.
- `systems/*.md` and `wiki/*.md`: on-demand durable knowledge.
