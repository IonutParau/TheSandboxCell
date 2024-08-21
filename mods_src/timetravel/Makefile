all: libtsc code
	mkdir -p ../../mods/timetravel
	cp -r textures ../../mods/timetravel
	cp config.json ../../mods/timetravel
libtsc:
	cd ../..; lua ./libtsc.lua generate > mods_src/more_movers/src/libtsc.h
code:
	mkdir -p ../../mods/timetravel/src
	cp -r src ../../mods/timetravel
	cp main.lua ../../mods/timetravel/main.lua
clean:
	rm -rf ../../mods/timetravel
