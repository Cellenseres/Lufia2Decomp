# SPC-700 sound driver

`src/spc/` is a C rebuild of the sound driver the game uploads to the
SPC-700: the 4 KB program at `$90:C000` (blocks `$0300`, `$09D2`, entry
`$0300`). It replaces the program, not the S-DSP: given the same CPU port
traffic and timer ticks it produces the same ARAM contents, port replies and
S-DSP register writes, in the same order, as the original running on a
sound CPU.

This makes the SPC-700 emulation unnecessary. Only the S-DSP still has to be
synthesized, which a host can do on another processor (on the 3DS, the DSP).

## Execution model

The original is one loop that polls ports and timers and blocks in a few
handshake loops. The rebuild runs as if the sound CPU were infinitely fast:
`Lufia2SpcDriverRun` continues from where the driver last waited and returns
when the next condition it polls is unmet. `Lufia2SpcDriver.wait` names the
polling loop it stopped in. Work that sits behind a handshake is a small
queue of steps (handshake, header bytes, block data, then the action).

The host calls `Lufia2SpcDriverRun` after anything the driver can react to:
a CPU port write and a timer tick. Calling it only when the CPU next reads a
port, at a tick or at the end of a frame is also valid; it corresponds to a
sound CPU that does not see the CPU's intermediate port states, which is
what the real hardware nearly always does.

Hardware access goes through `Lufia2SpcHardware`: CPU ports, S-DSP register
read and write, the timer control and targets, the 4-bit timer counters and
`delay`, which reports the busy-wait the original spends between key-off and
key-on (`$3F08`, `$0365`). ARAM is the host's 64 KB buffer; the driver's
persistent variables stay at their original addresses (`src/spc/spc_ram.h`),
so host code such as the MSU-1 driver can keep reading `$9B` and `$08CA`.

## Data

| ARAM | Contents |
| --- | --- |
| `$001D-$00BF` | track counters, flags, pitches, voice masks, effect ids |
| `$0200` | sample directory (`DIR` = `$02`), filled by command `$14`/`$1D` |
| `$0603-$08C2` | sixteen-entry track fields; loops have two levels |
| `$08C3-$08D1` | global settings: volumes, echo, fade, tempo flag |
| `$08D2` | instrument records: ADSR1, ADSR2, tuning (16 bits) |
| `$09D2` | effect table: one offset per effect, then the effect groups |
| `$3C10-$3C3E` | percussion note map: pitch, pan, instrument |
| `$3FDD` | four echo filter presets |
| `$4362`, `$437A` | semitone pitch table, sine table |
| `$4400` | the song image |

Tracks 0-7 play music, 8-15 sound effects. Effect track `n` plays on DSP
voice `n - 8`; while it does, music track `n - 8` is flagged
`SPC_TRACK_MUTED_BY_EFFECT` and gets its instrument back afterwards.

A song starts with an `S2` header: tempo (`$03`, the timer-0 target), track
modes (`$10`, `$FF` = unused, bit 7 = percussion), echo delay (`$19`) and an
order-list offset per track (`$20`). Order lists hold transposes (`$80+n`),
pattern addresses and `$FF`. Pattern events are notes (`pitch, length, gate,
velocity`), short notes `$80-$EF` that reuse the previous timing, and the
commands `$F0-$FF`; `$FF` has 31 sub-commands.

## Behaviour kept from the original

- `DIV` overflow: command `$0A` with 0 or 1 frames gives the S-SMP's odd
  quotient (254 and 0), not a fade of 256 steps.
- `$41BF` halves the pitch `8 - octave` times with an 8-bit counter, so
  octaves above 8 give pitch 0; octave 0 shifts eight times.
- Command `$1A` has no handshake. The original reruns it on every pass of its
  main loop while the port still holds `$1A`, also after every tick (it
  overrides pattern volume changes of that track). The rebuild does the same;
  a pass without ticks and with unchanged arguments is skipped because it
  would change nothing.
- `$FD` increments `$8D + track`, which for effect tracks lands on the key
  and voice masks after the music counters.
- Effect stop requests (`$19`) index the effect table with an 8-bit offset,
  starts (`$04`) with a 9-bit one.

## Not reproduced

- Pattern commands `$F5`, `$F8`-`$FA` and `$FF 1F` jump to code that never
  pops the saved track index and returns into data. No song or effect uses
  them; the rebuild skips the command byte.
- A command `$22` or higher reruns the tempo loop with a stale counter until
  the CPU changes the port; the rebuild runs it once per change.
- Effect groups name DSP channels 0-7 (all effects in the ROM do).

## Verification

`tests/spc_driver` in the game repository runs the original program on the
reference SPC-700 core next to the rebuild. Both start from the uploaded
image and receive the same CPU port writes (one at a time) and timer ticks;
after each event the original runs until it waits in one of its polling
loops. ARAM except the scratch bytes `$00-$1C`, the I/O registers and the
stack page, the ports, the S-DSP register file, the sequence of S-DSP
writes, the timers and the wait point must then agree.

The scenarios are the start-up handshake, 48 instrument definitions, every
CPU command, all 143 effects, the 63 songs of the ROM with effect, volume
and tempo commands interleaved, 100 generated effect groups and 400
generated songs that use every pattern command, `$FF` sub-command, loop,
transpose and percussion note. Together they execute every instruction of
the original except the unused stack-corrupting paths and one branch that
the transfer protocol cannot reach.

## Routines

<!-- spc-index:begin (scripts/metadata_index.py) -->
119 routines: 117 verified, 2 disabled.

| ARAM | C function | Status | Routine |
| --- | --- | --- | --- |
| `$0300` | `Lufia2SpcDriverStart` | verified | entry: clear state, reset tracks and DSP, start handshake |
| `$03FA` | `RunCommand` | verified | command $02: play song |
| `$0406` | `RunCommand` | verified | command $03: stop music |
| `$040C` | `RunCommand` | verified | command $0A: fade step = 256 / frames |
| `$0419` | `RunCommand` | verified | command $06: fade out |
| `$0420` | `RunCommand` | verified | command $07: play song fading in |
| `$042C` | `RunCommand` | verified | command $08: report patterns played and keyed voices |
| `$0435` | `RunCommand` | verified | command $09: master volume |
| `$0440` | `RunCommand` | verified | command $0B: music group volume |
| `$0448` | `RunCommand` | verified | command $0C: effect group volume |
| `$0450` | `RunCommand` | verified | command $0D: output options (mono) |
| `$0457` | `RunCommand` | verified | command $18: report output options |
| `$045E` | `RunCommand` | verified | command $1A: music track volume, no handshake |
| `$046B` | `RunCommand` | verified | command $1B: effect track volume |
| `$047C` | `RunCommand` | verified | command $0E: reset the DSP |
| `$0482` | `RunCommand` | verified | command $0F: report status |
| `$0488` | `RunCommand` | verified | command $10: enabled music voices |
| `$048E` | `RunCommand` | verified | command $11: set sample allocation end |
| `$0495` | `RunCommand` | verified | command $13: set instrument slot |
| `$049E` | `RunCommand` | verified | command $15: toggle double tempo |
| `$04A9` | `RunCommand` | verified | command $14: sample header and data |
| `$04B2` | `RunCommand` | verified | command $1D: sample header only |
| `$04B8` | `ReceiveSampleHeader` | verified | sample length, loop and instrument record |
| `$0517` | `ReceiveSampleHeader` | verified | one header byte handshake |
| `$0527` | `RunCommand` | verified | command $1E: report keyed voices |
| `$052D` | `RunCommand` | verified | command $1F: echo filter preset |
| `$0538` | `RunCommand` | verified | command $20: echo feedback |
| `$0545` | `RunCommand` | verified | command $21: echo volume |
| `$054D` | `RunCommand` | verified | command $12: report sample allocation end |
| `$0554` | `RunCommand` | verified | command $16: set upload cursor |
| `$055E` | `ReceiveBlock` | verified | command $17: block upload at the upload cursor |
| `$0595` | `RunCommand` | verified | command $1C: two bytes at the upload cursor |
| `$05B3` | `RunCommand` | verified | command $01: block upload to an address |
| `$05BA` | `ReceiveBlock` | verified | block transfer, two bytes per handshake |
| `$05EE` | `Acknowledge` | verified | done handshake $CD/$EF |
| `$3560` | `SpcStartSong` | verified | start the song at $4400 |
| `$35FB` | `SpcMusicTick` | verified | tempo timer tick of the music tracks |
| `$3645` | `SpcStopMusic` | verified | stop music, keep effects |
| `$3691` | `SpcProcessTrackEvents` | verified | read pattern events until a note |
| `$3754` | `RunPatternCommand` | verified | dispatch of pattern commands $F0-$FF |
| `$375D` | `RunPatternCommand` | verified | $F0 vibrato amount |
| `$3776` | `RunPatternCommand` | verified | $F1 volume |
| `$3785` | `RunPatternCommand` | verified | $F2 pan |
| `$3791` | `RunPatternCommand` | verified | $F3 wait |
| `$379A` | `RunPatternCommand` | verified | $F4 tempo |
| `$37AE` | `RunPatternCommand` | verified | $F6 pitch scale |
| `$37BA` | `RunPatternCommand` | verified | $F7 instrument |
| `$37D2` | `StartLoop` | verified | $FB loop start |
| `$3813` | `EndLoop` | verified | $FC loop end |
| `$386C` | `NextPattern` | verified | $FD pattern end |
| `$38BD` | `SilenceTrack` | verified | $FE track off |
| `$38D8` | `RunExtendedCommand` | verified | $FF sub-command dispatch |
| `$38E0` | `RunExtendedCommand` | verified | $FF 00 echo delay |
| `$38EB` | `RunExtendedCommand` | verified | $FF 01 echo feedback |
| `$38FB` | `RunExtendedCommand` | verified | $FF 02 echo filter |
| `$390B` | `RunExtendedCommand` | verified | $FF 03 echo on |
| `$3925` | `RunExtendedCommand` | verified | $FF 04 echo off |
| `$393F` | `RunExtendedCommand` | verified | $FF 05 no operation |
| `$3945` | `RunExtendedCommand` | verified | $FF 06 no operation |
| `$394B` | `RunExtendedCommand` | verified | $FF 07 no operation |
| `$3951` | `RunExtendedCommand` | verified | $FF 08 attack rate |
| `$3965` | `RunExtendedCommand` | verified | $FF 09 decay rate |
| `$397F` | `RunExtendedCommand` | verified | $FF 0A sustain level |
| `$399A` | `RunExtendedCommand` | verified | $FF 0B sustain rate |
| `$39B3` | `RunExtendedCommand` | verified | $FF 0C envelope setting |
| `$39C2` | `ChangeEnvelope` | verified | select ADSR1 of the track voice |
| `$39D5` | `RunExtendedCommand` | verified | $FF 0D echo volume |
| `$39E6` | `RunExtendedCommand` | verified | $FF 0E echo volume right |
| `$39F8` | `RunExtendedCommand` | verified | $FF 0F echo volume left |
| `$3A0A` | `SpcSetEchoVolume` | verified | echo volume relative to master |
| `$3A18` | `RunExtendedCommand` | verified | $FF 10 noise clock |
| `$3A33` | `RunExtendedCommand` | verified | $FF 11 noise on |
| `$3A4D` | `RunExtendedCommand` | verified | $FF 12 noise off |
| `$3A67` | `RunExtendedCommand` | verified | $FF 13 pitch modulation on |
| `$3A81` | `RunExtendedCommand` | verified | $FF 14 pitch modulation off |
| `$3A9B` | `RunExtendedCommand` | verified | $FF 15 tremolo depth |
| `$3AA6` | `RunExtendedCommand` | verified | $FF 16 vibrato depth |
| `$3AB1` | `RunExtendedCommand` | verified | $FF 17 tremolo speed |
| `$3ABC` | `RunExtendedCommand` | verified | $FF 18 vibrato speed |
| `$3AC7` | `RunExtendedCommand` | verified | $FF 19 invert left |
| `$3AD3` | `RunExtendedCommand` | verified | $FF 1A invert right |
| `$3ADF` | `RunExtendedCommand` | verified | $FF 1B no inversion |
| `$3AED` | `RunExtendedCommand` | verified | $FF 1C invert echo left |
| `$3AF5` | `RunExtendedCommand` | verified | $FF 1D invert echo right |
| `$3AFD` | `RunExtendedCommand` | verified | $FF 1E no echo inversion |
| `$3B07` | `AdvancePattern` | verified | advance and read on when no wait |
| `$3B12` | `RunPatternCommand` | verified | wait operand of $F0-$F7 |
| `$3B1A` | `AdvancePattern` | verified | advance the event cursor |
| `$3B28` | `SpcEnterPattern` | verified | next order-list pattern, transposes |
| `$3B59` | `RememberNote` | verified | current note becomes the previous note |
| `$3B74` | `SpcQueueKeyOn` | verified | queue key-on of the track voice |
| `$3B8F` | `SpcQueueKeyOff` | verified | queue key-off of the track voice |
| `$3BC0` | `RunPatternCommand` | disabled | $F5/$F8-$FA: corrupt the stack, unused |
| `$3C08` | `RunExtendedCommand` | disabled | $FF 1F: corrupts the stack, unused |
| `$3C3E` | `SpcEffectsTick` | verified | 64 Hz effect timer tick |
| `$3C54` | `EchoRestoreTick` | verified | restore echo after an effect |
| `$3C61` | `FadeTick` | verified | music fade in and out |
| `$3C93` | `SpcUpdateModulation` | verified | vibrato and tremolo oscillators |
| `$3D04` | `SpcStartEffect` | verified | command $04: start a sound effect |
| `$3DC4` | `EffectTracksTick` | verified | effect track events |
| `$3E2C` | `WriteMusicOwnedMasks` | verified | noise and pitch modulation masks |
| `$3E33` | `SpcStopAllEffects` | verified | command $05: stop all effects |
| `$3E57` | `SpcEffectGroupPointer` | verified | command $19: stop one effect group |
| `$3E78` | `SpcStopEffectGroup` | verified | stop effects on a group's channels |
| `$3EBE` | `ReleaseEffectTrack` | verified | give a voice back to music |
| `$3F08` | `SpcFlushKeys` | verified | write key-off and key-on |
| `$3F30` | `SpcResetTrack` | verified | default track state |
| `$3F90` | `SpcWritePitchModulation` | verified | DSP PMON |
| `$3F9C` | `SpcWriteEchoEnable` | verified | DSP EON |
| `$3FA8` | `SpcWriteNoiseEnable` | verified | DSP NON |
| `$3FB4` | `SpcWriteMasterVolume` | verified | DSP MVOL |
| `$3FC2` | `SpcWriteEchoFilter` | verified | DSP FIR coefficients |
| `$3FFD` | `SpcUpdateTrackVolume` | verified | velocity x volume x group volume |
| `$4018` | `SpcWriteVolumes` | verified | voice and echo volumes, pan, tremolo |
| `$40EF` | `SpcWritePitches` | verified | voice pitches, scale and vibrato |
| `$41BF` | `SpcSetNotePitch` | verified | note to pitch with instrument tuning |
| `$420F` | `SpcSetInstrument` | verified | source and envelope of an instrument |
| `$4242` | `SpcResetDsp` | verified | DSP defaults and startup wait |
| `$42C4` | `SpcSetupEcho` | verified | echo delay and buffer clear |
<!-- spc-index:end -->
