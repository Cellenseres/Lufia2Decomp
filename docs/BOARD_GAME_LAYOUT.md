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
