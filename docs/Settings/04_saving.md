# Saving

Once a level is finished, you may wish to share it with others. These settings affect this process.

## V3 Speed Level

> Short summary: Higher number saves faster but makes significantly bigger level code

When saving the level as a V3 level (no modded cells allowed, weak compression, slow speed, supported everywhere), TSC will normally run the algorithm
to completeness with best compression (which, by the way, is better than CMMM's V3 encoder). However, for grids with an area larger than 100,000 cells, the
level of computing power needed for best compression skyrockets and the saving process becomes very slow. To allow these levels to still be saved with the
very portable format, you can increase this value.

All it does is let the compressor do less work. A higher number makes it faster, but with diminishing returns. Really high numbers also cause the level size to
skyrocket.
