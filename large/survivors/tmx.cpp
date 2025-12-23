// https://github.com/fallahn/tmxlite/blob/master/ParseTest/src/main.cpp

#include <tmxlite/FreeFuncs.hpp>
#include <tmxlite/LayerGroup.hpp>
#include <tmxlite/Map.hpp>
#include <tmxlite/ObjectGroup.hpp>
#include <tmxlite/TileLayer.hpp>
#include <tmxlite/Tileset.hpp>

#include <array>
#include <iostream>
#include <string>

namespace {

    const std::array<std::string, 4u> LayerStrings = {
            std::string("Tile"),
            std::string("Object"),
            std::string("Image"),
            std::string("Group"),
    };

    void testLoadMap() {
        tmx::Map map;

        if (map.load("resources/tmx/maps/platform.tmx")) {
            std::cout << "Loaded Map version: " << map.getVersion().upper << ", "
                      << map.getVersion().lower << std::endl;
            if (map.isInfinite()) {
                std::cout << "Map is infinite.\n";
            } else {
                std::cout << "Map Dimensions: " << map.getBounds() << std::endl;
            }

            const auto &mapProperties = map.getProperties();
            std::cout << "Map class: " << map.getClass() << std::endl;

            std::cout << "Map tileset has " << map.getTilesets().size() << " tilesets" << std::endl;
            for (const auto &tileset: map.getTilesets()) {
                std::cout << "Tileset: " << tileset.getName() << std::endl;
                std::cout << "Tileset class: " << tileset.getClass() << std::endl;
                std::cout << "Tileset first GID: " << tileset.getFirstGID() << std::endl;
            }

            std::cout << "Map has " << mapProperties.size() << " properties" << std::endl;
            for (const auto &prop: mapProperties) {
                std::cout << "Found property: " << prop.getName() << std::endl;
                std::cout << "Type: " << int(prop.getType()) << std::endl;
            }

            std::cout << std::endl;

            const auto &layers = map.getLayers();
            std::cout << "Map has " << layers.size() << " layers" << std::endl;
            for (const auto &layer: layers) {
                std::cout << "Found Layer: " << layer->getName() << std::endl;
                std::cout << "Layer Type: "
                          << LayerStrings[static_cast<std::int32_t>(layer->getType())] << std::endl;
                std::cout << "Layer Dimensions: " << layer->getSize() << std::endl;
                std::cout << "Layer Tint: " << layer->getTintColour() << std::endl;

                if (layer->getType() == tmx::Layer::Type::Group) {
                    std::cout << "Checking sublayers" << std::endl;
                    const auto &sublayers = layer->getLayerAs<tmx::LayerGroup>().getLayers();
                    std::cout << "LayerGroup has " << sublayers.size() << " layers" << std::endl;
                    for (const auto &sublayer: sublayers) {
                        std::cout << "Found Layer: " << sublayer->getName() << std::endl;
                        std::cout << "Sub-layer Type: "
                                  << LayerStrings[static_cast<std::int32_t>(sublayer->getType())]
                                  << std::endl;
                        std::cout << "Sub-layer Class: " << sublayer->getClass() << std::endl;
                        std::cout << "Sub-layer Dimensions: " << sublayer->getSize() << std::endl;
                        std::cout << "Sub-layer Tint: " << sublayer->getTintColour() << std::endl;

                        if (sublayer->getType() == tmx::Layer::Type::Object) {
                            std::cout
                                    << sublayer->getName() << " has "
                                    << sublayer->getLayerAs<tmx::ObjectGroup>().getObjects().size()
                                    << " objects" << std::endl;
                        } else if (sublayer->getType() == tmx::Layer::Type::Tile) {
                            std::cout << sublayer->getName() << " has "
                                      << sublayer->getLayerAs<tmx::TileLayer>().getTiles().size()
                                      << " tiles" << std::endl;
                        }
                    }
                }

                if (layer->getType() == tmx::Layer::Type::Object) {
                    const auto &objects = layer->getLayerAs<tmx::ObjectGroup>().getObjects();
                    std::cout << "Found " << objects.size() << " objects in layer" << std::endl;
                    for (const auto &object: objects) {
                        std::cout << "Object " << object.getUID() << ", " << object.getName()
                                  << std::endl;
                        const auto &properties = object.getProperties();
                        std::cout << "Object has " << properties.size() << " properties"
                                  << std::endl;
                        for (const auto &prop: properties) {
                            std::cout << "Found property: " << prop.getName() << std::endl;
                            std::cout << "Type: " << int(prop.getType()) << std::endl;
                        }

                        if (!object.getTilesetName().empty()) {
                            std::cout << "Object uses template tile set " << object.getTilesetName()
                                      << "\n";
                        }
                    }
                }

                if (layer->getType() == tmx::Layer::Type::Tile) {
                    const auto &tiles = layer->getLayerAs<tmx::TileLayer>().getTiles();
                    if (tiles.empty()) {
                        const auto &chunks = layer->getLayerAs<tmx::TileLayer>().getChunks();
                        if (chunks.empty()) {
                            std::cout << "Layer has missing tile data\n";
                        } else {
                            std::cout << "Layer has " << chunks.size() << " tile chunks.\n";
                        }
                    } else {
                        std::cout << "Layer has " << tiles.size() << " tiles.\n";
                    }
                }

                const auto &properties = layer->getProperties();
                std::cout << properties.size() << " Layer Properties:" << std::endl;
                for (const auto &prop: properties) {
                    std::cout << "Found property: " << prop.getName() << std::endl;
                    std::cout << "Type: " << int(prop.getType()) << std::endl;
                }
            }
        } else {
            std::cout << "Failed loading map" << std::endl;
        }
    }

    void testLoadTilesetWithoutMap() {
        tmx::Tileset ts;
        if (!ts.loadWithoutMap("resources/tmx/images/tilemap/platform.tsx")) {
            std::cout << "Failed to load tileset" << std::endl;
            return;
        }

        std::cout << "Loaded tileset without map: " << ts.getName() << std::endl;
        std::cout << "Tileset class: " << ts.getClass() << std::endl;
        std::cout << "Tileset image: " << ts.getImagePath() << std::endl;
        std::cout << "Tileset first GID: " << ts.getFirstGID() << std::endl;
        if (ts.getColumnCount() > 0) {
            std::cout << "Tiles in tileset: " << ts.getTileCount() << std::endl;
        }
    }

    class TestFailure {};

    template<typename T1, typename T2>
    void checkEqImpl(const char *expr1, T1 &&v1, const char *expr2, T2 &&v2, const char *file,
                     size_t line) {
        if (v1 != v2) {
            std::cout << "FAIL: " << expr1 << " = " << expr2 << "\n  Left:  " << v1
                      << "\n  Right: " << v2 << "\nat " << file << ":" << line << std::endl;
            throw TestFailure();
        } else {
            std::cout << "OK: " << expr1 << " = " << expr2 << " = " << v1 << std::endl;
        }
    }

#define CHECK_EQ(a, b) checkEqImpl(#a, (a), #b, (b), __FILE__, __LINE__)

    struct RevertWindowsPathHandling {
        bool oldValue = tmx::enableWindowsPathHandling;
        ~RevertWindowsPathHandling() { tmx::enableWindowsPathHandling = oldValue; }
    };

    void testResolvingPaths() {
        RevertWindowsPathHandling revertWindowsPathHandling;
        tmx::enableWindowsPathHandling = true;

        CHECK_EQ(tmx::resolveFilePath("a/b/c", "A/B/C"), "A/B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c/", "A/B/C"), "A/B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "A/B/C/"), "A/B/C/a/b/c");

        CHECK_EQ(tmx::resolveFilePath("a/b/c", ""), "a/b/c");
        CHECK_EQ(tmx::resolveFilePath("", "A/B/C"), "A/B/C");

        CHECK_EQ(tmx::resolveFilePath("a///b//c", "A//B///C"), "A/B/C/a/b/c");

        CHECK_EQ(tmx::resolveFilePath("/a/b/c", "A/B/C"), "/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "/A/B/C"), "/A/B/C/a/b/c");

        CHECK_EQ(tmx::resolveFilePath("./a/b/c", "A/B/C"), "A/B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("../a/b/c", "A/B/C"), "A/B/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("../../a/b/c", "A/B/C"), "A/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("../../../a/b/c", "A/B/C"), "a/b/c");
        CHECK_EQ(tmx::resolveFilePath("../../../../a/b/c", "A/B/C"), "../a/b/c");
        CHECK_EQ(tmx::resolveFilePath("../../../../../a/b/c", "A/B/C"), "../../a/b/c");

        CHECK_EQ(tmx::resolveFilePath("a/b/c", "./A/B/C"), "A/B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "../A/B/C"), "../A/B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "../../A/B/C"), "../../A/B/C/a/b/c");

        CHECK_EQ(tmx::resolveFilePath("a/../b/c", "A/B/C"), "A/B/C/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "A/../B/C"), "B/C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/../b/../c", "A/B/C"), "A/B/C/c");
        CHECK_EQ(tmx::resolveFilePath("a/b/c", "A/../B/../C"), "C/a/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/../b/../c", "A/../B/../C"), "C/c");

        CHECK_EQ(tmx::resolveFilePath("a/b/c/../..", "A/B/C"), "A/B/C/a");
        CHECK_EQ(tmx::resolveFilePath("a/b/c/../../..", "A/B/C"), "A/B/C");
        CHECK_EQ(tmx::resolveFilePath("a/b/c/../../../..", "A/B/C"), "A/B");

        CHECK_EQ(tmx::resolveFilePath("/a/../b/c", "A/B/C"), "/b/c");
        CHECK_EQ(
                tmx::resolveFilePath("/a/../../b/c", "A/B/C"),
                "/../b/c"); // "/b/c" would also be valid but not worth handling this weird case IMO

        CHECK_EQ(tmx::resolveFilePath("C:/a/../b/c", "A/B/C"), "C:/b/c");
        CHECK_EQ(tmx::resolveFilePath("a/../b/c", "C:/A/B/C"), "C:/A/B/C/b/c");
        CHECK_EQ(tmx::resolveFilePath("C:/a/../b/c", "C:/A/B/C"), "C:/b/c");

        CHECK_EQ(tmx::resolveFilePath("a\\..\\b\\c", "C:\\A\\B\\..\\C"), "C:/A/C/b/c");
    }

} // namespace

int main() {
    testLoadMap();
    std::cout << std::endl << "------------------------------" << std::endl << std::endl;
    testLoadTilesetWithoutMap();
    std::cout << std::endl << "------------------------------" << std::endl << std::endl;
    testResolvingPaths();
    std::cout << std::endl << "------------------------------" << std::endl << std::endl;

#if defined(PAUSE_AT_END)
    std::cout << std::endl << "Press return to quit..." << std::endl;
    std::cin.get();
#endif

    std::cout << "Test complete" << std::endl;
    return 0;
}

/*
Loaded Map version: 1, 0
Map Dimensions: {0, 0, 12800, 2560}
Map class: TestMapClass
Map tileset has 2 tilesets
Tileset: platform
Tileset class: Level
Tileset first GID: 1
Tileset: tileset02
Tileset class: TestTilesetClass
Tileset first GID: 43
Map has 1 properties
Found property: test map property
Type: 1

Map has 3 layers
Found Layer: TestGroup
Layer Type: Group
Layer Dimensions: {0, 0}
Layer Tint: RGBA: 255, 255, 255, 255
Checking sublayers
LayerGroup has 3 layers
Found Layer: Far
Sub-layer Type: Tile
Sub-layer Class: TestLayerClass
Sub-layer Dimensions: {200, 40}
Sub-layer Tint: RGBA: 255, 255, 255, 255
Far has 8000 tiles
Found Layer: Middle
Sub-layer Type: Tile
Sub-layer Class:
Sub-layer Dimensions: {200, 40}
Sub-layer Tint: RGBA: 255, 255, 255, 255
Middle has 8000 tiles
Found Layer: Near
Sub-layer Type: Tile
Sub-layer Class:
Sub-layer Dimensions: {200, 40}
Sub-layer Tint: RGBA: 255, 255, 255, 255
Near has 8000 tiles
2 Layer Properties:
Found property: layer group prop
Type: 3
Found property: another property
Type: 0
Found Layer: Objects
Layer Type: Object
Layer Dimensions: {0, 0}
Layer Tint: RGBA: 255, 255, 255, 255
Found 30 objects in layer
Object 1, Buns
Object has 0 properties
Object 2, Flaps
Object has 1 properties
Found property: Custom Object Property
Type: 0
Object 3, Dicketry
Object has 0 properties
Object 4,
Object has 0 properties
Object 6,
Object has 0 properties
Object 8,
Object has 0 properties
Object 19,
Object has 0 properties
Object 23,
Object has 0 properties
Object 24,
Object has 0 properties
Object 25,
Object has 0 properties
Object 26,
Object has 0 properties
Object 27,
Object has 0 properties
Object 28,
Object has 0 properties
Object 29,
Object has 0 properties
Object 30,
Object has 0 properties
Object 31,
Object has 0 properties
Object 32,
Object has 0 properties
Object 33,
Object has 0 properties
Object 34,
Object has 0 properties
Object 35,
Object has 0 properties
Object 36,
Object has 0 properties
Object 37,
Object has 0 properties
Object 38,
Object has 0 properties
Object 39,
Object has 0 properties
Object 40,
Object has 0 properties
Object 41,
Object has 0 properties
Object 42,
Object has 0 properties
Object 43,
Object has 0 properties
Object 44,
Object has 0 properties
Object 45,
Object has 0 properties
0 Layer Properties:
Found Layer: Test Image Layer
Layer Type: Image
Layer Dimensions: {0, 0}
Layer Tint: RGBA: 255, 255, 255, 255
1 Layer Properties:
Found property: image layer property
Type: 3

------------------------------

Loaded tileset without map: platform
Tileset class: Level
Tileset image: resources/tmx/images/tilemap/tileset.png
Tileset first GID: 0
Tiles in tileset: 42

------------------------------

OK: tmx::resolveFilePath("a/b/c", "A/B/C") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("a/b/c/", "A/B/C") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("a/b/c", "A/B/C/") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("a/b/c", "") = "a/b/c" = a/b/c
OK: tmx::resolveFilePath("", "A/B/C") = "A/B/C" = A/B/C
OK: tmx::resolveFilePath("a///b//c", "A//B///C") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("/a/b/c", "A/B/C") = "/a/b/c" = /a/b/c
OK: tmx::resolveFilePath("a/b/c", "/A/B/C") = "/A/B/C/a/b/c" = /A/B/C/a/b/c
OK: tmx::resolveFilePath("./a/b/c", "A/B/C") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("../a/b/c", "A/B/C") = "A/B/a/b/c" = A/B/a/b/c
OK: tmx::resolveFilePath("../../a/b/c", "A/B/C") = "A/a/b/c" = A/a/b/c
OK: tmx::resolveFilePath("../../../a/b/c", "A/B/C") = "a/b/c" = a/b/c
OK: tmx::resolveFilePath("../../../../a/b/c", "A/B/C") = "../a/b/c" = ../a/b/c
OK: tmx::resolveFilePath("../../../../../a/b/c", "A/B/C") = "../../a/b/c" = ../../a/b/c
OK: tmx::resolveFilePath("a/b/c", "./A/B/C") = "A/B/C/a/b/c" = A/B/C/a/b/c
OK: tmx::resolveFilePath("a/b/c", "../A/B/C") = "../A/B/C/a/b/c" = ../A/B/C/a/b/c
OK: tmx::resolveFilePath("a/b/c", "../../A/B/C") = "../../A/B/C/a/b/c" = ../../A/B/C/a/b/c
OK: tmx::resolveFilePath("a/../b/c", "A/B/C") = "A/B/C/b/c" = A/B/C/b/c
OK: tmx::resolveFilePath("a/b/c", "A/../B/C") = "B/C/a/b/c" = B/C/a/b/c
OK: tmx::resolveFilePath("a/../b/../c", "A/B/C") = "A/B/C/c" = A/B/C/c
OK: tmx::resolveFilePath("a/b/c", "A/../B/../C") = "C/a/b/c" = C/a/b/c
OK: tmx::resolveFilePath("a/../b/../c", "A/../B/../C") = "C/c" = C/c
OK: tmx::resolveFilePath("a/b/c/../..", "A/B/C") = "A/B/C/a" = A/B/C/a
OK: tmx::resolveFilePath("a/b/c/../../..", "A/B/C") = "A/B/C" = A/B/C
OK: tmx::resolveFilePath("a/b/c/../../../..", "A/B/C") = "A/B" = A/B
OK: tmx::resolveFilePath("/a/../b/c", "A/B/C") = "/b/c" = /b/c
OK: tmx::resolveFilePath("/a/../../b/c", "A/B/C") = "/../b/c" = /../b/c
OK: tmx::resolveFilePath("C:/a/../b/c", "A/B/C") = "C:/b/c" = C:/b/c
OK: tmx::resolveFilePath("a/../b/c", "C:/A/B/C") = "C:/A/B/C/b/c" = C:/A/B/C/b/c
OK: tmx::resolveFilePath("C:/a/../b/c", "C:/A/B/C") = "C:/b/c" = C:/b/c
OK: tmx::resolveFilePath("a\\..\\b\\c", "C:\\A\\B\\..\\C") = "C:/A/C/b/c" = C:/A/C/b/c

------------------------------

Test complete
Press any key to continue . . .
*/
