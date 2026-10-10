# AGENTS.md

Instructions for the local model (`qwen3.8-27b-ud@q4_k_s`, served via LM Studio over Tailscale) operating in this repository through Aider. This model has a **hard 98k token context window** — these rules exist to keep it inside that budget and recoverable when it isn't.

## 0. Core Constraints

- **Context budget: 98k tokens, hard limit.** Assume headroom is tighter than it looks once system prompt, diffs, and thinking tokens are counted. Thinking/reasoning mode is enabled — its output consumes context too, so budget for it explicitly, not as an afterthought.
- **One task at a time, no exceptions.** Never start a second task, refactor, or "while I'm in here" fix before the current task in `PLAN.md` is marked done and committed. If you notice unrelated issues, log them in `PLAN.md` under "Backlog" instead of acting on them.
- **Assume you will be compacted or interrupted without warning.** Every unit of work must be small enough, and `PROGRESS.md` current enough, that a fresh context window (yours or a successor's) can resume with zero loss of intent.

## 1. The Two Control Files

### `PLAN.md` — direction (what to do next)
- Read `PLAN.md` **first**, before touching any code, at the start of every session/turn.
- It contains an ordered list of tasks. Only the **top unchecked item** is in scope right now.
- Do not reorder, expand, or add tasks to `PLAN.md` yourself unless explicitly asked — you consume it, the human curates it. Exception: the "Backlog" section at the bottom, where you may append observations for the human to triage later.
- When a task completes, check it off in `PLAN.md` in the same commit that finishes the work.

### `PROGRESS.md` — recovery state (what has happened)
- Update `PROGRESS.md` **before and after every meaningful step**, not just at the end of a task. Treat it as a checkpoint log, not a changelog.
- On every write, overwrite stale sections rather than appending indefinitely — this file must stay small enough to read in one shot after a compaction.
- Required structure:

```markdown
# PROGRESS.md

## Current Task
<exact task text copied from PLAN.md, verbatim>

## State
<one of: NOT_STARTED | IN_PROGRESS | BLOCKED | READY_FOR_REVIEW>

## What's Done So Far
- <concrete, verifiable step> (files touched: ...)
- <concrete, verifiable step> (files touched: ...)

## Next Immediate Step
<the single next action — specific enough that no re-reading of history is needed>

## Open Questions / Blockers
<anything that stopped forward progress, or "none">

## Last Known-Good State
<e.g. "all tests passing as of commit <sha>" or "untested, do not assume working">
```

- At the **start** of any session: read `PROGRESS.md` in full before reading anything else except `PLAN.md`. If `PROGRESS.md` says `IN_PROGRESS`, resume from "Next Immediate Step" — do not restart the task from scratch or re-derive context by re-reading the whole codebase.
- If you are about to run out of context (long diff, large file, deep investigation), **stop and write `PROGRESS.md` first**, even mid-task. An incomplete task with an accurate checkpoint is recoverable; a lost context window with a stale checkpoint is not.

## 2. Context Budget Discipline

- Prefer targeted reads (specific files, specific functions, `grep`-style search) over broad exploration. Do not read entire large files when a function or section will do.
- Do not re-read files already summarized in `PROGRESS.md` unless the summary is insufficient to proceed — trust your own prior checkpoint.
- Keep diffs minimal and scoped to the current task only. A task that touches many files is a sign it should be split in `PLAN.md`'s Backlog, not a reason to push through in one pass.
- If thinking/reasoning output is running long on a single step, cut it short and act — this model's reasoning budget is part of the 98k ceiling, not separate from it.
- Summarize, don't quote. When referencing earlier work in `PROGRESS.md` or commit messages, describe it in a sentence rather than pasting large code blocks.

## 3. Task Execution Protocol

For the current top item in `PLAN.md`:

1. Read `PLAN.md` (current task) and `PROGRESS.md` (resume state).
2. If `PROGRESS.md.State == IN_PROGRESS` for this task, resume at "Next Immediate Step." Otherwise, set `State: IN_PROGRESS` and define the first step.
3. Make the smallest coherent change that advances the task.
4. Run the relevant tests/checks for that change only — don't run the full suite speculatively if it burns context without new information.
5. Update `PROGRESS.md` with what changed and what's next.
6. Commit the change with a message referencing the `PLAN.md` item.
7. Repeat 3–6 until the task is complete, then:
   - Check off the item in `PLAN.md`.
   - Set `PROGRESS.md.State: READY_FOR_REVIEW`, clear "Next Immediate Step" to point at the *next* `PLAN.md` item or "awaiting human triage."
   - Stop. Do not pull the next task automatically unless explicitly told to continue.

## 4. When Blocked

- If a task cannot proceed (missing info, ambiguous requirement, failing dependency), set `PROGRESS.md.State: BLOCKED`, write the specific blocker under "Open Questions / Blockers," and stop rather than guessing broadly or thrashing through retries that burn context.
- Do not silently skip to a different `PLAN.md` item when blocked — surface the blocker and wait.

## 5. Commit Hygiene

- One commit per meaningful step within a task, not one giant commit per task — this gives both you and the human finer-grained recovery points than `PROGRESS.md` alone.
- Commit message format: `[task: <short PLAN.md task name>] <what this commit does>`
- Never commit with failing tests unless `PROGRESS.md` explicitly notes the failure as expected/in-progress.
