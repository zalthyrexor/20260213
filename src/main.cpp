#include "raylib.h"
#include "raymath.h"
#include "ChunkManager.h"
#include "entt/entt.hpp"
#include "rlgl.h" 
#include <vector>
#include <cmath>
#include <map>

struct PlayerTag {};
struct PlayerConfig
{
    float moveSpeed = 5.0f;
    float responsiveness = 15.0f;
    float jumpForce = 8.0f;
    float gravity = -25.0f;
    float mouseSensitivity = 0.002f;
};
struct PlayerRotation
{
    float pitch = 0.0f;
    float yaw = 0.0f;
};
struct AABB
{
    Vector3 min;
    Vector3 max;
};
struct KinematicState
{
    Vector3 position;
    Vector3 velocity;
    bool grounded;
};
static AABB GetAbsoluteBoundingBox(Vector3 pos, AABB aabb)
{
    return AABB {Vector3Add(pos, aabb.min), Vector3Add(pos, aabb.max)};
}
static void UpdatePlayerRotationSystem(entt::registry& registry)
{
    auto view = registry.view<PlayerRotation, PlayerConfig, PlayerTag>();
    view.each([](auto& rot, const auto& config)
        {
            Vector2 mouseDelta = GetMouseDelta();
            rot.yaw -= mouseDelta.x * config.mouseSensitivity;
            rot.pitch += mouseDelta.y * config.mouseSensitivity;
            rot.pitch = Clamp(rot.pitch, -1.5f, 1.5f);
        });
}
static void UpdatePlayerVelocitySystem(entt::registry& registry, float dt)
{
    auto view = registry.view<KinematicState, PlayerRotation, PlayerConfig, PlayerTag>();
    view.each([dt](auto& state, const auto& rot, const auto& config)
        {
            Vector3 localInput = {0.0f, 0.0f, 0.0f};
            if (IsKeyDown(KEY_W)) localInput.z += 1.0f;
            if (IsKeyDown(KEY_S)) localInput.z -= 1.0f;
            if (IsKeyDown(KEY_D)) localInput.x -= 1.0f;
            if (IsKeyDown(KEY_A)) localInput.x += 1.0f;
            Vector3 targetVelocity = {0, 0, 0};
            if (Vector3Length(localInput) > 0.0f)
            {
                localInput = Vector3Normalize(localInput);
                Quaternion horizontalRot = QuaternionFromEuler(0.0f, rot.yaw, 0.0f);
                Vector3 worldMove = Vector3RotateByQuaternion(localInput, horizontalRot);
                targetVelocity = Vector3Scale(worldMove, config.moveSpeed);
            }
            Vector3 diff = Vector3Subtract(targetVelocity, {state.velocity.x, 0, state.velocity.z});
            Vector3 velocityDelta = Vector3Scale({diff.x * config.responsiveness,    config.gravity,   diff.z * config.responsiveness}, dt);
            if (IsKeyDown(KEY_SPACE) && state.grounded)
            {
                velocityDelta.y += config.jumpForce;
            }
            state.velocity = Vector3Add(state.velocity, velocityDelta);
        });
}
static void UpdatePositionSystem(entt::registry& registry, float dt, ChunkManager& chunkManager)
{
    auto view = registry.view<KinematicState, AABB>();
    view.each([&](auto entity, auto& state, const auto& localAABB)
        {
            Vector3 movement = Vector3Scale(state.velocity, dt);
            AABB currentWorldAABB = GetAbsoluteBoundingBox(state.position, localAABB);
            AABB nextWorldAABB = GetAbsoluteBoundingBox(Vector3Add(state.position, movement), localAABB);
            AABB area = AABB {Vector3Min(currentWorldAABB.min, nextWorldAABB.min), Vector3Max(currentWorldAABB.max, nextWorldAABB.max)};
            std::vector<AABB> worldAABBs;
            const float eps = 0.01f;
            int minX = static_cast<int>(floorf(area.min.x - eps));
            int minY = static_cast<int>(floorf(area.min.y - eps));
            int minZ = static_cast<int>(floorf(area.min.z - eps));
            int maxX = static_cast<int>(ceilf(area.max.x + eps));
            int maxY = static_cast<int>(ceilf(area.max.y + eps));
            int maxZ = static_cast<int>(ceilf(area.max.z + eps));
            for (int x = minX; x < maxX; x++)
            {
                for (int y = minY; y < maxY; y++)
                {
                    for (int z = minZ; z < maxZ; z++)
                    {
                        if (chunkManager.IsBlockAt(
                            (float)x + 0.5f,
                            (float)y + 0.5f,
                            (float)z + 0.5f))
                        {
                            AABB blockAABB {};
                            blockAABB.min = {(float)x, (float)y, (float)z};
                            blockAABB.max = {(float)x + 1.0f, (float)y + 1.0f, (float)z + 1.0f};
                            worldAABBs.push_back(blockAABB);
                        }
                    }
                }
            }
            Vector3 originalDelta = movement;
            float* dArray = (float*)&movement;
            int axes[3] = {1, 0, 2};
            for (int axis : axes)
            {
                int a1 = (axis + 1) % 3;
                int a2 = (axis + 2) % 3;
                for (const auto& col : worldAABBs)
                {
                    const float* entMin = (float*)&currentWorldAABB.min;
                    const float* entMax = (float*)&currentWorldAABB.max;
                    const float* othMin = (float*)&col.min;
                    const float* othMax = (float*)&col.max;
                    if (entMin[a1] < othMax[a1] && entMax[a1] > othMin[a1] &&
                        entMin[a2] < othMax[a2] && entMax[a2] > othMin[a2])
                    {
                        if (dArray[axis] > 0 && othMin[axis] >= entMax[axis])
                        {
                            float diff = othMin[axis] - entMax[axis];
                            if (diff < dArray[axis]) dArray[axis] = diff;
                        }
                        else if (dArray[axis] < 0 && othMax[axis] <= entMin[axis])
                        {
                            float diff = othMax[axis] - entMin[axis];
                            if (diff > dArray[axis]) dArray[axis] = diff;
                        }
                    }
                }
            }
            state.position = Vector3Add(state.position, movement);
            state.grounded = (originalDelta.y < 0 && movement.y > originalDelta.y);
            if (movement.x != originalDelta.x) state.velocity.x = 0;
            if (movement.y != originalDelta.y) state.velocity.y = 0;
            if (movement.z != originalDelta.z) state.velocity.z = 0;
        });
}
int main()
{
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1500, 900, "Voxel Sandbox - Debug");
    MaximizeWindow();
    ChunkManager chunkManager;
    chunkManager.InitWorld(16, 1, 16);
    Camera camera = {
        .position = {0, 0, 0},
        .target = {0, 0, 1},
        .up = {0.0f, 1.0f, 0.0f},
        .fovy = 60.0f,
        .projection = CAMERA_PERSPECTIVE
    };
    DisableCursor();
    SetTargetFPS(150);
    entt::registry registry;
    auto player = registry.create();
    registry.emplace<PlayerTag>(player);
    registry.emplace<KinematicState>(player, Vector3 {1.0f, 24.0f, 1.0f}, Vector3 {0, 0, 0}, false);
    registry.emplace<PlayerRotation>(player, 0.0f, 0.0f);
    registry.emplace<PlayerConfig>(player);
    registry.emplace<AABB>(player, Vector3 {0.0f, 0.0f, 0.0f}, Vector3 {0.6f, 1.8f, 0.6f});
    Shader shadowShader = LoadShader("resources/shadow.vs", "resources/shadow.fs");
    int shadowMapWidth = 2048;
    int shadowMapHeight = 2048;
    RenderTexture2D shadowMap = LoadRenderTexture(shadowMapWidth, shadowMapHeight);
    Vector3 lightPos = {0, 150.0f, 0};
    Camera3D lightCam = {0};
    lightCam.position = {0, 150.0f, 0};
    lightCam.target = {64.0f, 0.0f, 64.0f};
    lightCam.up = {0.0f, 1.0f, 0.0f};
    lightCam.projection = CAMERA_ORTHOGRAPHIC;
    lightCam.fovy = 200.0f;
    int lightMatLoc = GetShaderLocation(shadowShader, "matLight");
    int lightPosLoc = GetShaderLocation(shadowShader, "lightPos");
    int shadowMapLoc = GetShaderLocation(shadowShader, "shadowMap");
    int lightColLoc = GetShaderLocation(shadowShader, "lightColor");
    Vector3 lightColor = {0.8f, 0.8f, 0.8f};
    while (!WindowShouldClose())
    {
        float dt = GetFrameTime();
        if (IsKeyPressed(KEY_R))
        {
            registry.get<KinematicState>(player).position = {1, 32, 1};
        }
        UpdatePlayerRotationSystem(registry);
        UpdatePlayerVelocitySystem(registry, dt);
        UpdatePositionSystem(registry, dt, chunkManager);
        auto& pState = registry.get<KinematicState>(player);
        auto& pRot = registry.get<PlayerRotation>(player);
        camera.position = Vector3Add(pState.position, {0.3f, 1.6f, 0.3f});
        camera.target = Vector3Add(camera.position, Vector3RotateByQuaternion({0, 0, 1}, QuaternionFromEuler(pRot.pitch, pRot.yaw, 0.0f)));
        BeginTextureMode(shadowMap);
        ClearBackground(WHITE);
        BeginMode3D(lightCam);
        Matrix lightView = rlGetMatrixModelview();
        Matrix lightProj = rlGetMatrixProjection();
        Matrix matLight = MatrixMultiply(lightView, lightProj);
        chunkManager.DrawWorld();
        EndMode3D();
        EndTextureMode();
        BeginDrawing();
        ClearBackground(SKYBLUE);
        SetShaderValueMatrix(shadowShader, lightMatLoc, matLight);
        SetShaderValue(shadowShader, lightPosLoc, &lightPos, SHADER_UNIFORM_VEC3);
        SetShaderValue(shadowShader, lightColLoc, &lightColor, SHADER_UNIFORM_VEC3);
        rlActiveTextureSlot(1);
        rlEnableTexture(shadowMap.depth.id);
        SetShaderValue(shadowShader, shadowMapLoc, (int[1])  1, SHADER_UNIFORM_INT);
        BeginMode3D(camera);
        chunkManager.DrawWorld(shadowShader);
        EndMode3D();
        auto& pPos = registry.get<KinematicState>(player).position;
        const char* coordsText = TextFormat("X: %.2f\nY: %.2f\nZ: %.2f", pPos.x, pPos.y, pPos.z);
        int fontSize = 20;
        int textWidth = MeasureText("X: 0000.00", fontSize);
        int padding = 20;
        DrawRectangle(GetScreenWidth() - textWidth - padding, padding - 5, textWidth + 10, 75, ColorAlpha(BLACK, 0.3f));
        DrawText(coordsText, GetScreenWidth() - textWidth - padding + 5, padding, fontSize, WHITE);
        DrawFPS(10, 10);
        EndDrawing();
    }
    UnloadShader(shadowShader);
    UnloadRenderTexture(shadowMap);
    CloseWindow();
    return 0;
}
