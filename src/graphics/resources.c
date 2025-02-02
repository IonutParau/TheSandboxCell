// The soon-to-be general resource pack manager
// Fairly efficient, very high-level and fault tolerant.

#include <math.h>
#include <raylib.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "resources.h"
#include "../utils.h"
#include "../api/api.h"
#include "../cells/grid.h"
#include "../api/tscjson.h"

tsc_resourcepack *defaultResourcePack;

tsc_resourcepack **rp_all = NULL;
size_t rp_allc = 0;

tsc_resourcepack **rp_enabled = NULL;
size_t rp_enabledc = 0;

typedef struct tsc_texture_resource {
    volatile Texture texture;
    volatile Color approximation;
} tsc_texture_resource;

// Not good for BIG textures.
static Color tsc_texture_computeApproximation(Texture texture) {
    // Copy to CPU the image buffer
    Image image = LoadImageFromTexture(texture);
    Color *colors = LoadImageColors(image);

    double r = 0, g = 0, b = 0;
    double div = 0;
    for(int x = 0; x < image.width; x++) {
        for(int y = 0; y < image.height; y++) {
            int i = y * image.width + x;
            Color pixel = colors[i];
            double red, green, blue, alpha = 0;
            red = ((double)pixel.r) / 255;
            green = ((double)pixel.g) / 255;
            blue = ((double)pixel.b) / 255;
            alpha = ((double)pixel.a) / 255;
            div += alpha;
            red *= red;
            green *= green;
            blue *= blue;

            r += red * alpha;
            g += green * alpha;
            b += blue * alpha;
        }
    }

    r = sqrt(r/div);
    g = sqrt(g/div);
    b = sqrt(b/div);

    // Cleanup
    UnloadImageColors(colors);
    UnloadImage(image);

    Color out = {.r = (unsigned char) (r * 255), .g = (unsigned char) (g * 255), .b = (unsigned char) (b * 255), .a = 255};
    return out;
}

static tsc_resourcetable *rp_createResourceTable(size_t itemsize) {
    tsc_resourcetable *table = malloc(sizeof(tsc_resourcetable));
    table->itemsize = itemsize;
    table->arrayc = 20;
    table->hashes = malloc(sizeof(size_t) * table->arrayc);
    table->arrays = malloc(sizeof(tsc_resourcearray) * table->arrayc);
    for(size_t i = 0; i < table->arrayc; i++) {
        table->hashes[i] = 0;
        tsc_resourcearray array;
        array.len = 0;
        array.ids = NULL;
        array.memory = NULL;
        array.itemsize = itemsize;
        table->arrays[i] = array;
    }
    return table;
}

// not static so main can do weird stuff
void *rp_resourceTableGet(tsc_resourcetable *table, const char *id) {
    size_t hash = (size_t)id;
    size_t idx = hash % table->arrayc;

    if(table->hashes[idx] != hash) return NULL;
    tsc_resourcearray array = table->arrays[idx];
    for(size_t i = 0; i < array.len; i++) {
        if(array.ids[i] == id) {
            // This is to support the actual C standard
            // void* math is invalid, but if it is allowed, technically it is useless as
            // sizeof(void) == 0.
            // However, GCC and Clang have sizeof(void) == 1.
            size_t addr = (size_t)array.memory;
            return (void *)(addr + i * array.itemsize);
        }
    }
    return NULL;
}

static void rp_resourceTablePut(tsc_resourcetable *table, const char *id, const void *item) {
    id = tsc_strintern(id);
    size_t hash = (size_t)id;
start:;
    size_t idx = hash % table->arrayc;
    tsc_resourcearray *array = table->arrays + idx;
    if(array->len == 0) {
        // Uninitialized, shove it here
        table->hashes[idx] = hash;
        array->len = 1;
        array->ids = malloc(sizeof(const char *));
        array->ids[0] = id;
        array->memory = malloc(array->itemsize);
        memcpy(array->memory, item, array->itemsize);
        return;
    }
    if(table->hashes[idx] != hash) {
        // Initialized (because uninitialized would've been handled first)
        // But the backing array is too small.
        // Resize
        size_t arrayc = table->arrayc * 2;
        tsc_resourcearray *arrays = malloc(sizeof(tsc_resourcearray) * arrayc);
        size_t *hashes = malloc(sizeof(size_t) * arrayc);
        for(size_t i = 0; i < arrayc; i++) {
            hashes[i] = 0;
            arrays[i].len = 0;
            arrays[i].ids = NULL;
            arrays[i].memory = NULL;
            arrays[i].itemsize = table->itemsize;
        }

        // Relocate
        for(size_t i = 0; i < table->arrayc; i++) {
            if(table->arrays[i].len == 0) continue;
            size_t idx = table->hashes[i] % arrayc;
            hashes[idx] = table->hashes[i];
            arrays[idx] = table->arrays[i];
        }
        free(table->arrays);
        free(table->hashes);
        table->arrays = arrays;
        table->hashes = hashes;
        table->arrayc = arrayc;
        goto start;
    }

    // Check for duplicates (for warning)
    for(size_t i = 0; i < array->len; i++) {
        if(array->ids[i] == id) {
            size_t addr = (size_t)array->memory;
            addr += i * array->itemsize;
            memcpy((void*)addr, item, array->itemsize);
            return;
        }
    }
    // Finally, just push.
    size_t i = array->len++;
    array->ids = realloc(array->ids, sizeof(const char *) * array->len);
    array->memory = realloc(array->memory, array->itemsize * array->len);
    size_t addr = (size_t)array->memory;
    addr += i * array->itemsize;
    memcpy((void*)addr, item, array->itemsize);
}

static void rp_init_textures(tsc_resourcepack *pack, const char *path, const char *modid) {
    static char buffer[2048];
    size_t bufsize = 2048;

    size_t filec;
    char **files = tsc_dirfiles(path, &filec);

    for(size_t i = 0; i < filec; i++) {
        char *file = files[i];
        const char *ext = tsc_fextension(file);
        if(ext == NULL) {
            snprintf(buffer, bufsize, "%s/%s", path, file);
            tsc_pathfix(buffer);
            if(modid == NULL && tsc_streql(file, "mods")) {
                size_t modc;
                char **mods = tsc_dirfiles(buffer, &modc);
                char *dirpath = tsc_strdup(buffer);
                for(size_t i = 0; i < modc; i++) {
                    snprintf(buffer, bufsize, "%s/mods/%s", path, mods[i]);
                    tsc_pathfix(buffer);
                    char *modDir = tsc_strdup(buffer);
                    rp_init_textures(pack, modDir, mods[i]);
                    free(modDir);
                }
                free(dirpath);
                tsc_freedirfiles(mods);
                continue;
            }
            // Copy because buffer is re-used when recursing.
            char *dirpath = tsc_strdup(buffer);
            rp_init_textures(pack, dirpath, modid);
            free(dirpath);
            continue;
        } else {
            const char *exts[] = {
                "png", "jpg", "jpeg",
            };
            size_t extc = sizeof(exts) / sizeof(const char *);
            for(size_t i = 0; i < extc; i++) {
                if(tsc_streql(exts[i], ext)) goto valid_extension;
            }
            continue; // if not valid, skip.
valid_extension:
            snprintf(buffer, bufsize, "%s/%s.%s", path, file, ext);
            tsc_pathfix(buffer);
            printf("Loading %s\n", buffer);
            Texture texture = LoadTexture(buffer);
            Color approximation = tsc_texture_computeApproximation(texture);
            tsc_texture_resource resource = {texture, approximation};
            if(modid == NULL) {
                rp_resourceTablePut(pack->textures, file, &resource);
            } else {
                snprintf(buffer, bufsize, "%s:%s", modid, file);
                rp_resourceTablePut(pack->textures, buffer, &resource);
            }
        }
    }

    tsc_freedirfiles(files);
}

static const char **rp_allsounds = NULL;
static size_t rp_allsoundc = 0;
static tsc_resourcetable *soundQueued = NULL;

static void rp_registerSound(const char *id) {
    for(size_t i = 0; i < rp_allsoundc; i++) {
        if(rp_allsounds[i] == id) return;
    }
    size_t idx = rp_allsoundc++;
    rp_allsounds = realloc(rp_allsounds, sizeof(const char *) * rp_allsoundc);
    rp_allsounds[idx] = id;
    bool no = false;
    rp_resourceTablePut(soundQueued, id, &no);
}

static void rp_init_sounds(tsc_resourcepack *pack, const char *path, const char *modid) {
    if(soundQueued == NULL) {
        soundQueued = rp_createResourceTable(sizeof(bool));
    }
    static char buffer[2048];
    size_t bufsize = 2048;

    size_t filec;
    char **files = tsc_dirfiles(path, &filec);

    for(size_t i = 0; i < filec; i++) {
        char *file = files[i];
        const char *ext = tsc_fextension(file);
        if(ext == NULL) {
            snprintf(buffer, bufsize, "%s/%s", path, file);
            tsc_pathfix(buffer);
            if(modid == NULL && tsc_streql(file, "mods")) {
                size_t modc;
                char **mods = tsc_dirfiles(buffer, &modc);
                char *dirpath = tsc_strdup(buffer);
                for(size_t i = 0; i < modc; i++) {
                    snprintf(buffer, bufsize, "%s/mods/%s", path, mods[i]);
                    tsc_pathfix(buffer);
                    char *modDir = tsc_strdup(buffer);
                    rp_init_sounds(pack, modDir, mods[i]);
                    free(modDir);
                }
                free(dirpath);
                tsc_freedirfiles(mods);
                continue;
            }
            // Copy because buffer is re-used when recursing.
            char *dirpath = tsc_strdup(buffer);
            rp_init_sounds(pack, dirpath, modid);
            free(dirpath);
            continue;
        } else {
            const char *exts[] = {
                "wav", "ogg", "mp3",
            };
            size_t extc = sizeof(exts) / sizeof(const char *);
            for(size_t i = 0; i < extc; i++) {
                if(tsc_streql(exts[i], ext)) goto valid_extension;
            }
            continue; // if not valid, skip.
valid_extension:
            snprintf(buffer, bufsize, "%s/%s.%s", path, file, ext);
            tsc_pathfix(buffer);
            printf("Loading %s\n", buffer);
            Sound sound = LoadSound(buffer);
            const char *key;
            if(modid == NULL) {
                rp_resourceTablePut(pack->audio, file, &sound);
                key = tsc_strintern(file);
            } else {
                snprintf(buffer, bufsize, "%s:%s", modid, file);
                rp_resourceTablePut(pack->audio, buffer, &sound);
                key = tsc_strintern(buffer);
            }
            rp_registerSound(key);
        }
    }

    tsc_freedirfiles(files);
}

static void rp_init_music(tsc_resourcepack *pack, const char *path) {
    static char buffer[2048];
    size_t bufsize = 2048;

    size_t filec;
    char **files = tsc_dirfiles(path, &filec);

    for(size_t i = 0; i < filec; i++) {
        char *file = files[i];
        const char *ext = tsc_fextension(file);
        if(ext == NULL) {
            snprintf(buffer, bufsize, "%s/%s", path, file);
            tsc_pathfix(buffer);
            // Copy because buffer is re-used when recursing.
            char *dirpath = tsc_strdup(buffer);
            rp_init_music(pack, dirpath);
            free(dirpath);
            continue;
        } else {
            const char *exts[] = {
                "wav", "ogg", "mp3",
            };
            size_t extc = sizeof(exts) / sizeof(const char *);
            for(size_t i = 0; i < extc; i++) {
                if(tsc_streql(exts[i], ext)) goto valid_extension;
            }
            continue; // if not valid, skip.
valid_extension:
            snprintf(buffer, bufsize, "%s/%s.%s", path, file, ext);
            tsc_pathfix(buffer);
            printf("Loading %s\n", buffer);
            // tsc_fextension mutated file such that it contains the file name.
            tsc_music_load(pack, file, buffer);
        }
    }

    tsc_freedirfiles(files);
}

// Shoves everything into pack that is obtainable.
static void rp_init(tsc_resourcepack *pack) {
    // 2kb for a filepath is very extreme.
    // If you need more, reconsider your life choices, resource pack developers.
    // Yes UTF-8 files will take up more space than they seem, but 2kb is still extreme.
    static char rpPath[2048];
    // rpPath only stores path to Resource Pack
    // path is used for sub-stuff
    static char path[2048];
    size_t bufsize = 2048;

    snprintf(rpPath, bufsize, "data/resources/%s", pack->id);
    tsc_pathfix(rpPath);

    snprintf(path, bufsize, "%s/textures", rpPath);
    tsc_pathfix(path);
    rp_init_textures(pack, path, NULL);

    snprintf(path, bufsize, "%s/audio", rpPath);
    tsc_pathfix(path);
    rp_init_sounds(pack, path, NULL);

    snprintf(path, bufsize, "%s/font.ttf", rpPath);
    tsc_pathfix(path);
    if(tsc_hasfile(path)) {
        pack->font = malloc(sizeof(Font));
        *pack->font = LoadFontEx(path, 32, NULL, 0);
    }

    snprintf(path, bufsize, "%s/music", rpPath);
    tsc_pathfix(path);
    rp_init_music(pack, path);
}

tsc_resourcepack *tsc_createResourcePack(const char *id) {
    tsc_resourcepack *pack = malloc(sizeof(tsc_resourcepack));

    static char packFileBuf[2048];
    size_t packFileBufSize = 2048;

    snprintf(packFileBuf, packFileBufSize, "data/resources/%s/pack.json", id);

    char *packJson = tsc_allocfile(packFileBuf, NULL);

    tsc_saving_buffer jsonErr = tsc_saving_newBuffer("");
    pack->value = tsc_json_decode(packJson, &jsonErr);
    if(!tsc_isObject(pack->value)) {
        tsc_destroy(pack->value);
        pack->value = tsc_object();
    }
    if(jsonErr.len != 0) {
        fprintf(stderr, "JSON Error: %s\n", jsonErr.mem);
    }

    pack->id = id;
    // Start with a nothingburger (not really a nothingburger anymore)
    pack->name = tsc_toString(tsc_getKey(pack->value, "name"));
    if(pack->name[0] == '\0') pack->name = NULL;
    
    pack->description = tsc_toString(tsc_getKey(pack->value, "description"));
    if(pack->description[0] == '\0') pack->description = NULL;
    
    pack->author = tsc_toString(tsc_getKey(pack->value, "author"));
    if(pack->author[0] == '\0') pack->author = NULL;

    pack->readme = NULL;
    pack->license = NULL;

    pack->textures = rp_createResourceTable(sizeof(tsc_texture_resource));
    pack->audio = rp_createResourceTable(sizeof(Sound));
    pack->font = NULL;
    pack->musictrack = NULL;
    pack->trackcount = 0;

    // Add the toppings
    rp_init(pack);

    // Yeet it to the customer
    size_t idx = rp_allc++;
    rp_all = realloc(rp_all, sizeof(tsc_resourcepack *) * rp_allc);
    rp_all[idx] = pack;
    return pack;
}

tsc_resourcepack *tsc_getResourcePack(const char *id) {
    id = tsc_strintern(id);
    for(size_t i = 0; i < rp_allc; i++) {
        if(rp_all[i]->id == id) {
            return rp_all[i];
        }
    }
    return NULL;
}

tsc_resourcepack *tsc_indexResourcePack(size_t idx) {
    if(idx >= rp_allc) return NULL;
    return rp_all[idx];
}

void tsc_enableResourcePack(tsc_resourcepack *pack) {
    size_t idx = rp_enabledc++;
    rp_enabled = realloc(rp_enabled, sizeof(tsc_resourcepack *) * rp_enabledc);
    rp_enabled[idx] = pack;

    Texture icon = textures_get(builtin.textures.icon);
    Image image = LoadImageFromTexture(icon);
    SetWindowIcon(image);
    UnloadImage(image);
}

void tsc_disableResourcePack(tsc_resourcepack *pack) {
    if(rp_enabledc == 0) return;
    size_t j = 0;
    for(size_t i = 0; i < rp_enabledc; i++) {
        if(rp_enabled[i] == pack) {
            j = i;
            goto found;
        }
    }
    return; // not found
found:;
    for(size_t i = j; i < rp_enabledc-1; i++) {
        rp_enabled[i] = rp_enabled[i+1];
    }
    rp_enabledc--;
    if(rp_enabledc > 0) {
        rp_enabled = realloc(rp_enabled, sizeof(tsc_resourcepack *) * rp_enabledc);
    } else {
        free(rp_enabled);
        rp_enabled = NULL;
    }
    Texture icon = textures_get(builtin.textures.icon);
    Image image = LoadImageFromTexture(icon);
    SetWindowIcon(image);
    UnloadImage(image);
}

tsc_resourcepack *tsc_indexEnabledResourcePack(size_t idx) {
    if(idx >= rp_enabledc) return NULL;
    return rp_enabled[idx];
}

Texture textures_get(const char *key) {
start:;
    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(rp_enabledc - i - 1);
        if(pack == NULL) {
            continue;
        }
        tsc_texture_resource *texture = rp_resourceTableGet(pack->textures, key);
        if(texture != NULL) return texture->texture;
    }

    if(key != NULL && tsc_streql(key, "fallback")) {
        // Fallback texture is not found, panic.
        fprintf(stderr, "fallback.png missing. Please ensure at least one enabled resource pack contains it\n");
        exit(1);
    }

    key = tsc_strintern("fallback");
    goto start;
}

Color textures_getApproximation(const char *key) {
start:;
    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(rp_enabledc - i - 1);
        if(pack == NULL) continue;
        tsc_texture_resource *texture = rp_resourceTableGet(pack->textures, key);
        if(texture != NULL) return texture->approximation;
    }

    if(tsc_streql(key, "fallback")) {
        // Fallback texture is not found, panic.
        fprintf(stderr, "fallback.png missing. Please ensure at least one enabled resource pack contains it\n");
        exit(1);
    }

    key = tsc_strintern("fallback");
    goto start;
}

Sound audio_get(const char *key) {
start:;
    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(rp_enabledc - i - 1);
        Sound *sound = rp_resourceTableGet(pack->audio, key);
        if(sound != NULL) return *sound;
    }

    if(tsc_streql(key, "fallback")) {
        // Fallback texture is not found, panic.
        fprintf(stderr, "fallback.wav missing. Please ensure at least one enabled resource pack contains it\n");
        exit(1);
    }

    key = tsc_strintern("fallback");
    goto start;
}

Font font_get() {
    if(rp_enabledc == 0) return GetFontDefault();

    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(rp_enabledc - i - 1);
        if(pack->font != NULL) return *pack->font;
    }

    return GetFontDefault();
}

int tsc_queryColor(const char *key) {
    return tsc_queryOptionalColor(key, 0);
}

int tsc_queryOptionalColor(const char *key, int defaultColor) {
    if(rp_enabledc == 0) return defaultColor;

    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = tsc_indexEnabledResourcePack(rp_enabledc - i - 1);
        if(pack == NULL) continue;

        const char *s = tsc_toString(tsc_getKey(pack->value, key));
        if(s[0] == '\0') continue; // Not valid, whatever
        if(s[0] == '#') s++;
        
        int c = 0;

        // yes invalid shit can give you weird colors, no I don't care
        for(int i = 0; s[i] != '\0'; i++) {
            char x = s[i];
            int y = 0;
            if(x >= '0' && x <= '9') {
                y = x - '0';
            }
            if(x >= 'A' && x <= 'F') {
                y = x - 'A' + 10;
            }
            if(x >= 'a' && x <= 'f') {
                y = x - 'a' + 10;
            }
            c *= 0x10;
            c += y;
        }

        if(strlen(s) == 6) {
            c *= 0x100;
            c += 0xFF;
        }

        // Color is actually read in reverse byte order so we just reverse it again
        char buf[4] = {0};
        for(int i = 0; i < 4; i++) {
            buf[3 - i] = (c >> (i*8)) & 0xFF;
        }

        return c;
    }

    return defaultColor;
}

const char *tsc_textures_load(tsc_resourcepack *pack, const char *id, const char *file) {
    static char filepath[1024];
    snprintf(filepath, 1024, "data/mods/%s/textures/%s", tsc_currentModID(), file);
    tsc_pathfix(filepath);

    const char *resource = tsc_strintern(tsc_padWithModID(id));
    Texture texture = LoadTexture(filepath);
    tsc_texture_resource tex;
    tex.texture = texture;
    tex.approximation = tsc_texture_computeApproximation(texture);
    rp_resourceTablePut(pack->textures, resource, &tex);
    return resource;
}

const char *tsc_sound_load(tsc_resourcepack *pack, const char *id, const char *file) {
    static char filepath[1024];
    snprintf(filepath, 1024, "data/mods/%s/audio/%s", tsc_currentModID(), file);
    tsc_pathfix(filepath);

    const char *resource = tsc_strintern(tsc_padWithModID(id));
    Sound sound = LoadSound(filepath);
    rp_resourceTablePut(pack->audio, resource, &sound);
    rp_registerSound(resource);
    return resource;
}

void tsc_sound_play(const char *id) {
    // bool no = false;'s evil twin
    bool yes = true;
    rp_resourceTablePut(soundQueued, id, &yes);
}

void tsc_sound_playQueue() {
    // no: return of the false
    bool no = false;
    for(size_t i = 0; i < rp_allsoundc; i++) {
        const char *id = rp_allsounds[i];
        bool *queued = rp_resourceTableGet(soundQueued, id);
        // Who fucked up soundQueued?
        if(queued == NULL) continue;
        Sound sound = audio_get(id);
        SetSoundVolume(sound, tsc_toNumber(tsc_getSetting(builtin.settings.sfxVolume)));
        if(!*queued) continue; // not queued, move on
        rp_resourceTablePut(soundQueued, id, &no);
        if(IsSoundPlaying(sound)) continue; // fixes my eardrums
        PlaySound(sound);
    }
}

void tsc_music_load(tsc_resourcepack *pack, const char *name, const char *file) {
    // name must be valid forever so this is a quick hack
    name = tsc_strintern(name);
    tsc_music_t music;
    music.name = name;
    music.music = LoadMusicStream(file);
    music.music.looping = false;
    music.source = pack;

    size_t idx = pack->trackcount++;
    pack->musictrack = realloc(pack->musictrack, sizeof(tsc_music_t) * pack->trackcount);
    pack->musictrack[idx] = music;
}

tsc_music_t tsc_music_getRandom() {
    tsc_music_t *enabledMusic = NULL;
    size_t len = 0;

    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = rp_enabled[i];
        len += pack->trackcount;
    }

    if(len == 0) {
        tsc_music_t notfound;
        // notfound.music is undefined
        notfound.name = NULL;
        notfound.source = NULL;
        return notfound;
    }

    enabledMusic = malloc(sizeof(tsc_music_t) * len);
    size_t j = 0;

    for(size_t i = 0; i < rp_enabledc; i++) {
        tsc_resourcepack *pack = rp_enabled[i];
        for(size_t k = 0; k < pack->trackcount; k++) {
            enabledMusic[j] = pack->musictrack[k];
            j++;
        }
    }

    int idx = rand() % len;
    tsc_music_t choice = enabledMusic[idx];
    free(enabledMusic);
    return choice;
}

tsc_music_t tsc_currentTrack = {NULL, {0}, NULL};

void tsc_music_playOrKeep() {
    if(tsc_currentTrack.name == NULL) {
        tsc_currentTrack = tsc_music_getRandom();
    }

    // If it is STILL NULL, then somehow no music exists so do nothing
    if(tsc_currentTrack.name == NULL) return;
    // That epic banger is still blasting so we can't stop it
    if(IsMusicStreamPlaying(tsc_currentTrack.music) && !IsKeyPressed(KEY_M)) {
        SetMusicVolume(tsc_currentTrack.music, tsc_toNumber(tsc_getSetting(builtin.settings.musicVolume)));
        UpdateMusicStream(tsc_currentTrack.music);
        return;
    }

    tsc_currentTrack = tsc_music_getRandom();
    SetMusicVolume(tsc_currentTrack.music, tsc_toNumber(tsc_getSetting(builtin.settings.musicVolume)));
    SeekMusicStream(tsc_currentTrack.music, 0);
    PlayMusicStream(tsc_currentTrack.music);
    printf("Playing music track %s from %s\n", tsc_currentTrack.name, tsc_currentTrack.source->id);
}
