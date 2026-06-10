# PANZI ARC-Style Release Governance

PANZI borrows the governance pattern from the FreeEQ8 ARC/Arc-RAR ecosystem.
The plugin must stay local, deterministic, and real-time safe. Governance
belongs in release evidence, manifests, and validation scripts.

## Rules

1. **Audio thread authority is narrow:** read baked coefficients, process
   samples, write atomic display taps. No packaging, networking, filesystem IO,
   or manifest work may touch `processBlock`.

2. **No heap allocation on the audio thread.** Verified by `scripts/validate_repo.py`
   and confirmed by ASan build before each public release.

3. **Background baker authority:** `TopologyBaker::bake()` runs on `BakeThread`
   only. Results are handed off via `CoeffHandoff` SPSC atomic swap. No locks.

4. **Every release must include:** a manifest, `validate_repo.py` output,
   pluginval evidence, CPU profiling result, and known limitations.

5. **Every DSP backend change must preserve parameter IDs** unless a major-version
   migration doc is added to `docs/`.

6. **Claims must match evidence:** "8-channel polyhedral spatializer" requires a
   REAPER 7.1 surround project smoke test. "< 2% CPU" requires profiling output.

## Release Lanes

- **Floor:** last known stable source package (FreeAutoPanner v0.1.0)
- **Candidate:** current improved branch (PANZI v0.2.0-dev)
- **Promoted:** candidate after `validate_repo.py` pass, local build, pluginval,
  and DAW smoke tests

## Why This Matters

A spatial audio plugin can sound impressive but still fail production if it
allocates, blocks, denormal-spikes, or lies about readiness. The release model
prevents hype from outrunning the actual engineering state — the same lesson
learned from the FreeEQ8 r/DSP incident.
