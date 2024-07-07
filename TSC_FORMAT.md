# TSC Saving Format - Draft 1

A highly compressable binary saving format for Cell Machine. It supports key-value pair cell data, cell flags, cell IDs (TSC cell ids),
backgrounds and more. It uses little endian byte order (least significant byte comes first).

# Overall text structure

The text structure of `TSC;` is simple.
```
TSC;<base74 width>;<base74 height>;<title;<description>;<base85 encoded binary cell data>;
```

TSC's Base85 uses a modified key that is Markdown-safe, thus it can be freely shared on applications such as Discord which may process it as markdown.
The key is `!\"#$%&'{)x+,-.}0123456789;w<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[v]^yzabcdefghijklmnopqrstu`
The index of the character represents its associated value, indexes starting at 0.

# The binary format

The binary format, at a high level, can be though up as:
```
<string intern table><cell id array><background id array><cell opcodes>
```

However, each part is not so simple.

## String Intern Table

Cell IDs and cell data keys are often shared, thus, they are first put here and instead indexes to them are stored.
The binary structure of the intern table is like this:
```
[<cstring>...]0
```
Where a cstring is defined as:
```
<string bytes>0
```
The `0` byte marking the end of the string.
Another `0` byte marks the end of the string intern table.

Indexes into this table start at `1`. This is so `0` can be used in other places for marking terminators.
The size of the integers storing these indexes varies. It is between the range of `1` to `8` bytes, however, the exact size is equal to the minimum
size that would store the length of the string intern table (which is last the last index).

### Cell ID array and background ID array

Both arrays follow the same format.
```
[<non-0 string intern index>]<string intern index of 0>
```

The cell ID array contains the strings that represent cell IDs.
The background ID array contains the strings that represent background IDs.
Indexes into these arrays start at `0`.
There are useful for shrinking the possible numerical values that encode a cell type id (more on that later) which can reduce the overall size of
the level code.

## Cell opcodes

`Opcodes` are instructions telling the decoder how to assemble the grid.
The grid must start out as empty.

### Coordinate space and rotations

0 means right, 1 means down, 2 means left, 3 means up. +x is right, -x is left, +y is down, -y is up.

### Opcode format

NOTE: Cells should be treated as both on the grid and on a big array list. These 2 don't have to be the same.

Each opcode starts as a `byte`, but may take additional bytes as operands.
- The opcode `255` means the cell is skipped. This could either mean an empty cell, or that the cell was put there by other operands before it. It does NOT
push anything onto the cell array list, which means that all opcodes that look back also skip this cell.
- The opcodes `247` through `254` (inclusive) mean skipping in a row. After the opcode, a `opcode - 246` byte integer should be there to encode
the amount of cells skipped, starting at 0. These cells also aren't pushed.
- The opcode `0` means put a simple cell. After it should be the binary encoding of a `cell` (defined later)
- The opcodes `1` through `8` (inclusive) mean 1 cell length compressed. After the opcode are 2 integers (c and r) which are `opcode` bytes long.
c is how many cells to look back (0 meaning the last cell put there), and r means how many times to copy it.
- The opcodes `9` through `16` (inclusive) mean slice copy compressed. After the opcode are 3 integers (c, l and r) which are `opcode - 8` bytes long.
c is how many cells to look back, representing the start of the slice. l is the length of the slice. r is how many times to copy that slice.
- The opcodes `17` through `24` (inclusive) mean block compressed. After it are 4 integers (c, w, h and r) which are `opcode - 16`) bytes long.
c is how many cells to look back, representing the top left corner of the block. w and h are the width and height of the block. r is how many times to repeat
the block. This should put on the grid all rows of the block, but only push the top one `r` times. The other tiles are assumed to be later skipped, although
the encoder can re-encode them if they so choose (though it is not recommended, as you are storing duplicate data).

## Cell binary format

At a high level, the binary format of a cell can be viewed as:
```
<cell type data><flags?><cell data?>
```

`Cell type data` is an integer that encodes the ID, background ID, rotation, background rotation, whether it has flags, whether it has cell data.

It and `maxn` (more on it in a bit) are calculated like so:
```lua
-- id and background are indexes in the string intern table (they start at 1 and thus 1 is subtracted)
-- hasflags and hasdata are 0 if false and 1 if true
-- #strings is the length of the string intern table

local n = rot + backgroundRot * 4 + hasflags * 16 + hasdata * 32 + id * 64 + background * 64 * #ids
-- Each part is set to its highest value
local maxn = 3 + 3 * 4 + 1 * 16 + 1 * 32 + (#ids-1) * 64 + (#backgrounds-1) * 64 * #ids
-- Can be simplified to
local maxn = 63 + (#ids-1) * 64 + (#backgrounds-1) * 64 * #ids
```

The length of the `cell type data` integer varies as well. It is the amount of bytes minimum to store `maxn`.

`<flags?>` is only there if hasflags is `1`. If it is there, it is an 8 byte integer. If flags isn't there, you can assume flags are set to `0`.

### Cell data

Cell data is very easy to follow
```
[<non-0 string intern id of key><cstring value>]<string intern id of 0 terminator>
```
It is a continuous array of key-value pairs.
