# Contributing

Keep changes narrowly tied to original-game behavior. Use conservative names
until evidence establishes a structure or purpose, and record original facts
in `metadata/` rather than consumer bridge names.

Every semantic implementation starts as `draft`. Promotion to `verified`
requires the documented differential contract against the supported original
program, including registers, flags, stack, memory, and bus effects. Keep
temporary test and debug harnesses outside committed source.

Do not add ROMs, extracted proprietary assets, generated game C, platform
dependencies, or a license chosen without the owner's decision.
