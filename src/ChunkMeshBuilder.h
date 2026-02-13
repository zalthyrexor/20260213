#ifndef CHUNK_MESH_BUILDER_HPP
#define CHUNK_MESH_BUILDER_HPP

#include <vector>
#include <cstring>
#include "raylib.h"
#include "raymath.h"

const int CHUNK_SIZE = 16;

namespace VoxelData
{
    static const Vector3 CubeVertices[8] = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    static const int FaceVertexIndices[6][4] = {{3, 7, 2, 6}, {1, 5, 0, 4}, {1, 2, 5, 6}, {4, 7, 0, 3}, {5, 6, 4, 7}, {0, 3, 1, 2}};
    static const Vector2 FaceUVs[4] = {{0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}};
    static const Vector3 FaceChecks[6] = {{0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    static const Vector3 FaceNormals[6] = {{0, 1, 0}, {0, -1, 0}, {1, 0, 0}, {-1, 0, 0}, {0, 0, 1}, {0, 0, -1}};
    static Vector3 CachedAOOffsets[6][4][3];
    static bool isAOCached = false;
    static void PrecomputeAO()
    {
        if (isAOCached) return;
        for (int f = 0; f < 6; ++f)
        {
            for (int v = 0; v < 4; ++v)
            {
                Vector3 vPosLocal = CubeVertices[FaceVertexIndices[f][v]];
                Vector3 vDir = {
                    (vPosLocal.x - 0.5f) * 2.0f,
                    (vPosLocal.y - 0.5f) * 2.0f,
                    (vPosLocal.z - 0.5f) * 2.0f
                };
                Vector3 s1 = {0, 0, 0};
                Vector3 s2 = {0, 0, 0};
                if (FaceChecks[f].x != 0) 
                { 
                    s1.y = vDir.y;
                    s2.z = vDir.z;
                }
                else if (FaceChecks[f].y != 0) { 
                    s1.x = vDir.x;
                    s2.z = vDir.z;
                }
                else {
                    s1.x = vDir.x;
                    s2.y = vDir.y;
                }
                CachedAOOffsets[f][v][0] = s1;
                CachedAOOffsets[f][v][1] = s2;
                CachedAOOffsets[f][v][2] = {s1.x + s2.x, s1.y + s2.y, s1.z + s2.z};
            }
        }
        isAOCached = true;
    }
}
class ChunkMeshBuilder
{
public:
    static Mesh GenerateMesh(unsigned char voxels[CHUNK_SIZE + 2][CHUNK_SIZE + 2][CHUNK_SIZE + 2])
    {
        VoxelData::PrecomputeAO();
        std::vector<float> vertices;
        std::vector<float> texcoords;
        std::vector<float> normals;
        std::vector<unsigned short> indices;
        std::vector<unsigned char> colors;
        int vertexCount = 0;
        auto getV = [&](int x, int y, int z) -> bool
            {
                if (x < 0 || x >= CHUNK_SIZE + 2 || y < 0 || y >= CHUNK_SIZE + 2 || z < 0 || z >= CHUNK_SIZE + 2) return false;
                return voxels[x][y][z] != 0;
            };
        for (int x = 1; x <= CHUNK_SIZE; x++)
        {
            for (int y = 1; y <= CHUNK_SIZE; y++)
            {
                for (int z = 1; z <= CHUNK_SIZE; z++)
                {
                    if (voxels[x][y][z] == 0) continue;
                    for (int f = 0; f < 6; f++)
                    {
                        int nx = x + (int)VoxelData::FaceChecks[f].x;
                        int ny = y + (int)VoxelData::FaceChecks[f].y;
                        int nz = z + (int)VoxelData::FaceChecks[f].z;
                        if (getV(nx, ny, nz)) continue;
                        int vertexAO[4] {};
                        for (int v = 0; v < 4; v++)
                        {
                            Vector3 vPos = VoxelData::CubeVertices[VoxelData::FaceVertexIndices[f][v]];
                            vertices.push_back(vPos.x + x - 1);
                            vertices.push_back(vPos.y + y - 1);
                            vertices.push_back(vPos.z + z - 1);
                            texcoords.push_back(VoxelData::FaceUVs[v].x);
                            texcoords.push_back(VoxelData::FaceUVs[v].y);
                            normals.push_back(VoxelData::FaceNormals[f].x);
                            normals.push_back(VoxelData::FaceNormals[f].y);
                            normals.push_back(VoxelData::FaceNormals[f].z);
                            Vector3 s1 = VoxelData::CachedAOOffsets[f][v][0];
                            Vector3 s2 = VoxelData::CachedAOOffsets[f][v][1];
                            Vector3 c = VoxelData::CachedAOOffsets[f][v][2];
                            bool side1 = getV(nx + (int)s1.x, ny + (int)s1.y, nz + (int)s1.z);
                            bool side2 = getV(nx + (int)s2.x, ny + (int)s2.y, nz + (int)s2.z);
                            bool corner = getV(nx + (int)c.x, ny + (int)c.y, nz + (int)c.z);
                            vertexAO[v] = (side1 && side2) ? 3 : (int)(side1 + side2 + corner);
                            unsigned char brightness = 255 - vertexAO[v] * 50;
                            colors.push_back(brightness);
                            colors.push_back(brightness);
                            colors.push_back(brightness);
                            colors.push_back(255);
                        }
                        if (vertexAO[0] + vertexAO[3] > vertexAO[1] + vertexAO[2])
                        {
                            indices.push_back(vertexCount + 0);
                            indices.push_back(vertexCount + 1);
                            indices.push_back(vertexCount + 2);
                            indices.push_back(vertexCount + 2);
                            indices.push_back(vertexCount + 1);
                            indices.push_back(vertexCount + 3);
                        }
                        else
                        {
                            indices.push_back(vertexCount + 0);
                            indices.push_back(vertexCount + 1);
                            indices.push_back(vertexCount + 3);
                            indices.push_back(vertexCount + 0);
                            indices.push_back(vertexCount + 3);
                            indices.push_back(vertexCount + 2);
                        }
                        vertexCount += 4;
                    }
                }
            }
        }
        Mesh mesh = {0};
        if (vertexCount > 0)
        {
            mesh.vertexCount = vertexCount;
            mesh.triangleCount = (int)indices.size() / 3;
            mesh.vertices = (float*)MemAlloc(vertices.size() * sizeof(float));
            memcpy(mesh.vertices, vertices.data(), vertices.size() * sizeof(float));
            mesh.texcoords = (float*)MemAlloc(texcoords.size() * sizeof(float));
            memcpy(mesh.texcoords, texcoords.data(), texcoords.size() * sizeof(float));
            mesh.normals = (float*)MemAlloc(normals.size() * sizeof(float));
            memcpy(mesh.normals, normals.data(), normals.size() * sizeof(float));
            mesh.indices = (unsigned short*)MemAlloc(indices.size() * sizeof(unsigned short));
            memcpy(mesh.indices, indices.data(), indices.size() * sizeof(unsigned short));
            mesh.colors = (unsigned char*)MemAlloc(colors.size() * sizeof(unsigned char));
            memcpy(mesh.colors, colors.data(), colors.size() * sizeof(unsigned char));
            UploadMesh(&mesh, false);
        }
        return mesh;
    }
};
#endif
