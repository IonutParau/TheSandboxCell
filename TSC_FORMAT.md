# TSC Saving Format - Draft 2

A highly compressable binary saving format for Cell Machine. It supports key-value pair cell data, cell flags, cell IDs (TSC cell ids),
backgrounds and more. It uses little endian byte order (least significant byte comes first).

# Overall text structure

The text structure of `TSC;` is simple.
```
TSC;<base74 width>;<base74 height>;<title;<description>;<base2048 encoded deflate compressed binary cell data>
```

See [this](https://github.com/qntm/base2048/tree/main) for more details.

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
When going "back", it goes to the left, unless the border is reached, in which case it goes up one row and out the other side.

### Opcode format

Each opcode starts as a `byte`, but may take additional bytes as operands.
- The opcode `0` means skip 1 cell
- The opcodes `1` - `8` means skin `N+2` cells, where `N` is an `opcode` byte long integer immediately aftwards
- The opcodes `9` - `16` are followed by a `cell data integer` that is `opcode - 8` bytes long immediately afterwards. They add that extra cell
- The opcodes `17` - `24` mean "slice copy compressed" and are followed by 2 `opcode - 16` byte long integers (N & L). It means "go back (N+1) cells where last
cell and copy a horizontal slice of size (L+1)"
- The opcodes `25` - `32` mean "vertical slice copy compressed" and are followed by 2 `opcode - 24` byte long integers (N & L). It is like slice copy
compression, except it goes vertically instead of horizontally, starting at the bottom.
- The opcodes `33` - `40` mean "block copy compressed" and are followed by 3 `opcode - 32` byte long integers (N, W & H). It goes back `N` cells, and copies a
`W+1` x `L+1` block, starting at the bottomleft corner.

## Cell binary format

At a high level, the binary format of a cell can be viewed as a single interger, `Cell type data`.
`Cell type data` is an integer that encodes the ID, background ID, rotation, background rotation.

It can calculated like so:
```lua
-- id and background are indexes in the string intern table (they start at 1 and thus 1 is subtracted)
-- #ids is the length of the cell ID array
-- #backgrounds is the length of the background ID array

local n = rot + backgroundRot * 4 + id * 16 + background * #ids * 16
```

It is recommended you use the right opcode for the smallest integer needed to store that value.

### Cell data

Cell data is very easy to follow
```
[<non-0 string intern id of key><cstring value>]<string intern id of 0 terminator>
```
It is a continuous array of key-value pairs.
