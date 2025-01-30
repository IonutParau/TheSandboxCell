# Architecture

This is a very technical explanation of the game's architecture. This is useful for modders or people looking to embed the game's engine.

## The parts of the game

- The game, which handles rendering, input, settings, loading texture packs and when ticking happens
- The engine, which handles ticking, loading and saving levels, loading mods and platforms and provides some utilities.

> NOTE: currently, these 2 parts are not separated well. In the future they will, so the engine can be used independently of the game.

## Mods and Platforms

There are 2 types of mods, native mods and scripted mods. The difference is that scripted mods depend on a platform.

### Mods

Mods extend the functionality of the game by adding cells or saving formats.
If a mod has a `mod.so` and no `platform` in `config.json`, it is a native mod, in which case `mod.dll` (or `.so` / `.dylib` depending on OS) is loaded.
Yes, this means mods can load any DLL, and thus run any code. Do not download mods from random people, they can contain viruses!

If a mod has a `platform` specified in its `config.json`, it will be loaded with it.

### Platforms

Platforms also provide a DLL that can be loaded, however instead of it loading new cells, it is meant to provide a function used to load mods.
It is assumed that the platform sets up some kind of interpreter and bindings to TSC's API, to allow support for more languages. A platform can theoretically
support any language who's interpreter can be a DLL and interact with C code, which includes (but is not limited to) Lua, Python, WebAssembly, etc.

> NOTE: Platforms are a dependency. If a mod is made and needs a Lua platform called lua54 for example, the user of the mod needs to have that platform
installed, or else the game prints an error to console and exits (on Windows, the console is invisible by default)

## Compiling from source

> This is not often needed. You dont need to link with the libtsc DLL for a mod or platform as the symbols are resolved by the dynamic linker.
If you wish to embed it into another project, you can link the libtsc DLL the game ships. The static library is rarely needed, as it breaks modding by
confusing the dynamic linker.

The game provides a Lua script (`new_build.lua`) to compile the game. It needs the LuaFileSystem library installed, which can be installed with LuaRocks.
Examples for compiling the game from source:
```sh
# syntax: lua new_build.lua arg -f --longFlags --this=that
# the example above passes in arg as a normal argument, f as an option set to true, longFlags as an option set to true, and this as an option set to that.

# tell it to compile the game, in release mode (all optimizations), verbosely (everything it does is logged to the console), for the host target (configurable
# with --target=<linux|windows>). This compiles with the clang C compiler.
lua new_build.lua game --mode=release -v

# This command would tell the build script to compile only the library (as a DLL), in debug mode (less performance), verbosely, with gcc as the C compiler
# and clang as the linker (an odd choice, but just an example. Linker defaults to C compiler and is assumed to link in libc)
# If you compile the game, it automatically also compiles the library
lua new_build.lua lib --mode=debug -v --cc=gcc --ld=clang

# Clean-up. Deletes the build cache and output.
lua new_build.lua clean

# Rarely use. Compiles libtsc to a static library, but subsequently breaks modding.
lua new_build.lua lib --mode=release --static -v
```
