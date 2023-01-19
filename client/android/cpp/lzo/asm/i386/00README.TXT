
Directory overview:
===================

As writing portable assembler sources supporting different operating
systems, compilers and assemblers has proven to be extremely painful,
the assembler sources have been converted into a more portable 'db'
format. Use these whenever possible.

  src_gas       sources converted for portable gcc/gas syntax
  src_masm      sources converted for portable masm/tasm/wasm syntax
  src_nasm      sources converted for portable nasm syntax

  src           assembler sources (you need the OpenSource nasm assembler)

  obj           pre-assembled object files

Also look 'src_XXX/all/asm_all.asm' which contains all assembler
functions conveniently arranged into a single file.


Notes:
======

- The assembler sources are designed for a flat 32-bit memory model
  running in protected mode - they should work with all i386
  32-bit compilers around.

- All functions expect a 'cdecl' (C stack based) calling convention.
  The function return value will be placed into 'eax'.
  All other registers are preserved.

- Prototypes for the assembler functions can be found in <lzo/lzo_asm.h>.

- For reasons of speed all fast assembler decompressors (having '_fast'
  in their name) can access (write to) up to 3 bytes past the end of
  the decompressed (output) block. Data past the end of the compressed
  (input) block is never accessed (read from).
  [ technical note: because data is transferred in 32-bit units ]

- Finally you should test if the assembler versions are actually faster
  than the C version on your machine - some compilers can do a very good
  optimization job, and they also can optimize the code for a specific
  processor type.

