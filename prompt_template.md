Use the `autosar-chief-engineer` subagent for this task.

## USER GOAL
[Describe exactly what you want implemented]

## CONTEXT
- Project/module:Autosar_SecOC/source/, Autosar_SecOC/include/

- Related files (if known): [path1, path2, ...]

- Constraints: [timing, safety, no API break, etc.]

## REQUIRED SKILLS TO CONSIDER
Chief engineer should use these skills as guidance before implementation:
- [autosar-coding-standards]
- [autosar-architecture-expert]
- [autosar-com-stack-expert]
- [autosar-diagnostics-stack-expert]
- [autosar-os-expert]
- [autosar-ecum-expert]
- [autosar-bswm-expert]
- [autosar-nvm-expert]
(Keep only relevant ones)

## EXECUTION POLICY
- Follow PDCA:
  - **Plant**: plan + research internet + research codebase
  - **Do**: implement based on internal + external evidence
  - **Check**: verify directly against my request
  - **Act**: fix errors and re-verify
- Cost rule: use at most **1 specialist subagent** unless I explicitly approve more.
- Follow project coding standards from `Autosar_SecOC/source/Code_Style.md`.

## DELIVERABLE FORMAT
Return exactly:
1. **Plant**
2. **Do**
3. **Check**
4. **Act**
5. **Final Result**
   - changed files
   - test/lint/build results
   - remaining risks (if any)

## SUCCESS CRITERIA
[Bullet list of what must be true when done]

## OPTIONAL APPROVAL GATE
Ask me before:
- adding dependencies
- large refactors
- using more than 1 specialist