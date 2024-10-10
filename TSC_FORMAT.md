# TSC Saving Format - Draft 3

A highly compressable binary saving format with parallelizable encoding for Cell Machine.
It supports key-value pair cell data, cell flags, cell IDs (TSC cell ids), backgrounds and more.
It uses little endian byte order (least significant byte comes first).
This is for occasional performance boosts on x86_64 CPUs.

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
[<cstring>...]00
```
Where a cstring is defined as:
```
<string bytes>0
```
The `0` byte marking the end of the string.
Another 2 `0` bytes marks the end of the string intern table. (equivalent to as if you put 2 empty strings, but since each entry should be unique, it is treated as a terminator for the whole table)

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

## Cell structure

The cell array is either a list of rows or a list of columns. It is a list of columns if there are more columns than rows, otherwise it is a list of rows.
Each entry will be referred to as a "segment" since whether it is a column or a row, they are encoded the same.
This document clarifies the difference between how column and row indexes should be turned into x, y coordinates later.

## Each cell segment

The cell segment is a mixture of bytes and cell data.

There is no implicit start and finish, it ends when the next cells are no longer at that segment.

### The opcodes

NOTE: In the sizes of numbers, 0 means 1 byte, 7 means 8 bytes, etc.
And in the amount of cells, or amount to look back, there is also an implicit +1.
This is to ensure maximum efficiency, since otherwise 0 would be unused.

```c
0b01SSSCCC<cell ints...>
```
this is placing cells with a "small count".
SSS is how many bytes per cell & CCC is the amount itself.
After it is a list of cell integers (more on them later.

```c
0b10SSSCC0<cell count><cell ints...>
```
this is placing cells with a "large count".
SSS is how many bytes per cell & CC is the amount of bytes storing the amount of cells.
After it is a list of cell integers (more on them later.

```c
0b10ERF__1<effects?><registry?><flags?>
```

This makes the previous cell optionally have data.

It may have Effects, Registry or 1 byte Flags.
The 2 bits afterwards are unused and should be 0, as they may be used in the future.

After it should be Effects if enabled, Registry if enabled or the 1 byte flags if enabled.
They may not be in any order other than that.

```c
0b11BBBCCC
```
for short copy instruction, where BBB is the look-back integer and CCC is how many bytes the count takes up.

```c
0b00BBBCCC<look-back int><count int>
```
for long copy optimization, where BBB is the amount of bytes storing the look-back integer, and CCC is the size of the count integer.

### Copy Optimization

Copy Optimization is the same as it is in V3. The look-back is the amount of cells to look back, and the count is how many cells to copy.
The "slice" can run into itself. That is to say, the count can be bigger than the look-back integer.

### Effects

Effects are stored identically to the string ID array.

### Registry

```
[<key: non-0 string index><value: string index>]<string index of 0>
```

### Flags

Just one byte.

### Cell Integer

The cell integer, no matter how big it is, represents an ID, background and rotation.

It is calculated as the math expression `rotation + bg + id * #bg`, where:
- `rotation` is the rotation of the cell (0-3). Rotations are clockwise and a mover with 0 rotation would move to the right.
- `id` is the id index (where 0 is the first one).
- `bg` is the background ID index (where 0 is the first one).
- `#bg` is the amount of backgrounds.

### A note on cell IDs.

The `empty` ID, representing no cell, has special properties when it comes to rotation (technically they only apply to all segments except the first one)

A rotation of 0 means an actual empty cell.
A rotation of 1 means it is identical to the cell above it.
A rotation of 2 means it is identical to the cell top-left of it.
A rotation of 3 means it is identical to the cell top-right of it.
This allows encoding to only store a "delta" compared to the previous row, allowing it to potentially achieve better compression.

NOTE: by above it, we mean from the equivalent spot in the segment before it. If it is a column, it would be to the left of it.

### Simplification

Cells are allowed to be "simplified" during encoding.
The vanilla cells are simplified by having their rotations changed in ways that do not affect their behavior.
This is done so compression can be achieved, although technically qualifying it as lossy compression.

### Coordinate space

Given index i, which is from 0 to W\*H - 1, where W is the width and H is the height, and where i = 0 means the first decoded cell and i = W\*H-1 means the very last cell,

In this coordinate system, `0, 0` is the top left of the grid, with positive x being right and positive y being down.

If each segment is a row, then x = i mod W, and y = i / W (integer division, rounded down).
If each segment is a column, then y = i mod H, and x = i / H (still integer division)
