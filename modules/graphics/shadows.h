#pragma once

#include <raylib.h>
#include <raymath.h>
#include <cmath>

#include "rlgl.h"
#include "lighting.h"

namespace graphics {

// shadow map state for directional + spot lights
struct ShadowRenderer {
    unsigned int fbo = 0;
    unsigned int depth_tex = 0;
    int resolution = 2048;
    float bias = 0.005f;

    Shader depth_shader = {};
    int depth_mvp_loc = -1;

    // locs on the PBR shader
    int shadow_map_loc = -1;
    int light_vp_loc = -1;
    int shadow_bias_loc = -1;

    // computed each frame
    Matrix light_vp = {};

    // spot shadow fields
    unsigned int spot_fbo = 0;
    unsigned int spot_depth_tex = 0;
    int spot_resolution = 1024;
    float spot_bias = 0.005f;

    int spot_shadow_map_loc = -1;
    int spot_light_vp_loc = -1;
    int spot_shadow_bias_loc = -1;

    Matrix spot_light_vp = {};

    // TODO: shadow frustum auto-fit — track scene AABB or explicit center instead of hardcoded origin

    // begin directional shadow depth pass — sets up FBO, viewport, matrices for light POV
    void begin(const DirectionalLight& dl, float radius) {
        if (!fbo) return;

        Vector3 center = {0, 0, 0};
        Vector3 dir = Vector3Normalize({dl.direction.x, dl.direction.y, dl.direction.z});
        Vector3 light_pos = Vector3Subtract(center, Vector3Scale(dir, radius));

        Matrix view = MatrixLookAt(light_pos, center, {0, 1, 0});
        Matrix proj = MatrixOrtho(-radius, radius, -radius, radius, 0.1f, radius * 2.0f);
        light_vp = MatrixMultiply(view, proj);

        rlDrawRenderBatchActive();
        rlEnableFramebuffer(fbo);
        rlViewport(0, 0, resolution, resolution);
        rlClearScreenBuffers();

        rlMatrixMode(RL_PROJECTION);
        rlPushMatrix();
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloatV(proj).v);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloatV(view).v);

        rlEnableDepthTest();
    }

    // end directional shadow depth pass — bind shadow tex + uniforms to PBR shader
    void end(const LightRenderer& lr) {
        if (!fbo) return;

        rlDrawRenderBatchActive();

        rlMatrixMode(RL_PROJECTION);
        rlPopMatrix();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlDisableDepthTest();

        rlDisableFramebuffer();
        rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());

        // bind shadow depth texture to PBR shader (slot 7)
        int slot = 7;
        rlActiveTextureSlot(slot);
        rlEnableTexture(depth_tex);
        SetShaderValue(lr.shader, shadow_map_loc, &slot, SHADER_UNIFORM_INT);
        SetShaderValueMatrix(lr.shader, light_vp_loc, light_vp);
        SetShaderValue(lr.shader, shadow_bias_loc, &bias, SHADER_UNIFORM_FLOAT);
    }

    // begin spot shadow depth pass — perspective projection from spot light
    void begin_spot(const SpotLight& sl, const Position& pos, float near) {
        if (!spot_fbo) return;

        Vector3 p = {pos.x, pos.y, pos.z};
        Vector3 dir = Vector3Normalize({sl.direction.x, sl.direction.y, sl.direction.z});
        Vector3 target = Vector3Add(p, dir);

        // pick up vector not parallel to direction
        Vector3 up = {0, 1, 0};
        if (fabsf(dir.y) > 0.99f) up = {0, 0, 1};

        Matrix view = MatrixLookAt(p, target, up);
        float fov = 2.0f * sl.outer_angle * DEG2RAD;
        Matrix proj = MatrixPerspective(fov, 1.0f, near, sl.range);
        spot_light_vp = MatrixMultiply(view, proj);

        rlDrawRenderBatchActive();
        rlEnableFramebuffer(spot_fbo);
        rlViewport(0, 0, spot_resolution, spot_resolution);
        rlClearScreenBuffers();

        rlMatrixMode(RL_PROJECTION);
        rlPushMatrix();
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloatV(proj).v);

        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();
        rlMultMatrixf(MatrixToFloatV(view).v);

        rlEnableDepthTest();
    }

    // end spot shadow depth pass — bind spot tex + uniforms
    void end_spot(const LightRenderer& lr) {
        if (!spot_fbo) return;

        rlDrawRenderBatchActive();

        rlMatrixMode(RL_PROJECTION);
        rlPopMatrix();
        rlMatrixMode(RL_MODELVIEW);
        rlLoadIdentity();

        rlDisableDepthTest();

        rlDisableFramebuffer();
        rlViewport(0, 0, GetScreenWidth(), GetScreenHeight());

        // bind spot depth texture to PBR shader (slot 8)
        int slot = 8;
        rlActiveTextureSlot(slot);
        rlEnableTexture(spot_depth_tex);
        SetShaderValue(lr.shader, spot_shadow_map_loc, &slot, SHADER_UNIFORM_INT);
        SetShaderValueMatrix(lr.shader, spot_light_vp_loc, spot_light_vp);
        SetShaderValue(lr.shader, spot_shadow_bias_loc, &spot_bias, SHADER_UNIFORM_FLOAT);
    }

    // draw a model into the active shadow map (swaps material shader to depth)
    void draw(Model& model, Vector3 pos, float scale, Color tint) const {
        if (!depth_shader.id) return;
        Shader saved[8];
        int n = model.materialCount < 8 ? model.materialCount : 8;
        for (int i = 0; i < n; i++) {
            saved[i] = model.materials[i].shader;
            model.materials[i].shader = depth_shader;
        }
        DrawModel(model, pos, scale, tint);
        for (int i = 0; i < n; i++)
            model.materials[i].shader = saved[i];
    }

    // draw shadow map as 2D overlay — call after EndMode3D (PostRender phase)
    static void debug_overlay(unsigned int dtex, int tex_res, int x, int y, int size, const char* label) {
        Texture2D tex = {dtex, tex_res, tex_res, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE};
        Rectangle src = {0, 0, (float)tex_res, -(float)tex_res}; // neg height = flip FBO
        Rectangle dst = {(float)x, (float)y, (float)size, (float)size};
        DrawTexturePro(tex, src, dst, {0, 0}, 0, WHITE);
        DrawText(label, x + 4, y + size - 18, 14, YELLOW);
    }

    // draw light frustum wireframe — call inside BeginMode3D (OnRenderOverlay phase)
    static void debug_frustum(Matrix vp, Color color) {
        // invert light VP to go from NDC → world
        Matrix inv = MatrixInvert(vp);

        // 8 NDC corners of the clip box
        Vector3 ndc[8] = {
            {-1,-1,-1}, { 1,-1,-1}, { 1, 1,-1}, {-1, 1,-1},
            {-1,-1, 1}, { 1,-1, 1}, { 1, 1, 1}, {-1, 1, 1},
        };
        Vector3 w[8];
        for (int i = 0; i < 8; i++)
            w[i] = Vector3Transform(ndc[i], inv);

        // near face
        DrawLine3D(w[0], w[1], color);
        DrawLine3D(w[1], w[2], color);
        DrawLine3D(w[2], w[3], color);
        DrawLine3D(w[3], w[0], color);
        // far face
        DrawLine3D(w[4], w[5], color);
        DrawLine3D(w[5], w[6], color);
        DrawLine3D(w[6], w[7], color);
        DrawLine3D(w[7], w[4], color);
        // connecting edges
        DrawLine3D(w[0], w[4], color);
        DrawLine3D(w[1], w[5], color);
        DrawLine3D(w[2], w[6], color);
        DrawLine3D(w[3], w[7], color);
    }
};

} // namespace graphics
