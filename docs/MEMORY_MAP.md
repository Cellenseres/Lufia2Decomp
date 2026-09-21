# Memory map

The supported metadata ROM id is `lufia2-usa`. Address notation is
`BB:AAAA`, where `BB` is the 65816 bank and `AAAA` is the 16-bit address.

The initial `$83:BBF3` draft reads six bank-$00-visible WRAM locations:
`$09A8`, `$05B5`, `$05B7`, `$0622`, `$099B`, and `$09A7`. Their surrounding
structures and durable semantic names are not yet proven. The portable routine
therefore reads them through an abstract byte accessor and keeps the literal
addresses visible in the implementation.
