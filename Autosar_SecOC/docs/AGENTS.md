# Cursor agents (from Claude Code)

This project mirrors **Claude Code subagents** as **Cursor project skills**.

## Where things live

| Claude Code | Cursor |
|-------------|--------|
| `.claude/agents/*.md` | `.cursor/skills/<agent-name>/SKILL.md` |
| Task/subagent invocation | Ask the AI to use a skill by **name** or describe the **role** (the skill `description` drives discovery) |

## Regenerating skills

After editing files under `.claude/agents/`, run:

```bash
python .cursor/scripts/convert_claude_agents_to_cursor_skills.py
```

## Notes

- **Hooks** in `automotive-settings-snippet.json` (MISRA/safety shell hooks) target Claude Code `settings.json`; Cursor does not use the same hook API. Use editor tasks, CI, or manual checks instead.
- **Slash commands** under `.claude/commands/` remain for Claude Code; in Cursor, use **Chat prompts** or **Rules** for similar workflows.
