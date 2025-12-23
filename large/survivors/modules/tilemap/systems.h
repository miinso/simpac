#pragma once

#include <flecs.h>
#include <tmxlite/Layer.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/TileLayer.hpp>

#include "modules/engine/core/components.h"
#include "modules/engine/physics/components.h"
#include "modules/engine/physics/physics_module.h"
#include "modules/engine/rendering/components.h"
#include "modules/tilemap/components.h"

namespace tilemap {
    namespace systems {
        struct TilesetData {
            flecs::entity entity;
            tmx::Tileset tileset;
            Texture2D texture;
            int first_gid;
            int last_gid;

            TilesetData(flecs::entity e, const tmx::Tileset &ts, Texture2D tex) :
                entity(e), tileset(ts), texture(tex), first_gid(ts.getFirstGID()),
                last_gid(ts.getLastGID()) {}
        };

        struct TileTransform {
            Rectangle source;
            Rectangle destination;
            Vector2 origin;
            float rotation;
        };

        inline void load_tilesets(flecs::entity parent, const tmx::Map &map,
                                  std::vector<TilesetData> &tilesets) {
            const auto &tmx_tilesets = map.getTilesets();
            for (size_t i = 0; i < tmx_tilesets.size(); ++i) {
                const tmx::Tileset &tileset = tmx_tilesets[i];
                Texture2D texture = LoadTexture(tileset.getImagePath().c_str());

                flecs::entity tileset_entity =
                        parent.world().entity().child_of(parent).set<TilemapTileset>(
                                TilemapTileset{texture});

                tilesets.push_back(TilesetData(tileset_entity, tileset, texture));
            }
        }

        inline const TilesetData *find_tileset_for_tile(int tile_id,
                                                        const std::vector<TilesetData> &tilesets) {
            for (size_t i = 0; i < tilesets.size(); ++i) {
                const TilesetData &tileset = tilesets[i];
                if (tile_id >= tileset.first_gid && tile_id < tileset.last_gid) {
                    return &tileset;
                }
            }
            return NULL;
        }

        inline TileTransform calculate_tile_transform(const tmx::Tileset::Tile *tile,
                                                      const tmx::TileLayer::Tile &tile_data, int x,
                                                      int y, float scale) {
            TileTransform transform;

            int width = tile->imageSize.x;
            int height = tile->imageSize.y;
            transform.rotation = 0.0f;

            if (tile_data.flipFlags & tmx::TileLayer::FlipFlag::Diagonal) {
                transform.rotation = -90.0f;
                height = -height;
            }
            if (tile_data.flipFlags & tmx::TileLayer::FlipFlag::Horizontal) {
                width = -width;
            }
            if (tile_data.flipFlags & tmx::TileLayer::FlipFlag::Vertical) {
                height = -height;
            }

            // source rectangle (where to sicssor atlas)
            transform.source.x = static_cast<float>(tile->imagePosition.x);
            transform.source.y = static_cast<float>(tile->imagePosition.y);
            transform.source.width = static_cast<float>(width);
            transform.source.height = static_cast<float>(height);

            // Destination rectangle
            transform.destination.x = static_cast<float>(x) * scale * tile->imageSize.x;
            transform.destination.y = static_cast<float>(y) * scale * tile->imageSize.y;
            transform.destination.width = static_cast<float>(tile->imageSize.x) * scale;
            transform.destination.height = static_cast<float>(tile->imageSize.y) * scale;

            // Origin (rotation anchor)
            transform.origin.x = transform.destination.width / 2.0f;
            transform.origin.y = transform.destination.height / 2.0f;

            return transform;
        }

        inline bool is_tile_collidable(const tmx::Tileset::Tile *tile) {
            if (!tile || tile->properties.empty()) {
                return false;
            }

            for (size_t i = 0; i < tile->properties.size(); ++i) {
                if (tile->properties[i].getName() == "collide") {
                    return tile->properties[i].getBoolValue();
                }
            }
            return false;
        }

        inline void process_tile_layer(flecs::entity parent, const tmx::TileLayer &tile_layer,
                                       const tmx::Map &map, const Tilemap &tilemap,
                                       const std::vector<TilesetData> &tilesets,
                                       std::vector<bool> &collision_map) {

            tmx::Vector2u layer_size = tile_layer.getSize();
            tmx::Vector2u tile_size = map.getTileSize();

            // bake atlas to offscreen rendertexture
            RenderTexture2D layer_texture =
                    LoadRenderTexture(layer_size.x * tile_size.x * tilemap.scale,
                                      layer_size.y * tile_size.y * tilemap.scale);

            BeginTextureMode(layer_texture);
            ClearBackground(BLANK);

            flecs::entity layer_entity =
                    parent.world().entity(tile_layer.getName().c_str()).child_of(parent);

            const auto &tiles = tile_layer.getTiles();

            // traverse all tiles
            for (int y = 0; y < static_cast<int>(layer_size.y); ++y) {
                for (int x = 0; x < static_cast<int>(layer_size.x); ++x) {
                    int index = y * layer_size.x + x;
                    int tile_id = tiles[index].ID;

                    if (tile_id == 0)
                        continue; // empty tile

                    // search the tile in the set
                    const TilesetData *tileset_data = find_tileset_for_tile(tile_id, tilesets);
                    if (!tileset_data)
                        continue;

                    const tmx::Tileset::Tile *tile = tileset_data->tileset.getTile(tile_id);
                    if (!tile)
                        continue;

                    TileTransform transform =
                            calculate_tile_transform(tile, tiles[index], x, y, tilemap.scale);

                    // tile gets their own entity
                    TilemapLayerTile tile_component;
                    tile_component.tileset = tileset_data->entity;
                    tile_component.source = transform.source;
                    tile_component.destination = transform.destination;
                    tile_component.rotation = transform.rotation;

                    flecs::entity tile_entity = parent.world()
                                                        .entity()
                                                        .child_of(layer_entity)
                                                        .set<TilemapLayerTile>(tile_component);

                    if (is_tile_collidable(tile)) {
                        collision_map[index] = true;
                    }

                    DrawTexturePro(tileset_data->texture, transform.source, transform.destination,
                                   transform.origin, transform.rotation, WHITE);
                }
            }

            EndTextureMode();

            // RenderTexture2D inverted =
            //         LoadRenderTexture(layer_size.x * tile_size.x * tilemap.scale,
            //                           layer_size.y * tile_size.y * tilemap.scale);
            // BeginTextureMode(inverted);
            // DrawTexture(layer_texture.texture, 0, 0, WHITE);
            // EndTextureMode();

            // rendering::Renderable renderable = {inverted.texture, {0, 0}, 1.0f, WHITE};
            // rendering::Priority priority = {0};

            rendering::Renderable renderable = {layer_texture.texture, {0, 0}, 1.0f, WHITE};
            rendering::Priority priority = {0};

            layer_entity.set<rendering::Renderable>(renderable).set<rendering::Priority>(priority);
        }

        inline void merge_collision_rectangles(std::vector<bool> &collision_map,
                                               const tmx::Vector2u &map_size,
                                               const tmx::Vector2u &tile_size, float scale,
                                               std::vector<Rectangle> &merged_colliders) {

            for (int y = 0; y < static_cast<int>(map_size.y); ++y) {
                for (int x = 0; x < static_cast<int>(map_size.x); ++x) {
                    int current_index = y * map_size.x + x;

                    if (!collision_map[current_index]) {
                        continue;
                    }

                    int rect_x = x;
                    int rect_y = y;
                    int rect_width = 1;
                    int rect_height = 1;

                    // 1. expand to the right
                    while (rect_x + rect_width < static_cast<int>(map_size.x) &&
                           collision_map[y * map_size.x + (rect_x + rect_width)]) {
                        rect_width++;
                    }

                    // 2. expand to the bottom
                    bool can_extend_down = true;
                    while (can_extend_down && rect_y + rect_height < static_cast<int>(map_size.y)) {
                        // Check the entire row at current_y + rect_height for collisions within
                        // current rect_width
                        for (int current_rect_x = rect_x; current_rect_x < rect_x + rect_width;
                             ++current_rect_x) {
                            if (!collision_map[(rect_y + rect_height) * map_size.x +
                                               current_rect_x]) {
                                can_extend_down = false;
                                break; // This row cannot be fully extended
                            }
                        }
                        if (can_extend_down) {
                            rect_height++;
                        }
                    }

                    // Now we have the largest possible rectangle starting at (rect_x, rect_y)
                    // Mark all tiles within this rectangle as processed in the collision_map
                    for (int mark_y = rect_y; mark_y < rect_y + rect_height; ++mark_y) {
                        for (int mark_x = rect_x; mark_x < rect_x + rect_width; ++mark_x) {
                            collision_map[mark_y * map_size.x + mark_x] = false;
                        }
                    }

                    // transform to world coords
                    Rectangle merged_rect;
                    merged_rect.x = rect_x * tile_size.x * scale - tile_size.x * scale / 2.0f;
                    merged_rect.y = rect_y * tile_size.y * scale - tile_size.y * scale / 2.0f;
                    merged_rect.width = rect_width * tile_size.x * scale;
                    merged_rect.height = rect_height * tile_size.y * scale;

                    merged_colliders.push_back(merged_rect);
                }
            }
        }

        inline void create_collision_entities(flecs::entity parent,
                                              const std::vector<Rectangle> &merged_colliders) {
            for (size_t i = 0; i < merged_colliders.size(); ++i) {
                const Rectangle &col = merged_colliders[i];

                core::Position position({Eigen::Vector3f(col.x, col.y, 0)});
                physics::Collider collider = {
                        false, // is_correctable
                        true, // is_static
                        {0, 0, col.width, col.height}, // aabb
                        physics::environment, // collision_layer
                        physics::environment_filter, // collision_filter
                        physics::ColliderType::Box // type
                };

                parent.world()
                        .entity()
                        .set<core::Position>(position)
                        .set<physics::Collider>(collider)
                        .add<physics::BoxCollider>()
                        .add<physics::StaticCollider>();
            }
        }

        inline void create_tilemap(flecs::entity e, const Tilemap &tilemap) {
            tmx::Map map;
            if (!map.load(tilemap.tmx_file_path)) {
                return;
            }

            std::vector<TilesetData> tilesets;
            load_tilesets(e, map, tilesets);

            // collision
            tmx::Vector2u map_size = map.getTileCount();
            std::vector<bool> collision_map(map_size.x * map_size.y, false);

            // layers
            const auto &layers = map.getLayers();
            for (size_t i = 0; i < layers.size(); ++i) {
                const auto &layer = layers[i];
                if (layer->getType() == tmx::Layer::Type::Tile) {
                    const tmx::TileLayer &tile_layer = layer->getLayerAs<tmx::TileLayer>();
                    process_tile_layer(e, tile_layer, map, tilemap, tilesets, collision_map);
                }
            }

            // collision body
            std::vector<Rectangle> merged_colliders;
            merge_collision_rectangles(collision_map, map_size, map.getTileSize(), tilemap.scale,
                                       merged_colliders);


            create_collision_entities(e, merged_colliders);
        }
    } // namespace systems
} // namespace tilemap
