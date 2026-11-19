# GitHub Actions scheduling policy for Abcd-huy

## The latest revision wins for build and validation workflows

Each build or validation workflow has a top-level GitHub Actions `concurrency`
group scoped to its **workflow name/file** and its branch or pull request.
Its `cancel-in-progress: true` ensures GitHub cancels a superseded running
build, and replaces an older pending build when a newer run is queued.
The `General build matrix` follows this policy on `master` as well as PRs:
the previous exception for an in-progress `master` build was removed.

**Do not use a single global concurrency group for the whole repository.**
A lint job must not cancel the APK job, and a translation check must not
cancel a C++ compiler. GitHub doesn't promise chronological ordering of
multiple pending runs. The per-workflow groups restrict the runnable set;
publishers must also verify the commit is still current.

## Parallelism

An Android APK uses up to **four native C/C++ compiler jobs** (`-Pj=4`)
and **four Gradle workers** (`--max-workers=4`). There is **one APK workflow
run per branch** across debug, test-signed Release and production-signed
Release. The test-signed ARM64 Release is the **only APK auto-triggered by a
push to master**; debugging and official production builds are manual.
The Release trial rechecks the `master` SHA before publishing the artifact.
The ARM64 bundle (AAB) is a separate manual production operation; it also
uses four build workers but is not cancelled mid-publication.

The general master test matrix also limits simultaneous jobs to four,
and the experimental release's package matrix and the reusable tileset
matrix each use `max-parallel: 4`. This prevents single revision workflows
from launching dozens of parallel runner builds. It does **not** enforce a
global four-runner cap across independent workflows; that would require
repo-level runner/concurrency quotas and would incorrectly cancel unrelated
checks.

Four is a *maximum for compatible builds*, not a promise that all phases
saturate four CPUs. Memory-heavy host compiler variants may deliberately
use fewer jobs to avoid out-of-memory failures. Workflow-wide concurrency
controls the number of **runs**, not CPU threads inside each build.

## Events which must not be discarded

Automations that **apply a change to GitHub**, such as per-comment commands,
one-time contributor labels, post-build PR feedback,
release publication and reusable release assets, are scoped to their own
comment/PR/upstream-run or source ref and use `cancel-in-progress: false`
as appropriate. Do not lose a distinct user's command just because a later
comment appears on the same issue. Post-processing an upstream workflow is
grouped by that **upstream workflow run ID**, not by the default branch.
These workflows are protected from indiscriminate "cancel all runs" cleanup.

## Manual legacy queue cleanup

`.github/workflows/actions-queue-reset.yml` is a **manual-only** cleanup
for backlogged runs created by older configurations. It requests cancellation
of older **push/PR CI** runs while retaining the newest active run per
workflow and branch/PR. It never cancels manual releases, deployment,
scheduled maintenance, workflow_run post-processing, or comment commands.
It does not dispatch an extra Android build. Ordinary pushes do not trigger
this cleanup because the concurrency groups already handle superseded CI.

## How to validate

`python3 -m unittest discover -s tools/accessibility -p 'test_repo_actions_latest_only.py' -v`

Review the GitHub Actions page for an actual old run marked `cancelled`
and a newer run completed before claiming the runtime policy is verified.
