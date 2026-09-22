# External reverse-engineering research

External reverse-engineering projects are useful evidence, but they are not
authoritative replacements for the supported US ROM. Lufia2Decomp records the
source and confidence of imported knowledge so external labels do not silently
become assumed game semantics.

## Evidence policy

- Portable decompilation semantics are reconstructed from the supported ROM.
- External research may provide names, formats, likely boundaries and test
  hypotheses.
- Version-specific absolute addresses are not promoted without checking them
  against the supported US ROM or generated code.
- A semantic implementation still requires the normal differential-verification
  process before it can become `verified`.
- Distinct Lufia II bytecode languages must not share opcode names merely
  because their numeric opcode values happen to match.

## LEdit

Source: `pdinklag/LEdit`, main commit
`08be20aaa174916a7e2c25625d835622ab92f2b5`.

LEdit is an archived, unfinished Lufia II ROM editor whose author published the
project specifically because it contains reverse-engineered ROM knowledge
beyond the Reliquary. The repository is released under CC0 1.0.

### Version caveat

Most of LEdit's original research was performed against the German ROM.
Consequently, old documents containing hardcoded ROM/PC addresses are treated
as version-specific research notes rather than US-ROM facts.

The later Java code explicitly supports an English ROM subclass, so some
English-specific values can be checked independently.

### Resource file system: confirmed useful

LEdit's English implementation reports:

- resource pointer-table PC offset: `0x138000`;
- resource count: `0x2A8` entries;
- each table entry: a 3-byte relative offset from the table base;
- each compressed stream begins with a 16-bit decompressed size followed by
  the compression control state.

The repository README says there are 679 compressed files, but the
implementation uses `0x2A8 = 680`, and its default file metadata contains
indices 0 through 679. Lufia2Decomp therefore uses **680** as the working
count.

Owner-side validation on the supported headerless US ROM independently checked
the format:

- all 680 relative table entries resolve inside the ROM;
- the LEdit-described decompression algorithm completed for all 680 streams;
- every stream produced exactly its declared decompressed byte count.

This is strong evidence for the resource-container and compression semantics.
It is not yet a differential proof of the original CPU routine's full
entry/exit state.

LEdit also classifies resource ranges such as maps beginning around file
`0x040`, map block sets around `0x131`, map tile sets around `0x14C` and
monster graphics around `0x1B8`. These category boundaries are useful
research hints but remain separately verifiable facts before they are relied on
by runtime code.

### $80:8E9D / $80:8EAF decompression routine

LEdit's ROM map labels `$80:8EAF` as the file-decompression routine and notes
that the file index is present in A at `$80:8EB5`.

The generated Lufia2SNESRecomp program groups this region inside the function
beginning at `$80:8E9D`. Its M0X0 and M1X0 variants are AOT-eligible and each
contain 232 analyzed instructions. Runtime captures also show heavy loop
activity inside the function.

For project naming, the function is currently tracked conservatively as:

`$80:8E9D Lufia2DecompressResourceFile` — `identified`.

Planned progression:

1. reconstruct the complete original routine as portable semantics;
2. build an original-ROM differential verifier for its call boundary and
   produced bytes;
3. only then consider a native replacement;
4. keep any host-side fast decompressor or decompressed-resource cache in the
   separate patch layer.

The distinction in step 4 matters: this function is already AOT-eligible, so a
plain C decomp does not remove an LLE boundary the way C7F8/D508 eventually
will. A host decoder/cache can still be a meaningful optimization because it
can replace the original per-byte/backreference work rather than merely
expressing the same loop more readably.

### Map setup commands are not the actor VM

LEdit's old map research documents a setup-command stream with examples such
as:

- `0x4B`: music;
- `0x68` / `0x7B`: character/monster setup;
- `0x69`: map settings;
- `0x74`: battle background.

This command language is not assumed to be the actor script VM currently being
reconstructed at `$83:C83C` and `$83:D59A`. Numeric opcode collisions
between those systems have no semantic meaning without ROM evidence.

LEdit also contains a partially decoded battle-script language and a
disassembler with instruction-length knowledge and commands such as GOTO,
failure branches and chance branches. That is a promising reference for future
battle/item/monster script decompilation, but it is likewise a distinct VM
from the current bank-$83 actor work.

### Other useful future references

LEdit contains substantial research on:

- map headers and map resource structure;
- map sprites and palettes;
- map collision flags;
- doors, warp transitions and presets;
- monster, item, spell and IP-attack data;
- battle scripts;
- resource extraction/decompression.

These sources can accelerate naming and hypothesis generation, but any
German-ROM absolute address must be translated or rediscovered for the
supported US ROM before becoming project metadata.
