# Bootstrap handoff

Date: 2026-09-21

## Baseline

- Repository: `https://github.com/Cellenseres/Lufia2Decomp.git`
- Initial remote state: empty (no refs).
- Local baseline: unborn `main` branch; no prior files or dirty changes.
- No license, ROM, extracted assets, submodules, commits, or pushes were added.

## Bootstrap contents

- Portable target: `lufia2_decomp`, alias `Lufia2::Decomp`.
- Metadata schema version: `format = 1`, ROM id `lufia2-usa`.
- Function statuses: `identified`, `draft`, `verified`, `disabled`.
- `$83:BBF3` is `Lufia2PlayerSlotSpecialUpdate`, status `draft`.
- The semantic implementation covers gates at `$09A8`, `$05B5`, `$05B7`,
  `$0622`, and `$099B`, plus the `$09A7` child-branch decision.

## Verification still required

Do not promote `$83:BBF3` until the consumer-side differential contract has
passed for WRAM, A/X/Y, flags including M/X, S/D/DB/PB, both child branches,
return/stack behavior, and relevant bus effects. No such pass is claimed here.

## Bootstrap build verification

The standalone project was configured with CMake 4.4.2 and Ninja 1.13.2 and
built with Clang 19.1.5 on Windows. A temporary local semantic harness covered
the known gates and both child branches before being excluded from the commit.
This confirms the portable library setup; it does not promote `$83:BBF3` from
`draft` or replace the required differential verification.

## Owner commands

Standalone library build after the owner chooses a build directory:

```powershell
cmake -S G:/Development/SNESRecomp/Lufia2Decomp -B <decomp-build-dir>
cmake --build <decomp-build-dir> --config Release
```

After reviewing these files, create the first commit and push it. The consumer
handoff contains the exact command for adding the pinned submodule afterward.
