#pragma once

#include <flecs.h>
#include <raylib.h>
#include <string>

namespace tilemap {
    struct Tilemap {
        std::string tmx_file_path;
        float scale;
    };

    struct TilemapTileset {
        Texture2D texture;
    };

    struct TilemapLayer {
        Texture2D texture;
    };

    struct TilemapLayerTile {
        flecs::entity_t tileset;
        Rectangle source;
        Rectangle destination;
        float rotation;
    };
} // namespace tilemap
