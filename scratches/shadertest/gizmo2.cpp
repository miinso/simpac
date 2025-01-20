/***************************************************************************************************
*
*   LICENSE: zlib
*
*   Copyright (c) 2024 Claudio Z. (@cloudofoz)
*
*   This software is provided "as-is," without any express or implied warranty. In no event
*   will the authors be held liable for any damages arising from the use of this software.
*
*   Permission is granted to anyone to use this software for any purpose, including commercial
*   applications, and to alter and redistribute it freely, subject to the following restrictions:
*
*     1. The origin of this software must not be misrepresented; you must not claim that you
*     wrote the original software. If you use this software in a product, an acknowledgment
*     in the product documentation would be appreciated but is not required.
*
*     2. Altered source versions must be plainly marked as such and must not be misrepresented
*     as being the original software.
*
*     3. This notice may not be removed or altered from any source distribution.
*
***************************************************************************************************/

//--------------------------------------------------------------------------------------------------
// Example 02 - Gizmo Types
// Demonstrates multiple gizmo modes (translate, rotate, scale, and all combined) with fixed
// configurations.
//--------------------------------------------------------------------------------------------------

#include "raygizmo.h"
#include "raylib.h"
#include "raymath.h"


//--------------------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------------------

const char* EXAMPLE_TITLE = "Example 02 - Gizmo Types";

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 540;

enum { CRATE_COUNT = 4 };

//--------------------------------------------------------------------------------------------------
// Main Entry Point
//--------------------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    // These Transforms store the translation, rotation, and scaling of the crates
    // They will be dynamically updated by the gizmos during the program
    Transform crateTransforms[CRATE_COUNT];

    // Initialize the transforms with default values
    for (int i = 0; i < CRATE_COUNT; ++i) {
        crateTransforms[i] = GizmoIdentity();
        crateTransforms[i].translation.x =
            -12.0f + 6.0f * (float)i; // Offset crates along the X-axis
    }

    // Assign each crate a different gizmo type
    const int gizmoTypes[CRATE_COUNT] = {GIZMO_TRANSLATE, GIZMO_SCALE, GIZMO_ROTATE, GIZMO_ALL};

    // Setup: Initialize the window
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TextFormat("raylib-gizmo | %s", EXAMPLE_TITLE));
    SetTargetFPS(60);

    // Load the crate texture
    Texture crateTexture = LoadTexture("resources/textures/crate_texture.jpg");
    GenTextureMipmaps(&crateTexture);
    SetTextureFilter(crateTexture, TEXTURE_FILTER_TRILINEAR);

    // Load the crate model and apply the texture
    Model crateModel = LoadModel("resources/models/crate_model.obj");
    crateModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = crateTexture;

    // Setup the 3D camera
    Camera cam = {0};
    cam.fovy = 45.0f;
    cam.position = Vector3{-5.5f, 10.5f, 14.0f};
    cam.target = Vector3{-2.5f, 0, 0};
    cam.up = Vector3{0, 1, 0};
    cam.projection = CAMERA_PERSPECTIVE;

    // Main loop
    while (!WindowShouldClose()) {
        BeginDrawing();

        // Clear the background with a dark blue color
        ClearBackground(Color{0, 0, 25, 255});

        BeginMode3D(cam);

        // Draw the crates with their updated transforms
        for (int i = 0; i < CRATE_COUNT; ++i) {
            crateModel.transform = GizmoToMatrix(crateTransforms[i]);
            DrawModel(crateModel, Vector3Zero(), 1.0f, WHITE);
        }

        // Draw the gizmos and handle user input
        for (int i = 0; i < CRATE_COUNT; ++i) {
            DrawGizmo3D(gizmoTypes[i], &crateTransforms[i]);
        }

        EndMode3D();

        EndDrawing();
    }

    // Unload resources and clean up
    UnloadTexture(crateTexture);
    UnloadModel(crateModel);
    CloseWindow();

    return 0;
}