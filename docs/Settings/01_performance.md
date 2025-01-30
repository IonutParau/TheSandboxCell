# Performance

The performance section contains settings that affect the game's performance as a whole.
By default, TSC uses a capped TPS of 20, but may use all available computing power to reach it.
The TPS cap may be undesireable if you wish to run things at massive speeds (for this, change Update Delay), and using all available computing power may be
undesireable if you do not want maximum performance or if your entire computer or background apps start lagging from the game (for this, change Thread
Count or Multi-Tick Per Frame).

## Thread Count
> Short summary: "The default is often best. If your PC lags, set it to -1 or -2"

`Thread Count` is the number of [threads](https://en.wikipedia.org/wiki/Thread_(computing)) the game can use.
For those who don't know, threads are parts of a program running in parallel. More threads means more parallel work (though TSC may choose to not use multiple
threads for small grids to overcome bottlenecks). Unfortunately, TSC, for maximum performance, needs to overuse these threads, and since by default it has
as many as is practical, the default CPU usage for a massive grid is 100%. If it used half as many threads, for example, you'd only have slightly over 50%
usage at most.

> NOTE: TSC lies to you. It secretly has 2 extra threads. The thread that renders the game and handles inputs, called the Main Thread, and the thread that
processes ticks and tells the other threads to work, called the Update Thread. These 2 cannot be removed, as they are fundamental to the game's functionality,
and thus are ignored by the setting.

Setting the thread count to `0` will disable parallelism entirely. Setting it to `1` will keep the parallelism functionality, but only use one thread.
Setting it to another positive number will use that many threads (capped at how many your CPU allows to prevent you from thread-bombing your computer).
Setting it to a negative number will use all threads except a few. Essentially, setting it to `-2` will use all except 2 threads. This lets your OS use
those 2 hardware threads for background applications or drivers, to prevent TSC from lagging out your entire computer, at the cost of TPS. (Note: only on
*one* computer running Windows has extreme lag been seen)

## Update Delay

If you dont need insane TPS, you can set the update delay to a number other than `0`. This will make ticks happen at those regular intervals (unless they take
longer than said intervals, for which the game gives a warning). A non-`0` update delay also means the grid becomes subject to Tick Time Scaling and disables
the TPS counter (not to be confused with the Tick Counter, which is still shown).

Setting it back to `0` will revert it back to the default maximum TPS possible.
The range is between `0` and `1`, and is in seconds, rounded to 2 decimal places.

### Reference

- Cell Machine: 0.2s update delay (5 TPS) (Mystic Mod does not change this)
- The Puzzle Cell: 0.15s update delay (6.6666666666... TPS)
- The Sandbox Cell (by default): 0.05s update delay (20 TPS)

## Multi-Tick Per Frame

> Short summary: If your CPU keeps exploding while the game is idle, turn this off.

This is on by default.
For the average grid (sub 20,000 cell area), rendering the grid is slower than ticking it. To fix this bottleneck, MTpF was made. It will let the
Update Thread no longer wait for ticking signals (just pretend you understand these words) and instead constantly check if a tick should happen.
Of course, this means the Update Thread is now overused. This is often not a real issue, aside from reduced battery life.

Turning it off will not allow the TPS to go much past the framerate. This means your TPS becomes limited at your FPS, which for some grids can be a huge
difference, whilst for others it may not matter.
