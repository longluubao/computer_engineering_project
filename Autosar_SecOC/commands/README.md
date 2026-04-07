# Project Commands

This folder contains project-local shell commands to keep compliance workflows
inside the repository and avoid scattered external paths.

## Structure

- `commands/compliance/` — compliance checks and report generators

## Quick Start

From project root:

```bash
bash commands/compliance/run-all.sh
```

Or run a single check:

```bash
bash commands/compliance/misra-report.sh
```

