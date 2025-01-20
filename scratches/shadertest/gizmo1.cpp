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
// Example 01 - Getting Started
// Demonstrates how to use a simple, globally oriented, 3D translation gizmo in a raylib scene.
//--------------------------------------------------------------------------------------------------

#include "raylib.h"
#include "raygizmo.h"
#include "raymath.h"

//--------------------------------------------------------------------------------------------------
// Constants
//--------------------------------------------------------------------------------------------------

const char* EXAMPLE_TITLE = "Example 01 - Getting Started";

const int SCREEN_WIDTH = 960;
const int SCREEN_HEIGHT = 540;

//--------------------------------------------------------------------------------------------------
// Main Entry Point
//--------------------------------------------------------------------------------------------------

int main(int argc, char** argv)
{
    // This Transform stores the translation, rotation, and scaling of our crate.
    // It will be updated dynamically by the gizmo during the program.
    Transform crateTransform = GizmoIdentity();

    // Setup: Initialize the window and basic configurations.
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_WINDOW_RESIZABLE);
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, TextFormat("raylib-gizmo | %s", EXAMPLE_TITLE));
    SetTargetFPS(60);

    // Load the crate texture.
    Texture crateTexture = LoadTexture("resources/textures/crate_texture.jpg");
    GenTextureMipmaps(&crateTexture);
    SetTextureFilter(crateTexture, TEXTURE_FILTER_TRILINEAR);

    // Load the crate model and apply the texture.
    Model crateModel = LoadModel("resources/models/crate_model.obj");
    crateModel.materials[0].maps[MATERIAL_MAP_ALBEDO].texture = crateTexture;

    // Setup the 3D camera.
    Camera cam = { 0 };
    cam.fovy = 45.0f;
    cam.position = Vector3{ 7.5f, 5.5f, 5.0f };
    cam.target = Vector3{ 0, 1.5f, 0 };
    cam.up = Vector3{ 0, 1, 0 };
    cam.projection = CAMERA_PERSPECTIVE;

    // Main game loop.
    while (!WindowShouldClose())
    {
        BeginDrawing();

        // Clear the background with a dark blue color.
        ClearBackground(Color { 0, 0, 25, 255 });

        BeginMode3D(cam);

        // Update the crate's transform matrix from the gizmo.
        crateModel.transform = GizmoToMatrix(crateTransform);

        // Draw the crate model using the updated transform.
        DrawModel(crateModel, Vector3Zero(), 1.0f, WHITE);

        // Draw the translation gizmo and handle its input.
        // This will directly update the crateTransform variable.
        DrawGizmo3D(GIZMO_TRANSLATE, &crateTransform);

        EndMode3D();

        EndDrawing();
    }

    // Unload resources and clean up.
    UnloadTexture(crateTexture);
    UnloadModel(crateModel);
    CloseWindow();

    return 0;
}