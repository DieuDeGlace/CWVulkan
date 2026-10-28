#pragma once
// Inventory icons drawn from an item's own .cub sprite, as the game shows its
// items: the voxel model at a three-quarter view, top faces lit, sides shaded.
#include "CubModel.h"
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>

inline std::vector<uint8_t> renderItemIcon(const CubModel &cub, int size) {
    std::vector<uint8_t> out((size_t)size * size * 4, 0);
    if (!cub.sx || !cub.sy || !cub.sz)
        return out;
    // View: turned 45 degrees, looking down 30 degrees. Screen x = right,
    // screen y = down; depth toward the viewer.
    const float yaw = 0.785398f, pitch = 0.523599f;
    const float cy = std::cos(yaw), sy = std::sin(yaw), cp = std::cos(pitch), sp = std::sin(pitch);
    auto project = [&](float x, float y, float z, float &u, float &v, float &d) {
        float rx = x * cy - y * sy, ry = x * sy + y * cy; // turn around z
        u = rx;
        v = -(z * cp - ry * sp);
        d = ry * cp + z * sp;
    };
    // Bounds of the solid voxels on screen, to fit the item into the icon.
    int lo[3] = {cub.sx, cub.sy, cub.sz}, hi[3] = {0, 0, 0};
    for (int z = 0; z < cub.sz; z++)
        for (int y = 0; y < cub.sy; y++)
            for (int x = 0; x < cub.sx; x++)
                if (cub.solid(x, y, z)) {
                    lo[0] = std::min(lo[0], x), lo[1] = std::min(lo[1], y), lo[2] = std::min(lo[2], z);
                    hi[0] = std::max(hi[0], x + 1), hi[1] = std::max(hi[1], y + 1), hi[2] = std::max(hi[2], z + 1);
                }
    float minU = 1e9f, maxU = -1e9f, minV = 1e9f, maxV = -1e9f;
    for (int c = 0; c < 8; c++) {
        float u, v, d;
        project((float)(c & 1 ? hi[0] : lo[0]), (float)(c & 2 ? hi[1] : lo[1]), (float)(c & 4 ? hi[2] : lo[2]), u, v, d);
        minU = std::min(minU, u), maxU = std::max(maxU, u), minV = std::min(minV, v), maxV = std::max(maxV, v);
    }
    float scale = (size - 2) / std::max(maxU - minU, maxV - minV);
    float offU = (size - (maxU - minU) * scale) / 2 - minU * scale, offV = (size - (maxV - minV) * scale) / 2 - minV * scale;
    std::vector<float> depth((size_t)size * size, -1e9f);
    // Visible faces only: top (+z), and the two sides facing the viewer.
    struct Face {
        int dx, dy, dz;
        float shade;
        float corners[4][3];
    };
    const Face faces[] = {
        {0, 0, 1, 1.f, {{0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}}},
        {0, -1, 0, .78f, {{0, 0, 0}, {1, 0, 0}, {1, 0, 1}, {0, 0, 1}}},
        {1, 0, 0, .62f, {{1, 0, 0}, {1, 1, 0}, {1, 1, 1}, {1, 0, 1}}},
        {-1, 0, 0, .62f, {{0, 0, 0}, {0, 1, 0}, {0, 1, 1}, {0, 0, 1}}},
        {0, 1, 0, .78f, {{0, 1, 0}, {1, 1, 0}, {1, 1, 1}, {0, 1, 1}}},
    };
    for (int z = 0; z < cub.sz; z++)
        for (int y = 0; y < cub.sy; y++)
            for (int x = 0; x < cub.sx; x++) {
                if (!cub.solid(x, y, z))
                    continue;
                VoxelColor col = cub.color(x, y, z);
                for (auto &f : faces) {
                    if (cub.solid(x + f.dx, y + f.dy, z + f.dz))
                        continue;
                    float u[4], v[4], d[4];
                    for (int k = 0; k < 4; k++) {
                        project(x + f.corners[k][0], y + f.corners[k][1], z + f.corners[k][2], u[k], v[k], d[k]);
                        u[k] = u[k] * scale + offU;
                        v[k] = v[k] * scale + offV;
                    }
                    // Back faces (turned away) are skipped by their winding.
                    float area = 0;
                    for (int k = 0; k < 4; k++)
                        area += u[k] * v[(k + 1) % 4] - u[(k + 1) % 4] * v[k];
                    if (std::abs(area) < 1e-6f)
                        continue;
                    float faceDepth = (d[0] + d[1] + d[2] + d[3]) / 4;
                    int x0 = std::max(0, (int)std::floor(std::min({u[0], u[1], u[2], u[3]}))),
                        x1 = std::min(size - 1, (int)std::ceil(std::max({u[0], u[1], u[2], u[3]}))),
                        y0 = std::max(0, (int)std::floor(std::min({v[0], v[1], v[2], v[3]}))),
                        y1 = std::min(size - 1, (int)std::ceil(std::max({v[0], v[1], v[2], v[3]})));
                    for (int py = y0; py <= y1; py++)
                        for (int px = x0; px <= x1; px++) {
                            float cx = px + .5f, cyy = py + .5f;
                            bool inside = true;
                            for (int k = 0; k < 4 && inside; k++) {
                                float e = (u[(k + 1) % 4] - u[k]) * (cyy - v[k]) - (v[(k + 1) % 4] - v[k]) * (cx - u[k]);
                                inside = area > 0 ? e >= 0 : e <= 0;
                            }
                            if (!inside || depth[py * size + px] > faceDepth)
                                continue;
                            depth[py * size + px] = faceDepth;
                            uint8_t *o = &out[((size_t)py * size + px) * 4];
                            o[0] = (uint8_t)std::min(255.f, col.r * f.shade);
                            o[1] = (uint8_t)std::min(255.f, col.g * f.shade);
                            o[2] = (uint8_t)std::min(255.f, col.b * f.shade);
                            o[3] = 255;
                        }
                }
            }
    return out;
}

// A character for the selection screen: every part in its recorded pose (matrices
// in blocks, mapping a sprite's voxel corner coordinates), seen from the front
// right and a little above, feet at the bottom of the picture.
struct PosedPart {
    const CubModel *cub = nullptr;
    glm::mat4 m{1};
};
inline std::vector<uint8_t> renderPortrait(const std::vector<PosedPart> &parts, float facing, int w, int h) {
    std::vector<uint8_t> out((size_t)w * h * 4, 0);
    const float turn = facing - .55f, pitch = .22f;
    const glm::vec3 up(0, 0, 1);
    const glm::vec3 toViewer(std::cos(turn) * std::cos(pitch), std::sin(turn) * std::cos(pitch), std::sin(pitch));
    const glm::vec3 right = glm::normalize(glm::cross(-toViewer, up)), screenUp = glm::cross(right, -toViewer);
    const glm::vec3 light = glm::normalize(toViewer + up * 1.5f - right * .5f);
    struct Quad {
        glm::vec2 p[4];
        float depth;
        glm::u8vec3 color;
    };
    std::vector<Quad> quads;
    static const int dirs[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
    for (auto &part : parts) {
        const CubModel &cub = *part.cub;
        for (int z = 0; z < cub.sz; z++)
            for (int y = 0; y < cub.sy; y++)
                for (int x = 0; x < cub.sx; x++) {
                    if (!cub.solid(x, y, z))
                        continue;
                    VoxelColor col = cub.color(x, y, z);
                    for (auto &d : dirs) {
                        if (cub.solid(x + d[0], y + d[1], z + d[2]))
                            continue;
                        glm::vec3 n = glm::normalize(glm::mat3(part.m) * glm::vec3(d[0], d[1], d[2]));
                        if (glm::dot(n, toViewer) <= 0)
                            continue;
                        // The face's corners: the unit cube side on direction d.
                        int axis = d[0] ? 0 : d[1] ? 1 : 2, a = (axis + 1) % 3, b = (axis + 2) % 3;
                        Quad q;
                        float depth = 0;
                        for (int k = 0; k < 4; k++) {
                            glm::vec3 c(x, y, z);
                            c[axis] += d[axis] > 0 ? 1.f : 0.f;
                            c[a] += (k == 1 || k == 2) ? 1.f : 0.f;
                            c[b] += (k >= 2) ? 1.f : 0.f;
                            glm::vec3 p = glm::vec3(part.m * glm::vec4(c, 1));
                            q.p[k] = {glm::dot(p, right), -glm::dot(p, screenUp)};
                            depth += glm::dot(p, toViewer) / 4;
                        }
                        q.depth = depth;
                        float shade = .55f + .45f * std::max(0.f, glm::dot(n, light));
                        q.color = {(uint8_t)std::min(255.f, col.r * shade), (uint8_t)std::min(255.f, col.g * shade),
                                   (uint8_t)std::min(255.f, col.b * shade)};
                        quads.push_back(q);
                    }
                }
    }
    if (quads.empty())
        return out;
    glm::vec2 lo(1e9f), hi(-1e9f);
    for (auto &q : quads)
        for (auto &p : q.p)
            lo = glm::min(lo, p), hi = glm::max(hi, p);
    float scale = std::min((w - 4) / (hi.x - lo.x), (h - 4) / (hi.y - lo.y));
    glm::vec2 offset((w - (hi.x - lo.x) * scale) / 2 - lo.x * scale, (h - 2) - hi.y * scale);
    std::vector<float> depth((size_t)w * h, -1e9f);
    for (auto &q : quads) {
        glm::vec2 p[4];
        for (int k = 0; k < 4; k++)
            p[k] = q.p[k] * scale + offset;
        float area = 0;
        for (int k = 0; k < 4; k++)
            area += p[k].x * p[(k + 1) % 4].y - p[(k + 1) % 4].x * p[k].y;
        if (std::abs(area) < 1e-6f)
            continue;
        int x0 = std::max(0, (int)std::floor(std::min({p[0].x, p[1].x, p[2].x, p[3].x}))),
            x1 = std::min(w - 1, (int)std::ceil(std::max({p[0].x, p[1].x, p[2].x, p[3].x}))),
            y0 = std::max(0, (int)std::floor(std::min({p[0].y, p[1].y, p[2].y, p[3].y}))),
            y1 = std::min(h - 1, (int)std::ceil(std::max({p[0].y, p[1].y, p[2].y, p[3].y})));
        for (int py = y0; py <= y1; py++)
            for (int px = x0; px <= x1; px++) {
                float cx = px + .5f, cy = py + .5f;
                bool inside = true;
                for (int k = 0; k < 4 && inside; k++) {
                    float e = (p[(k + 1) % 4].x - p[k].x) * (cy - p[k].y) - (p[(k + 1) % 4].y - p[k].y) * (cx - p[k].x);
                    inside = area > 0 ? e >= 0 : e <= 0;
                }
                if (!inside || depth[(size_t)py * w + px] > q.depth)
                    continue;
                depth[(size_t)py * w + px] = q.depth;
                uint8_t *o = &out[((size_t)py * w + px) * 4];
                o[0] = q.color.r, o[1] = q.color.g, o[2] = q.color.b, o[3] = 255;
            }
    }
    return out;
}
