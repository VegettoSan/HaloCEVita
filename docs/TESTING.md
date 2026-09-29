# Testing protocol

## Build evidence

For every testable build record:

- git commit SHA;
- debug/release configuration;
- exact build command;
- resulting VPK SHA-256 when sharing a build;
- console model/firmware if relevant;
- vitaGL/libshacccg prerequisites;
- game-data version if relevant;
- startup log.

## Startup log milestones

Add simple ordered markers as early as possible, for example:

```text
[VITA 001] process start
[VITA 002] filesystem ready
[VITA 003] log opened
[VITA 004] controls initialized
[VITA 005] graphics init begin
[VITA 006] vitaGL ready
[VITA 007] Halo memory init begin
[VITA 008] data root found
[VITA 009] maps verified
[VITA 010] Halo main entered
```

Keep markers stable once used in test reports so crash locations remain comparable.

## Real Vita vs Vita3K

Do not assume Vita3K perfectly represents hardware for homebrew, plugins, shader compiler behavior or memory limits. Label each result explicitly:

- `REAL VITA`
- `VITA3K`
- `BOTH`

A feature should not be marked `STABLE` for hardware based only on Vita3K.

## Minimum report for a failure

- last visible/logged milestone;
- error code/crash address if available;
- log excerpt;
- whether failure is deterministic;
- whether the previous known-good build works with the same data/setup.
