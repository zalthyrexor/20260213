#ifndef CHUNK_MANAGER_H
#define CHUNK_MANAGER_H

#include "raylib.h"
#include <map>
#include <vector>
#include <cmath>
#include "ChunkMeshBuilder.h"

#define STB_PERLIN_IMPLEMENTATION
#include "stb_perlin.h"

struct ChunkPos
{
    int x, y, z;
    bool operator<(const ChunkPos& other) const
    {
        if (x != other.x) return x < other.x;
        if (y != other.y) return y < other.y;
        return z < other.z;
    }
};
struct Chunk
{
    unsigned char voxels[CHUNK_SIZE][CHUNK_SIZE][CHUNK_SIZE];
    Model model;
    Vector3 position;
    bool isModified = true;
    void GenerateData(int cx, int cy, int cz)
    {
        position = {(float)cx * CHUNK_SIZE, (float)cy * CHUNK_SIZE, (float)cz * CHUNK_SIZE};
        for (int x = 0; x < CHUNK_SIZE; x++)
        {
            for (int z = 0; z < CHUNK_SIZE; z++)
            {
                float worldX = (float)(cx * CHUNK_SIZE + x);
                float worldZ = (float)(cz * CHUNK_SIZE + z);
                float noise = stb_perlin_noise3(worldX * 0.03f, 0.0f, worldZ * 0.03f, 0, 0, 0);
                int worldHeight = (int)(8 + noise * 10);
                for (int y = 0; y < CHUNK_SIZE; y++)
                {
                    float worldY = (float)(cy * CHUNK_SIZE + y);
                    voxels[x][y][z] = (worldY < worldHeight) ? 1 : 0;
                }
            }
        }
    }
};
class ChunkManager
{
private:
    std::map<ChunkPos, Chunk*> chunks;
    Texture2D worldTexture;
    void BuildChunkMesh(int cx, int cy, int cz)
    {
        auto it = chunks.find({cx, cy, cz});
        if (it == chunks.end()) return;
        unsigned char neighborData[CHUNK_SIZE + 2][CHUNK_SIZE + 2][CHUNK_SIZE + 2] = {0};
        for (int x = -1; x <= CHUNK_SIZE; x++)
        {
            for (int y = -1; y <= CHUNK_SIZE; y++)
            {
                for (int z = -1; z <= CHUNK_SIZE; z++)
                {
                    if (IsBlockAt(
                        (float)(cx * CHUNK_SIZE + x) + 0.5f,
                        (float)(cy * CHUNK_SIZE + y) + 0.5f,
                        (float)(cz * CHUNK_SIZE + z) + 0.5f))
                    {
                        neighborData[x + 1][y + 1][z + 1] = 1;
                    }
                }
            }
        }
        if (it->second->model.meshCount > 0) UnloadModel(it->second->model);
        Mesh mesh = ChunkMeshBuilder::GenerateMesh(neighborData);
        it->second->model = LoadModelFromMesh(mesh);
        it->second->model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = worldTexture;
    }
public:
    ~ChunkManager()
    {
        UnloadTexture(worldTexture);
    }
    void InitWorld(int width, int height, int depth)
    {
        worldTexture = LoadTexture("resources/my_texture.png");
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                for (int z = 0; z < depth; z++)
                {
                    Chunk* c = new Chunk();
                    c->GenerateData(x, y, z);
                    chunks[{x, y, z}] = c;
                }
            }
        }
        for (int x = 0; x < width; x++)
        {
            for (int y = 0; y < height; y++)
            {
                for (int z = 0; z < depth; z++)
                {
                    BuildChunkMesh(x, y, z);
                }
            }
        }
    }
    void DrawWorld(Shader shadowShader = {0})
    {
        for (auto const& [coords, c] : chunks)
        {
            if (shadowShader.id != 0)
            {
                c->model.materials[0].shader = shadowShader;
            }
            DrawModel(c->model, c->position, 1.0f, WHITE);
        }
    }
    bool IsBlockAt(float wx, float wy, float wz)
    {
        int cx = (int)floor(wx / CHUNK_SIZE);
        int cy = (int)floor(wy / CHUNK_SIZE);
        int cz = (int)floor(wz / CHUNK_SIZE);
        auto it = chunks.find({cx, cy, cz});
        if (it != chunks.end())
        {
            int lx = (int)floor(wx) - (cx * CHUNK_SIZE);
            int ly = (int)floor(wy) - (cy * CHUNK_SIZE);
            int lz = (int)floor(wz) - (cz * CHUNK_SIZE);
            if (lx >= 0 && lx < CHUNK_SIZE &&
                ly >= 0 && ly < CHUNK_SIZE &&
                lz >= 0 && lz < CHUNK_SIZE)
            {
                return it->second->voxels[lx][ly][lz] != 0;
            }
        }
        return false;
    }
};

#endif