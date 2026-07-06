---
name: setup-review-skill
description: Set up a PROJECT-SPECIFIC post-implementation review skill in the current project. Use when the user wants a repeatable multi-pass code-review skill (security, performance, tests, architecture, ...) tailored to this project, or asks to port/recreate such a review skill here. This is a setup/meta skill: it does not run a review, it guides you to author a project-specific review skill that IS tailored to what matters for this codebase, and that re-checks its own relevance before every run.
---

# Set up a project-specific post-implementation review skill

Your job here is NOT to review code. It is to **author a new, project-specific review skill** at
`.claude/skills/post-implementation-review/SKILL.md` in the current project, tailored to what this
codebase actually is. A generic copy is worse than nothing , it sends a reviewer hunting for an
accessibility problem in a project with no UI, or skips the one risk that matters here.

The pattern being set up: a **post-implementation review** is a sequence of focused passes run after
a longer implementation push, to catch what accumulated (security, performance, test gaps,
architecture drift, inconsistency, and project-specific concerns). Each pass reads the whole codebase
through one lens, presents findings, and STOPS for approval before anything is fixed.

## The non-negotiables (carry all of these into the skill you write)

1. **It must be adapted to THIS project.** Do not copy a fixed list of passes. Derive the passes from
   what the codebase is and what could actually go wrong in it (see Step 1). Drop passes that do not
   apply; add passes for this project's real risks.
2. **The skill must re-check its own relevance before every run.** The skill you write MUST contain,
   as the first thing that happens (before pass 1), a standing instruction: *before each review
   round, check whether this review skill itself needs updating because the project has changed since
   it was last run* (new components/directories the passes do not mention, files or artifacts moved or
   renamed so a pass points at something that no longer exists, changed conventions, a new concern, a
   removed feature). If so, propose the change, STOP for approval, and update + commit the skill
   FIRST , a stale review skill reviews the wrong things. This is a hard requirement from the user.
3. **Stop between every pass.** Present findings + a concrete fix plan, then STOP and wait for
   explicit approval. Never start fixing before the user says what to fix. Fix only what is approved.
4. **Commit gate between passes, one pass at a time.** Once the approved fixes for a pass are done and
   verified (build + tests green), follow the user's git routine; only then move to the next pass.
5. **It orchestrates within the user's global preferences, it does not replace them** (writing style,
   code style, error handling, git routine, plan-mode). Write the skill in the project's working
   language.

## Step 1 , Learn the project, then decide the passes

Before writing, inspect the codebase: languages, layers, entry points, frameworks, data stores,
external integrations, auth model, what is user-facing, what regulations or contracts apply, what the
dominant conventions are (error handling, testing). Read the project's CLAUDE.md/README if present.

Then choose passes. Order them by severity, security-type concerns first, the docs/memory-update pass
last. Use these as a starting grid, not a checklist to copy:

**Almost always relevant**
- **Security** , authz/authn, input validation at boundaries, injection, secrets, unsafe defaults,
  rate limiting, PII in logs/errors, crypto choices. Name THIS project's auth/tenant model in the
  prompt.
- **Performance** , the hot paths and data-access patterns that exist HERE (N+1, missing indexes,
  unbounded results, sync I/O, missing cancellation, caching where warranted).
- **Test coverage** , uncovered branches, authz/negative cases, boundaries, concurrency, external
  integrations. State this project's preferred test levels and what "don't mock our own code" means.
- **Architecture and code quality** , responsibility/layering, dependency direction, coupling, DI,
  complexity, dead code, duplication worth extracting.
- **Consistent patterns** , find code that deviates from the dominant pattern HERE (error handling,
  endpoint/handler shape, naming, validation, async, logging, config access).
- **Update docs/memory (last)** , bring the project's CLAUDE.md and any memory/notes current with the
  decisions and learnings from the session and the passes above.

**Conditional , include only if they apply**
- **Accessibility (a11y)** , only with a user-facing frontend. Drop it for a pure backend/library/CLI.
- **Legal / compliance** , only for a regulated domain. Make it the project's actual regime (GDPR,
  CRA, HIPAA, PCI-DSS, SOC2, financial), not a generic one. Drop it if nothing applies.
- **Manual-test sync** , only if the project has a manual/acceptance test suite to keep in step with
  the features (see the `manual-test-harness` skill if one is being set up).

**Project-specific , add what this codebase needs**
Examples to consider, pick what fits: public API contract / backwards compatibility; database
migration safety (destructive/irreversible migrations, zero-downtime); i18n/localization coverage;
infrastructure-as-code / deployment config; concurrency & data races; observability (are new paths
logged/metered); dependency & supply-chain (new packages, licenses); ML model/eval drift; cost/quota.
If a pass would have no real surface in this project, leave it out.

## Step 2 , Write the skill

Create `.claude/skills/post-implementation-review/SKILL.md` with frontmatter (`name`,
`description` listing the chosen passes) and this structure:

1. **Intro** , one paragraph: what it is and the passes, in order.
2. **Ground rules** , the non-negotiables 3-5 above, plus: thoroughness over speed, and *read the
   whole codebase each pass* (do not rely on memory from a prior pass; a perf bug can hide in a file
   you skimmed during the security pass).
3. **What "the whole codebase" means here** , name the directories that are product code, the ones
   that are tooling/lower-bar, and the build/dependency artifacts that are never reviewed
   (`node_modules/`, `dist/`, build output, etc.). This is project-specific , spell it out.
4. **The self-check section (non-negotiable #2)** , a short section placed BEFORE pass 1, instructing
   the reviewer to verify the skill is still current vs the repo and to update + commit it first if
   not. Phrase it for this project.
5. **The passes, in order** , each a short section: the lens, a project-specific "think about" list
   (reference the real stack, layers, conventions, regulations), and a closing "present findings
   grouped by severity / with file:line, propose a plan, and STOP."
6. **Closing** , after all passes, a short plain-language summary of what was reviewed, fixed +
   committed per pass, and deliberately left.

Keep each pass prompt concrete to this codebase. "Check authz" is weak; "verify every endpoint under
the tenant-scoped policy 404s on another tenant's resource, including mutations" is the kind of
prompt that finds real bugs.

## Skeleton to fill

```markdown
---
name: post-implementation-review
description: <one line: thorough post-implementation review of THIS project; list the chosen passes>
---

# Post-implementation review

<intro: the passes, in order, and why this order>

## Ground rules for the whole run
- Thoroughness over speed. Read actual code, follow code paths, verify suspicions.
- Read the WHOLE codebase each pass, through that pass's lens.
- STOP after each pass; present findings + plan; fix only what is approved.
- Commit gate between passes; one pass at a time; follow the user's global preferences.

## What "the whole codebase" covers here
<product dirs | tooling dirs (lower bar) | never-reviewed artifacts> , project-specific.

## Before pass 1: self-check the skill against the repo
Always check first whether THIS skill needs updating because the project changed since it last ran
(new components/dirs the passes miss, moved/renamed targets, changed conventions, new/removed
concerns). If so: propose the change, STOP for approval, update + commit the skill, THEN start. A
stale review skill reviews the wrong things.

## The passes, in order
### 1. Security
<project-specific lens> ... present by severity, file:line, plan, STOP.
### 2. <...>
...
### N. Update docs/memory (last)
<bring CLAUDE.md + memory current> ... present a diff, STOP.

## Closing
<short summary of reviewed / fixed+committed / deliberately left>
```

## Reference: one project's tailoring (example, not a template to copy)

cra-reporting (multi-tenant regulated web app: .NET backend + Vue SPA + a manual-test tool) chose
nine passes: 1 Security, 2 Performance, 3 Test coverage, 4 Architecture & code quality, 5 Consistent
patterns, 6 Accessibility, 7 Legal (CRA/GDPR), 8 Manual-test sync, 9 Update CLAUDE.md/memory. It
includes a11y because it has a frontend, a legal pass because it is GDPR/CRA-regulated, and a
manual-test pass because it ships a manual-test harness; its "whole codebase" note distinguishes the
product app from the lower-bar dev tool and excludes build artifacts. A backend-only library would
drop passes 6-8 and might add an API-compatibility pass instead. Derive yours the same way , from
what the project is.

## After writing

Show the user the proposed skill, confirm the chosen passes fit the project, then follow the git
routine to commit it. Note that it is invoked later (e.g. `/post-implementation-review`) after a big
implementation push.
