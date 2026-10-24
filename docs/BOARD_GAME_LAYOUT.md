# Five-by-five board game workspace

The original `$84:C826` controller uses a five-by-five board. The reconstructed
setup and line-counting routines live in `src/menu/board_game.c`. The neutral
subsystem name describes the proven data; it does not identify game branding.

These bank-relative arrays overlay the card and poker workspace. Their local
names avoid adding overlapping locations to the canonical memory map.

| Address | Use |
| --- | --- |
| `$7000` | Particle position words |
| `$703C` | Particle horizontal steps |
| `$7078` | Particle vertical steps |
| `$70B6` | Position used by round reset |
| `$70B8` | Twenty-five board cell words |
| `$711C` | Twenty-five cell-group words |
| `$714E` | Fifty draw-order words |

Initialization performs fifty particle iterations despite the thirty-word
spacing of their array bases. These writes overlap in the original and remain
ordered. It also initializes thirty OAM attribute words and five OAM-high words.
The group assignments come from the ROM-backed table at `$84:DA39`.

Cell shuffling performs twenty-five random swaps. Draw-order preparation calls
that shuffle, then performs eighty-one swaps among its first twenty-five words.
It clears thirteen line-total words, including the adjacent completed-line count.
Round reset instead clears twelve totals and clears that count separately.

Line counting reads the cells' high bytes, using byte additions for each sum.
It processes five rows, five columns and two diagonals in that order. The
original N-bit comparisons classify sums at least 100 as complete and sums
88 through 99 as near-complete. No host-side sum replaces the byte arithmetic.

All seven reconstructed entries preserve their incoming accumulator width
through PHP/PLP; both M0X0 and M1X0 are verified. Their canonical metadata mode
is M1X0, as used by the controller. DB remains live, DP must be zero and decimal
arithmetic must be disabled. Runtime binding is a separate consumer decision.

## Controller and draw animation

`src/menu/board_game_session.c` reconstructs the complete `$84:C826` controller,
the `$CAF1` replay selector, `$CB64` continuation, `$CCD3` cell draw and `$CDA8`
animation update. The controller normalizes either incoming accumulator width
to M1 and returns in M1; the other four routines preserve the caller's width.

The controller retains palette transfers, both saved data-bank frames, initial
shuffling, byte-based slowdown counters, grouped and ungrouped draws, result
messages, optional continuation, replay and the empty-balance exit. The N flag
from a word decrement survives the switch to byte mode before the group branch.

The animation keeps original sine/cosine calls, packed byte coordinates, angle,
radius, fall-step, phase and timer updates. Confirming a draw shortens the timer
only in the original angle window. The `$84:CEB3` multiplication child remains
original, including its four hardware-delay NOPs; no host multiplication or
guessed timing replaces it.

The replay selector uses OAM slot 127 at `$02FC`, with its tile word at `$02FE`.
The separately named `card_game_replay_cursor` at `$7133` contains selection
layout parameters for a different card-game routine and is not this OAM slot.

## Selection, movement and board tiles

`src/menu/board_game_selection.c` implements the remaining-count digits, group
border drawing and clearing, group movement, cell tile transfers, cell rotation
and complete board rendering. Border clearing retains the original frame child.
The ROM tables at D8ED/D8F9/D909, D911..D9E9, DA31 and DA69 remain bus reads.

Movement performs three phases of six tile transfers. Each transfer saves and
erases four tile words before restoring them elsewhere. The words at 71D2..71D8
are named `board_game_saved_cell_tiles`. The adjacent six words at 71DA..71E4
are `board_game_saved_group_cells`, used to snapshot a group before rotation.
Rotation updates the cell-ID-to-location overlay at 70EA after each cell write.
This overlay shares existing card/poker storage and has no duplicate map record.

Board rendering reads each cell twice, combines its high-byte attributes using
the original addition, and writes four tiles in order. Rows advance 192 bytes.
The counter retains the original signed subtraction even for malformed counts.
The three tile leaves require M0X0; the other six entries preserve either M width.
All nine are verified against the original ROM and remain runtime-unbound.

## Stakes, payout and particles

`board_game_results.c` reconstructs the stake chooser, its six-sprite digit
helper, payout calculation, payout-step credit, cell-location rebuilding and
particle update. Stake selection keeps balance reservation and refunds, all
controls, twelve frames of twelve sprites and the final tile conversion.

Line payout scans twelve totals at thresholds101/108, applies the original
fivefold bonus and repeats the original shifts/additions. The payout step at
69E4 starts at10000. Its A3FC child uses the original hardware division and eight
NOP waits. A421 credits one step and keeps the original low-word cap behavior.
69E2 is the bonus/multiplier word. These fields have canonical WRAM names.

Particles use the ring index at70B4 and OAM origin at70B6. Both overlay card-hand
storage, so their names remain local. Twenty-five positions move through packed
byte arithmetic. The byte comparison before REP retains its N flag. Horizontal
velocity reads use fixed bank7E while other workspace accesses use live DB.
Two marker words at7204/7206 move their OAM positions alongside the particles.

The normal payout caller uses DB7E. With DB7F, relative denomination stores and
fixed-bank reads can disagree indefinitely. A malformed particle ring start can
also fail to terminate. Reconstruction preserves both behaviors and provides
exact original-PC handoffs at bounded child/iteration prefixes. These are
continuation boundaries, not fixes to the original game.

All six contracts are verified and remain runtime-unbound. The five PHP/PLP
roots preserve either accumulator width; the digit helper requires M0X0.
