# Memory map

The supported metadata ROM id is `lufia2-usa`. Address notation is
`BB:AAAA`, where `BB` is the 65816 bank and `AAAA` is the 16-bit address.

## Catalog

[`metadata/memory_map.toml`](../metadata/memory_map.toml) is the canonical
catalog of named work-RAM locations. Each `[[location]]` records:

| Field | Meaning |
| --- | --- |
| `address` | First byte, `7E:xxxx` or `7F:xxxx` |
| `name` | `lower_snake_case`, or `unk_<bank><address>` such as `unk_7FE216` |
| `width` | Bytes per element |
| `count` | Elements of a per-slot array (default 1) |
| `stride` | Bytes between elements (default `width`) |
| `owner` | Owning subsystem (`actor`, `object`, `field`, `text`, `system`, ...) |
| `notes` | The evidence for the name and any addressing remarks |
| `aliases` | Optional names another header uses for the same address |

The catalog is deliberately partial. It starts with the actor and object
slot arrays, the field flags and the direct-page slot and NMI state that the
verified routines use most; it grows as reconstruction needs more names. It is
not an attempt to map all of WRAM.

## Addressing convention

Addresses are canonical WRAM addresses. The low 8 KiB `$7E:0000-$7E:1FFF` is
mirrored into banks `$00-$3F` and `$80-$BF`, and the game usually reaches it
through that mirror: as a direct-page offset while D = 0, as an absolute
address while DB is a system bank, or as a long `$00:xxxx` address. The
catalog records those bytes at `$7E`, where they physically live; the code
that reads them keeps its original addressing mode, so DB and D stay
observable. Other locations are recorded at their long `$7E`/`$7F` address.

Per-slot arrays are parallel arrays, one byte (or word, or 24-bit record) per
slot. The actor slots 0-39 index byte arrays by the slot (`$A7`), word arrays
by `2 * slot` (`$A9`) and 24-bit records by `3 * slot` (`$AB`); `$83:AB4F`
computes those offsets. Object slots 0-31 follow the same pattern.

## Naming rule

A name must be supported by verified source, the research notes or a verifier
contract, and the entry's `notes` say which. When the meaning is not
established, the entry uses the neutral `unk_<bank><address>` name, even if a
guess is tempting. A later task renames it once evidence exists.

## Generated constants

`python3 scripts/metadata_index.py` validates the catalog (fields, names,
overlaps, the direct-page and mirror boundaries) and regenerates the constants
block in `src/system/wram.h`:

| Address | Constant | Value |
| --- | --- | --- |
| `7E:0000-00FF` | `DP_<NAME>` | 8-bit direct-page offset, for D = 0 |
| `7E:0100-1FFF` | `WRAM_<NAME>` | 16-bit offset into the bank-$00 mirror |
| other | `WRAM_<NAME>` | 24-bit long address |

Arrays also get `<CONSTANT>_COUNT`. The script rejects a `WRAM_`/`DP_`
constant, or any constant holding a long `$7E`/`$7F` address, in another
header that repeats a catalogued address under a name the entry does not list
in `aliases`; there is one naming truth. Subsystem headers such as
`src/cave/wram.h` and `src/field/event_script_internal.h` still own constants
for locations that are not catalogued yet.

## Magic numbers

New semantic code uses a named constant, or a slot view built on one
(`src/actor/actor_slot_view.h`), when the catalog has the location. A
genuinely unknown address may stay literal until it is catalogued with a
neutral name. ROM addresses, hardware registers and instruction PCs are
addresses by nature and stay literal; they are not game-state fields and are
never replaced mechanically.

## $83:BBF3

`$83:BBF3` reads six bank-$00-visible WRAM locations through its own
16-bit accessor: `$09A8`, `$05B5` (`field_flags`), `$05B7`
(`field_requests`), `$0622` (`actor_state`, slot 0), `$099B` (`text_state`)
and `$09A7` (`window_mode`). The routine keeps its verified bridge ABI and
the literal addresses.
