# Card-column continuation contracts

These are internal boundaries of the existing verified column algorithms,
not additional original subroutines. The existing entry count is unchanged.

| API | Original entry | Count limit | Next original instruction |
| --- | --- | --- | --- |
| `Lufia2CardTableContinueSpriteStrip` | `84:EA30` | 1..24 sprites | `EA30` or `EA40` |
| `Lufia2CardTableContinueColumnDrawing` | `84:EAB4` | 1..6 symbol rows | `EAB4` or `EB35` |

Both require native M0X0, bank84, DP0 and binary arithmetic. They use the
supplied bus callbacks and preserve original read/write order, flags, registers
and stack. Each completes at most the requested number of whole iterations.
Original zero tests still end the loop earlier. The result is an exact
`LUFIA2_EXECUTION_BOUNDARY`; the caller resumes at its PC. Unsupported inputs
return the original entry boundary without bus accesses or architectural changes.

The original whole routines share these loop bodies and retain their original
termination conditions, including counter underflow. They have no host iteration
limit. The standalone library does not inspect a host deadline or omit guest work.

The shared checkpoint compares 51,712 prefixes against the original interpreter
with CPU state, full WRAM and ordered bus accesses, including 1,024 corrupt-counter prefixes, plus 20 input guards.
Affected original entries pass 9,216 cases, eight child unwinds, 40 changed child
returns and 2,304 nested cases. Consumer evidence is separate: 5,760 timed
ROM/ABI cases, 85 admission guards, 9,358 live-registry/step-accounting checks and
224 pairs of original-caller hardware/PCM comparisons.

Host timing and partial-run admission belong to the consumer. Its two hooks
preserve live ownership, observer exclusions, interrupt/device boundaries,
refresh and APU synchronization. Native admission alone is not a speed claim.
