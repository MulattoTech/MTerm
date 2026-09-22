# CI recipe and activation barrier

`native.workflow.yml` is the exact Windows/Linux Release/Debug workflow source, stored here as
**non-executable documentation**. GitHub Actions only installs it from `.github/workflows/`.
The current saved Git OAuth credential was denied installation because it lacks `workflow`
permission. Source pushes work. No credential/settings change or authorization bypass was attempted.

The local development branch retains `.github/workflows/native.yml` and the rejected commit;
the source-only publishing branch excludes that executable workflow path. Nothing was deleted.
An authorized maintainer must install the documented recipe after appropriate workflow-write
access is granted. Then inspect actual hosted results; local tests are not hosted-CI evidence.

The recipe pins third-party actions to commits, limits permissions to contents-read,
disables persisted checkout credentials, uses isolated runners, records platform skips,
and never invokes real models. Linux PTY tests intentionally report unsupported/skipped.
Qt 6.8.3 is an initial reproducibility pin and needs dependency-security review before release.
