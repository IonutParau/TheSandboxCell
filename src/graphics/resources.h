#ifndef TSC_RESOURCES_H
#define TSC_RESOURCES_H

#include <raylib.h>
#include <stddef.h>
// hideapi
#include "../api/value.h"
// hideapi

// hideapi

// These are for the implementation.
// You likely won't care.

typedef struct tsc_music_t {
    const char *name;
    Music music;
    struct tsc_resourcepack *source;
} tsc_music_t;

typedef struct tsc_resourcearray {
    size_t len;
    const char **ids;
    void *memory;
    size_t itemsize;
} tsc_resourcearray;

typedef struct tsc_resourcetable {
    size_t arrayc;
    tsc_resourcearray *arrays;
    size_t *hashes;
    size_t itemsize;
} tsc_resourcetable;

typedef struct tsc_atlas {
    RenderTexture atlas;
    int cellWidth;
    int height;
    bool *supported;
} tsc_atlas;

typedef struct tsc_atlas_part {
    Texture texture;
    Rectangle part;
} tsc_atlas_part;

// Actual API

// hideapi

typedef struct tsc_resourcepack
// hideapi
{
    const char *id;
    const char *name;
    const char *description;
    const char *author;
    const char *readme;
    const char *license;
    tsc_resourcetable *textures;
    tsc_resourcetable *audio;
    Font *font;
    tsc_music_t *musictrack;
    size_t trackcount;
    tsc_value value;
    tsc_atlas *cellAtlas;
}
// hideapi
tsc_resourcepack;

// This is highly important shit
// If something isn't found, it is yoinked from here.
// If something isn't found here, then you'll likely get segfaults (or errors).
extern tsc_resourcepack *defaultResourcePack;
tsc_resourcepack *tsc_getResourcePack(const char *id);

// hideapi

tsc_resourcepack *tsc_createResourcePack(const char *id);
tsc_resourcepack *tsc_indexResourcePack(size_t idx);
void tsc_enableResourcePack(tsc_resourcepack *pack);
void tsc_disableResourcePack(tsc_resourcepack *pack);
// Moving it higher gives it higher priority
// Not implemented yet
// void tsc_moveResourcePack(tsc_resourcepack *pack, int amount);
tsc_resourcepack *tsc_indexEnabledResourcePack(size_t idx);

Texture textures_get(const char *key);
Color textures_getApproximation(const char *key);
tsc_atlas_part textures_getAtlasPart(tsc_id_t id);

Sound audio_get(const char *key);

Font font_get();

// Default is black with 0 opacity
int tsc_queryColor(const char *key);
int tsc_queryOptionalColor(const char *key, int defaultColor);

// hideapi

const char *tsc_textures_load(tsc_resourcepack *pack, const char *id, const char *file);
const char *tsc_sound_load(tsc_resourcepack *pack, const char *id, const char *file);

void tsc_sound_play(const char *id);

// hideapi
extern tsc_music_t tsc_currentTrack;
void tsc_music_load(tsc_resourcepack *pack, const char *name, const char *file);
void tsc_sound_playQueue();
tsc_music_t tsc_music_getRandom();
void tsc_music_playOrKeep();
// hideapi

#endif
