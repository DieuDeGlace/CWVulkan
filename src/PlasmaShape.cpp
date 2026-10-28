#include "PlasmaShape.h"

#include <glm/gtc/matrix_transform.hpp>

#include <windows.h>
#include <GL/glu.h> // Windows' GLU: the polygon tessellator Plasma fills outlines with

#include <algorithm>
#include <array>
#include <cmath>
#include <deque>
#include <map>
#include <set>

namespace plasma {

namespace {

constexpr float PI = 3.14159265358979f;
constexpr float DEGREES_TO_RADIANS = 0.017453292f;
constexpr float RADIANS_TO_DEGREES = 57.29578f;
constexpr int MAX_SUBDIVISIONS = 6;
constexpr float MIN_STROKE_WIDTH = 0.1f;          // thinner stroke widths are drawn this wide
constexpr float MIN_MITER_COSINE = 0.3f;          // limits how far a miter reaches out
constexpr float STRAIGHT_JOIN_COSINE = 0.95f;     // a sharp point this straight gets no join
constexpr float ROUND_JOIN_STEPS_PER_HALF_TURN = 20; // a round join's steps per 180 degrees of turn
constexpr int ROUND_CAP_SEGMENTS = 20;
// The midpoint weight next to a weighted corner, 1 / (2 cos(pi/4) + 1), about 0.414214.
const float CORNER_WEIGHT = 1.0f / (2.0f * std::cos(PI / 4.0f) + 1.0f);
// The high 16 bits of a vertex's flags: its handle rule (spec 2.3).
constexpr int HANDLE_OWNER = 1, HANDLE_INTERPOLATED = 2;

int pointRule(int32_t flags) { return flags & 0xffff; }
int handleRule(int32_t flags) { return (flags >> 16) & 0xffff; }
int wrap(int i, int n) { return ((i % n) + n) % n; }
int floorDiv(int a, int b) { return a >= 0 ? a / b : -((-a + b - 1) / b); }

glm::vec2 perpendicular(glm::vec2 v) { return glm::vec2(-v.y, v.x); }
glm::vec2 normalizedOrZero(glm::vec2 v) {
    const float length = glm::length(v);
    return length > 0.0f ? v / length : glm::vec2(0);
}
glm::vec2 rotated(glm::vec2 v, float degrees) {
    const float c = std::cos(degrees * DEGREES_TO_RADIANS), s = std::sin(degrees * DEGREES_TO_RADIANS);
    return glm::vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

// Everything a curve or surface point carries; subdivision mixes them all alike.
struct Values {
    glm::vec2 position{0};
    glm::vec4 color{0};
    glm::vec2 uv{0};
    glm::vec4 strokeColor{0};
    float width = 0;
};
Values operator+(Values a, const Values &b) {
    a.position += b.position, a.color += b.color, a.uv += b.uv, a.strokeColor += b.strokeColor, a.width += b.width;
    return a;
}
Values operator*(Values a, float s) {
    a.position *= s, a.color *= s, a.uv *= s, a.strokeColor *= s, a.width *= s;
    return a;
}
Values operator-(const Values &a, const Values &b) { return a + b * -1.0f; }
// a weighted by alpha, b by the rest.
Values blend(const Values &a, const Values &b, float alpha) { return a * alpha + b * (1.0f - alpha); }

// The midpoint weight of a segment next to weighted corners (spec 2.7 step 1, 2.8.2 step 6).
float cornerAlpha(bool aCorner, bool bCorner) {
    if (aCorner && !bCorner)
        return CORNER_WEIGHT;
    if (bCorner && !aCorner)
        return 1.0f - CORNER_WEIGHT;
    return 0.5f;
}

// ---------------------------------------------------------------------------------------------
// Handle rules (spec 2.3): handle owners place the points next to them along the line to the
// point beyond; interpolated points sit between their nearest non-interpolated neighbours.

void applyHandleRules(SmoothMesh &m, const std::vector<std::vector<int>> &loops) {
    std::vector<glm::vec2> &p = m.positions;
    auto handleOf = [&](int v) { return handleRule(m.vertexFlags[(size_t)v]); };
    auto place = [&](int owner, int moved, int target) {
        const glm::vec2 d = p[(size_t)target] - p[(size_t)owner];
        if (d != glm::vec2(0))
            p[(size_t)moved] = p[(size_t)owner] + d * m.vertexParameters[(size_t)owner] / glm::length(d);
    };
    for (const std::vector<int> &loop : loops) {
        const int n = (int)loop.size();
        for (int i = 0; i < n; i++) {
            const int v = loop[(size_t)i];
            if (handleOf(v) != HANDLE_OWNER)
                continue;
            for (int direction : {1, -1}) {
                const int beyond = loop[(size_t)wrap(i + 3 * direction, n)];
                const int target = handleOf(beyond) == HANDLE_OWNER ? beyond : loop[(size_t)wrap(i + 2 * direction, n)];
                place(v, loop[(size_t)wrap(i + direction, n)], target);
            }
        }
        for (int i = 0; i < n; i++) {
            const int v = loop[(size_t)i];
            if (handleOf(v) != HANDLE_INTERPOLATED)
                continue;
            int after = -1, before = -1;
            for (int s = 1; s <= n - 2 && after < 0; s++)
                if (handleOf(loop[(size_t)wrap(i + s, n)]) != HANDLE_INTERPOLATED)
                    after = loop[(size_t)wrap(i + s, n)];
            for (int s = 1; s <= n - 2 && before < 0; s++)
                if (handleOf(loop[(size_t)wrap(i - s, n)]) != HANDLE_INTERPOLATED)
                    before = loop[(size_t)wrap(i - s, n)];
            if (after >= 0 && before >= 0) {
                const float t = m.vertexParameters[(size_t)v];
                p[(size_t)v] = p[(size_t)after] * (1.0f - t) + p[(size_t)before] * t;
            }
        }
    }
}

// ---------------------------------------------------------------------------------------------
// Outline curves (spec 2.5, 2.7): each loop with `k` curve points per segment, refined level by
// level.

struct CurvePoint {
    Values v;
    int32_t flags = 0;
};
struct Curve {
    std::vector<CurvePoint> points;
    bool closed = true;
};

Curve seedCurve(const SmoothMesh &m, const std::vector<int> &loop, int k) {
    Curve c;
    c.closed = !(m.flags & MESH_OPEN);
    const int n = (int)loop.size();
    c.points.resize((size_t)(c.closed ? n * k : (n - 1) * k + 1));
    const bool stroked = m.flags & MESH_STROKE;
    for (int j = 0; j < n; j++) {
        const size_t v = (size_t)loop[(size_t)j];
        CurvePoint &p = c.points[(size_t)(j * k)];
        p.v.position = m.positions[v];
        p.v.color = m.colors[v];
        p.v.uv = m.texCoords[v];
        p.flags = m.vertexFlags[v];
        if (stroked) {
            p.v.strokeColor = m.strokeColors[v];
            p.v.width = std::max(m.strokeWidths[v], MIN_STROKE_WIDTH);
        }
    }
    return c;
}

// One level of curve subdivision (spec 2.7): stride s, m segments, level index `level`.
void subdivideCurve(Curve &c, int s, int m, float w, int level) {
    std::vector<CurvePoint> &p = c.points;
    const int count = (int)p.size();
    const bool first = level == 0;
    auto at = [&](int i) -> CurvePoint & { return p[(size_t)wrap(i, count)]; };
    auto linearRule = [&](const CurvePoint &q) {
        const int r = pointRule(q.flags);
        return r == POINT_LINEAR || (first && r == POINT_WEIGHTED_CORNER);
    };
    const int half = s / 2;
    // 1. Insert the midpoints.
    for (int j = 0; j < m; j++) {
        const CurvePoint &a = at(j * s), &b = at((j + 1) * s);
        const int ra = pointRule(a.flags), rb = pointRule(b.flags);
        const float alpha = first ? cornerAlpha(ra == POINT_WEIGHTED_CORNER, rb == POINT_WEIGHTED_CORNER) : 0.5f;
        CurvePoint &mid = at(j * s + half);
        mid.v = blend(a.v, b.v, alpha);
        mid.flags = (ra == POINT_LINEAR || rb == POINT_LINEAR || (first && (ra == POINT_WEIGHTED_CORNER || rb == POINT_WEIGHTED_CORNER)))
                        ? POINT_LINEAR
                        : POINT_SMOOTH;
    }
    // 2. Move the old points towards their new neighbours.
    const int lastOld = c.closed ? m : m - 1;
    for (int j = 1; j <= lastOld; j++) {
        CurvePoint &q = at(j * s);
        const int r = pointRule(q.flags);
        if (r == POINT_SHARP || (r == POINT_WEIGHTED_CORNER && !first))
            continue;
        const Values average = (at(j * s - half).v + at(j * s + half).v) * 0.5f;
        q.v = r == POINT_SMOOTH ? q.v + (average - q.v) * w : average;
    }
    // 3. Straighten the runs between linear points.
    const int lastSegment = c.closed ? m : m - 1;
    for (int j = 0; j <= lastSegment; j++) {
        const CurvePoint &a = at(j * s), &b = at((j + 1) * s);
        if (linearRule(a) && linearRule(b)) {
            CurvePoint &mid = at(j * s + half);
            mid.v = (a.v + b.v) * 0.5f;
            mid.flags = POINT_SHARP;
        }
    }
}

void refineCurve(Curve &c, int levels, float w) {
    const int count = (int)c.points.size();
    for (int level = 0; level < levels; level++) {
        const int stride = 1 << (levels - level);
        subdivideCurve(c, stride, count / stride, w, level);
    }
}

// ---------------------------------------------------------------------------------------------
// The stroke (spec 2.10): a ribbon of vertex pairs along the curve, with caps on open curves.

struct Pair {
    Vertex a, b;
};

std::vector<Pair> ribbon(const Curve &curve, const SmoothMesh &mesh, int k) {
    const std::vector<CurvePoint> &pts = curve.points;
    const int count = (int)pts.size();
    const bool open = !curve.closed;
    const float alignment = (float)mesh.strokeAlignment;
    auto position = [&](int i) { return pts[(size_t)wrap(i, count)].v.position; };
    // The control points further back and further on, used when a neighbour coincides.
    auto backward = [&](int j) { return wrap(floorDiv(j - k - 1 + count, k) * k, count); };
    auto forward = [&](int j) { return wrap(floorDiv(j + k + 1, k) * k, count); };
    std::vector<Pair> pairs;
    float u = 0;
    const int visits = open ? count : count + 1;
    for (int visit = 0; visit < visits; visit++) {
        const int j = visit % count;
        const CurvePoint &cp = pts[(size_t)j];
        const glm::vec2 p = cp.v.position;
        const float h = cp.v.width * 0.5f;
        auto emit = [&](glm::vec2 a, glm::vec2 b) {
            pairs.push_back({{a, cp.v.strokeColor, glm::vec2(u, 0)}, {b, cp.v.strokeColor, glm::vec2(u, 1)}});
        };
        auto emitEnd = [&](glm::vec2 d) {
            const glm::vec2 n = normalizedOrZero(perpendicular(d));
            emit(p + n * h * (1.0f + alignment), p + n * h * (alignment - 1.0f));
        };
        if (open && j == 0) {
            glm::vec2 d = p - position(1);
            if (d == glm::vec2(0))
                d = p - position(forward(0));
            emitEnd(d);
        } else if (open && j == count - 1) {
            glm::vec2 d = position(j - 1) - p;
            if (d == glm::vec2(0))
                d = position(backward(j)) - p;
            emitEnd(d);
        } else {
            glm::vec2 e1 = position(j - 1) - p, e2 = p - position(j + 1);
            if (e1 == glm::vec2(0))
                e1 = position(backward(j)) - p;
            if (e2 == glm::vec2(0))
                e2 = p - position(forward(j));
            const glm::vec2 n1 = normalizedOrZero(perpendicular(e1)), n2 = normalizedOrZero(perpendicular(e2));
            glm::vec2 miter = normalizedOrZero(n1 + n2);
            const bool sharp = j % k == 0 && pointRule(cp.flags) == POINT_SHARP;
            if (sharp) {
                const glm::vec2 n = n1 != glm::vec2(0) ? n1 : n2;
                miter /= std::max(glm::dot(miter, n), MIN_MITER_COSINE);
            }
            const glm::vec2 reach = miter * h, offset = reach * alignment;
            if (!sharp || glm::dot(n1, n2) >= STRAIGHT_JOIN_COSINE) {
                emit(p + reach + offset, p - reach + offset);
            } else {
                // The outer side of the bend gets the join; the inner side meets at the mean.
                const bool outer = glm::dot(miter, e2) >= 0.0f;
                const glm::vec2 s1 = n1 * h, s2 = n2 * h, mean = (s1 + s2) * 0.5f;
                emit(p + s1 + offset, p - s1 + offset);
                if (mesh.strokeJoint == JOIN_ROUND) {
                    float theta = std::acos(std::clamp(glm::dot(n2, n1), -1.0f, 1.0f)) * RADIANS_TO_DEGREES;
                    const int steps = (int)(std::abs(theta) / 180.0f * ROUND_JOIN_STEPS_PER_HALF_TURN);
                    if (!outer)
                        theta = -theta;
                    const float step = steps > 0 ? theta / (float)steps : 0.0f;
                    glm::vec2 r = s1;
                    for (int i = 1; i < steps; i++) {
                        r = rotated(r, step);
                        if (outer)
                            emit(p + r + offset, p - mean + offset);
                        else
                            emit(p + mean + offset, p - r + offset);
                    }
                } else if (outer) // miter
                    emit(p + reach + offset, p - mean + offset);
                else
                    emit(p + mean + offset, p - reach + offset);
                emit(p + s2 + offset, p - s2 + offset);
            }
        }
        const CurvePoint &next = pts[(size_t)((j + 1) % count)];
        const float widths = cp.v.width + next.v.width;
        if (widths > 0.0f)
            u += 2.0f * glm::length(p - next.v.position) / widths;
    }
    return pairs;
}

// A cap from ribbon pairs `j` and `j + 1`, `end` false at the curve's start (spec 2.10).
std::vector<Pair> cap(const std::vector<Pair> &pairs, size_t j, bool end, int type) {
    std::vector<Pair> out;
    if (pairs.size() < 2 || (type != CAP_ROUND && type != CAP_SQUARE))
        return out;
    const Pair &p0 = pairs[j], &p1 = pairs[j + 1];
    const float t = end ? 1.0f : 0.0f;
    const glm::vec2 mid0 = (p0.a.position + p0.b.position) * 0.5f, mid1 = (p1.a.position + p1.b.position) * 0.5f;
    const glm::vec2 c = glm::mix(mid0, mid1, t);
    const glm::vec2 r = glm::mix(p0.a.position, p1.a.position, t) - c;
    const glm::vec2 q(r.y, -r.x); // outward at the start, inward at the end
    const glm::vec4 color = glm::mix(p0.a.color, p1.a.color, t);
    const float u0 = p0.a.uv.x, u1 = p1.a.uv.x, u = glm::mix(u0, u1, t);
    const float between = glm::length(mid0 - mid1);
    const float reach = between > 0.0f ? (u1 - u0) * glm::length(r) / between : 0.0f;
    if (type == CAP_SQUARE) {
        const glm::vec2 tip = end ? c - q : c + q;
        const float tipU = end ? u + reach : u - reach;
        out.push_back({{tip + r, color, glm::vec2(tipU, 0)}, {tip - r, color, glm::vec2(tipU, 1)}});
        return out;
    }
    for (int i = 0; i < ROUND_CAP_SEGMENTS; i++) {
        float phi = ((float)i + 0.5f) * (PI / 2) / ((float)ROUND_CAP_SEGMENTS + 0.5f);
        if (end)
            phi += (0.5f / (float)ROUND_CAP_SEGMENTS + 1.0f) * (PI / 2);
        const float cs = std::cos(phi), sn = std::sin(phi);
        const float capU = u - reach * cs;
        out.push_back({{c + q * cs + r * sn, color, glm::vec2(capU, 0.5f - 0.5f * sn)},
                       {c + q * cs - r * sn, color, glm::vec2(capU, 0.5f + 0.5f * sn)}});
    }
    return out;
}

// The ribbon's triangles: a strip with alternating winding, wrapping on a closed curve.
void strokeCurve(const Curve &curve, const SmoothMesh &mesh, int k, Triangles &out) {
    if (curve.points.empty())
        return;
    std::vector<Pair> pairs = ribbon(curve, mesh, k);
    if (!curve.closed && pairs.size() >= 2) {
        std::vector<Pair> start = cap(pairs, 0, false, mesh.strokeCap);
        std::vector<Pair> finish = cap(pairs, pairs.size() - 2, true, mesh.strokeCap);
        pairs.insert(pairs.begin(), start.begin(), start.end());
        pairs.insert(pairs.end(), finish.begin(), finish.end());
    }
    const uint32_t base = (uint32_t)out.vertices.size();
    for (const Pair &p : pairs)
        out.vertices.push_back(p.a), out.vertices.push_back(p.b);
    const int n = (int)pairs.size() * 2;
    const int triangles = curve.closed ? n : n - 2;
    for (int t = 0; t < triangles; t++)
        for (int corner : {t, t + 1 + (t & 1), t + 1 + ((t + 1) & 1)})
            out.indices.push_back(base + (uint32_t)(corner % n));
}

// ---------------------------------------------------------------------------------------------
// The outline fill (spec 2.9): the curves of all loops filled together, coloured by the plane
// that best fits their colours.

void outlineFill(const std::vector<Curve> &curves, Triangles &fill) {
    size_t count = 0;
    glm::vec2 meanPosition(0);
    glm::vec4 meanColor(0);
    for (const Curve &c : curves)
        for (const CurvePoint &p : c.points)
            meanPosition += p.v.position, meanColor += p.v.color, count++;
    if (!count)
        return;
    meanPosition /= (float)count, meanColor /= (float)count;
    float sxx = 0, syy = 0, sxy = 0;
    glm::vec4 sxc(0), syc(0);
    for (const Curve &c : curves)
        for (const CurvePoint &p : c.points) {
            const glm::vec2 d = p.v.position - meanPosition;
            sxx += d.x * d.x, syy += d.y * d.y, sxy += d.x * d.y;
            sxc += d.x * (p.v.color - meanColor), syc += d.y * (p.v.color - meanColor);
        }
    // spec: open question - the game has no guard for a degenerate outline (determinant 0, which
    // gives non-finite colours); here such an outline takes the mean colour.
    const float determinant = sxx * syy - sxy * sxy;
    glm::vec4 gradientX(0), gradientY(0);
    if (determinant != 0.0f) {
        gradientX = (sxc * syy - syc * sxy) / determinant;
        gradientY = (syc * sxx - sxc * sxy) / determinant;
    }
    std::vector<glm::vec4> planeColors;
    glm::vec4 lo(0), hi(1);
    for (const Curve &c : curves)
        for (const CurvePoint &p : c.points) {
            const glm::vec2 d = p.v.position - meanPosition;
            planeColors.push_back(meanColor + gradientX * d.x + gradientY * d.y);
            lo = glm::min(lo, planeColors.back()), hi = glm::max(hi, planeColors.back());
        }
    std::vector<std::vector<uint32_t>> contours;
    size_t i = 0;
    for (const Curve &c : curves) {
        std::vector<uint32_t> contour;
        for (const CurvePoint &p : c.points) {
            contour.push_back((uint32_t)fill.vertices.size());
            fill.vertices.push_back({p.v.position, (planeColors[i++] - lo) / (hi - lo), p.v.uv});
        }
        contours.push_back(std::move(contour));
    }
    std::vector<uint32_t> indices = tessellate(fill.vertices, contours);
    fill.indices.insert(fill.indices.end(), indices.begin(), indices.end());
}

// The fill of an unsubdivided shape (spec 2.6): a fan per face.
void fanFill(const SmoothMesh &m, Triangles &fill) {
    for (size_t v = 0; v < m.positions.size(); v++)
        fill.vertices.push_back({m.positions[v], m.colors[v], m.texCoords[v]});
    for (const std::vector<int> &face : m.faces)
        for (size_t i = 1; i + 1 < face.size(); i++)
            for (int v : {face[0], face[i], face[i + 1]})
                fill.indices.push_back((uint32_t)v);
}

// ---------------------------------------------------------------------------------------------
// Surface subdivision (spec 2.8): the polygons become quads around face and edge points, and
// every further level splits each quad in four. Points keep their identity across levels: a
// level's vertices are the previous level's, then its edge points, then its face points.

struct SurfacePoint {
    Values v;
    bool sharp = false, linear = false, corner = false, boundary = false;
    Values sum;
    int count = 0;
    void add(const Values &x) { sum = sum + x, count++; }
};

struct QuadEdge {
    int a = 0, b = 0;
    int quads[2] = {-1, -1};
    bool open() const { return quads[1] < 0; } // a quad is missing on one side
};

struct Level {
    int vertexCount = 0;
    std::vector<std::array<int, 4>> quads;
    std::vector<QuadEdge> edges;
    std::vector<std::array<int, 4>> sides;   // the edge of each quad's side j (corner j to j + 1)
    std::vector<int> edgePoints, facePoints; // the next level's new points
};

// Finds a level's edges, and marks the ends of edges with one quad as boundary.
void connect(Level &level, std::vector<SurfacePoint> &points) {
    std::map<std::pair<int, int>, int> found;
    level.sides.assign(level.quads.size(), {});
    for (size_t q = 0; q < level.quads.size(); q++)
        for (int j = 0; j < 4; j++) {
            const int a = level.quads[q][(size_t)j], b = level.quads[q][(size_t)((j + 1) % 4)];
            auto [it, added] = found.try_emplace({std::min(a, b), std::max(a, b)}, (int)level.edges.size());
            if (added)
                level.edges.push_back({a, b});
            QuadEdge &e = level.edges[(size_t)it->second];
            (e.quads[0] < 0 ? e.quads[0] : e.quads[1]) = (int)q;
            level.sides[q][(size_t)j] = it->second;
        }
    for (const QuadEdge &e : level.edges)
        if (e.open())
            points[(size_t)e.a].boundary = points[(size_t)e.b].boundary = true;
}

// A point moved by what was gathered into it (spec 2.8.2 step 9, 2.8.3 pass 6).
void movePoint(SurfacePoint &p, float w) {
    if (p.sharp || p.count == 0)
        return;
    const Values average = p.sum * (1.0f / (float)p.count);
    if (!p.boundary)
        p.v = p.v * 0.25f + average * 0.75f;
    else if (!p.linear)
        p.v = p.v * (1.0f - w) + average * w;
    else
        p.v = average;
}

// One level step (spec 2.8.3), making the next level's points from level `l`.
void refineLevel(const Level &l, std::vector<SurfacePoint> &pts, float w) {
    for (int i = 0; i < l.vertexCount; i++)
        pts[(size_t)i].sum = Values{}, pts[(size_t)i].count = 0;
    for (size_t q = 0; q < l.quads.size(); q++) {
        SurfacePoint &f = pts[(size_t)l.facePoints[q]];
        f.v = Values{};
        for (int c : l.quads[q])
            f.v = f.v + pts[(size_t)c].v * 0.25f;
        f.sharp = f.linear = f.corner = false;
    }
    for (size_t e = 0; e < l.edges.size(); e++) {
        const QuadEdge &edge = l.edges[e];
        const SurfacePoint &a = pts[(size_t)edge.a], &b = pts[(size_t)edge.b];
        SurfacePoint &p = pts[(size_t)l.edgePoints[e]];
        p.sharp = p.linear = p.corner = false;
        if (!edge.open() && !a.sharp && !b.sharp) {
            const Values &f1 = pts[(size_t)l.facePoints[(size_t)edge.quads[0]]].v, &f2 = pts[(size_t)l.facePoints[(size_t)edge.quads[1]]].v;
            p.v = ((a.v + b.v) * (1.0f - w) + (f1 + f2) * w) * 0.5f;
        } else {
            p.v = (a.v + b.v) * 0.5f;
            p.linear = a.linear || b.linear;
        }
    }
    for (const QuadEdge &edge : l.edges) {
        const Values mid = (pts[(size_t)edge.a].v + pts[(size_t)edge.b].v) * 0.5f;
        for (int end : {edge.a, edge.b})
            if (edge.open() || !pts[(size_t)end].boundary)
                pts[(size_t)end].add(mid);
    }
    for (size_t q = 0; q < l.quads.size(); q++)
        for (int c : l.quads[q])
            if (!pts[(size_t)c].boundary)
                pts[(size_t)c].add(pts[(size_t)l.facePoints[q]].v);
    for (int i = 0; i < l.vertexCount; i++)
        movePoint(pts[(size_t)i], w);
    for (size_t e = 0; e < l.edges.size(); e++) {
        const QuadEdge &edge = l.edges[e];
        if (edge.open() && pts[(size_t)edge.a].linear && pts[(size_t)edge.b].linear) {
            SurfacePoint &p = pts[(size_t)l.edgePoints[e]];
            p.v = (pts[(size_t)edge.a].v + pts[(size_t)edge.b].v) * 0.5f;
            p.linear = false, p.sharp = true;
        }
    }
}

struct Polygon {
    std::vector<int> corners; // shape vertices
    std::vector<int> sides;   // the polygon edge from corner i to i + 1
    int facePoint = -1;
};
struct PolygonEdge {
    int a = 0, b = 0; // shape vertices
    int polygons[2] = {-1, -1};
    int point = -1;
    bool open() const { return polygons[1] < 0; }
};

// The subdivided fill (spec 2.8): returns the last level's quad corners for the bounds.
void surfaceFill(const SmoothMesh &m, int levels, Triangles &fill, std::vector<glm::vec2> &corners) {
    const int vertexCount = (int)m.positions.size();
    // Level 0 from the polygons (faces of 3 or more vertices).
    std::vector<Polygon> polygons;
    std::set<int> used;
    for (const std::vector<int> &face : m.faces) {
        if (face.size() < 3 || std::any_of(face.begin(), face.end(), [&](int v) { return v < 0 || v >= vertexCount; }))
            continue;
        polygons.push_back({face, {}, -1});
        used.insert(face.begin(), face.end());
    }
    if (polygons.empty())
        return;
    std::map<int, int> controlPoint; // shape vertex -> point
    for (int v : used) {
        const int id = (int)controlPoint.size();
        controlPoint[v] = id;
    }
    std::vector<PolygonEdge> polygonEdges;
    std::map<std::pair<int, int>, int> edgeOf;
    for (size_t f = 0; f < polygons.size(); f++) {
        Polygon &poly = polygons[f];
        for (size_t i = 0; i < poly.corners.size(); i++) {
            const int a = poly.corners[i], b = poly.corners[(i + 1) % poly.corners.size()];
            auto [it, added] = edgeOf.try_emplace({std::min(a, b), std::max(a, b)}, (int)polygonEdges.size());
            if (added)
                polygonEdges.push_back({a, b});
            PolygonEdge &e = polygonEdges[(size_t)it->second];
            if (e.polygons[0] < 0)
                e.polygons[0] = (int)f;
            else if (e.polygons[1] < 0 && e.polygons[0] != (int)f)
                e.polygons[1] = (int)f;
            poly.sides.push_back(it->second);
        }
    }
    int nextPoint = (int)controlPoint.size();
    for (PolygonEdge &e : polygonEdges)
        e.point = nextPoint++;
    for (Polygon &poly : polygons)
        poly.facePoint = nextPoint++;

    std::vector<Level> topology((size_t)levels);
    topology[0].vertexCount = nextPoint;
    for (const Polygon &poly : polygons) {
        const size_t n = poly.corners.size();
        for (size_t i = 0; i < n; i++)
            topology[0].quads.push_back({poly.facePoint, polygonEdges[(size_t)poly.sides[(i + n - 1) % n]].point,
                                         controlPoint[poly.corners[i]], polygonEdges[(size_t)poly.sides[i]].point});
    }
    for (int k = 0; k + 1 < levels; k++) {
        Level &l = topology[(size_t)k], &next = topology[(size_t)k + 1];
        // (the edges are needed to number the new points; connect() finds them below as well)
        std::map<std::pair<int, int>, int> found;
        std::vector<std::pair<int, int>> edges;
        for (const auto &q : l.quads)
            for (int j = 0; j < 4; j++) {
                const int a = q[(size_t)j], b = q[(size_t)((j + 1) % 4)];
                if (found.try_emplace({std::min(a, b), std::max(a, b)}, (int)edges.size()).second)
                    edges.push_back({a, b});
            }
        int point = l.vertexCount;
        l.edgePoints.resize(edges.size());
        for (int &p : l.edgePoints)
            p = point++;
        l.facePoints.resize(l.quads.size());
        for (int &p : l.facePoints)
            p = point++;
        next.vertexCount = point;
        for (size_t q = 0; q < l.quads.size(); q++) {
            const auto &c = l.quads[q];
            auto edgePoint = [&](int j) {
                const int a = c[(size_t)wrap(j, 4)], b = c[(size_t)wrap(j + 1, 4)];
                return l.edgePoints[(size_t)found[{std::min(a, b), std::max(a, b)}]];
            };
            for (int j = 0; j < 4; j++)
                next.quads.push_back({l.facePoints[q], edgePoint(j - 1), c[(size_t)j], edgePoint(j)});
        }
    }
    std::vector<SurfacePoint> pts((size_t)topology.back().vertexCount);
    for (Level &l : topology)
        connect(l, pts); // (edge order matches the numbering above: both follow the quads)

    // The first step (spec 2.8.2): polygons to level 0.
    const float w = m.smoothWeight;
    for (auto [v, id] : controlPoint)
        pts[(size_t)id] = SurfacePoint{};
    // spec: open question - boundary is kept per point; control points take it from the
    // polygons (and two-vertex faces) as here, the other points from their level's quads.
    for (const Polygon &poly : polygons)
        for (size_t i = 0; i < poly.corners.size(); i++)
            if (polygonEdges[(size_t)poly.sides[i]].open())
                pts[(size_t)controlPoint[poly.corners[i]]].boundary =
                    pts[(size_t)controlPoint[poly.corners[(i + 1) % poly.corners.size()]]].boundary = true;
    for (const std::vector<int> &face : m.faces)
        if (face.size() == 2)
            for (int v : face)
                if (auto it = controlPoint.find(v); it != controlPoint.end())
                    pts[(size_t)it->second].boundary = true;
    for (auto [v, id] : controlPoint) {
        SurfacePoint &p = pts[(size_t)id];
        p.v.position = m.positions[(size_t)v];
        p.v.color = m.colors[(size_t)v];
        p.v.uv = m.texCoords[(size_t)v];
        const int r = pointRule(m.vertexFlags[(size_t)v]);
        p.sharp = r == POINT_SHARP;
        p.linear = r == POINT_LINEAR || r == POINT_WEIGHTED_CORNER;
        p.corner = r == POINT_WEIGHTED_CORNER;
    }
    auto control = [&](int v) -> SurfacePoint & { return pts[(size_t)controlPoint[v]]; };
    // (new points keep the boundary their level's quads gave them)
    for (const Polygon &poly : polygons) {
        SurfacePoint &f = pts[(size_t)poly.facePoint];
        f.v = Values{};
        f.sharp = f.linear = f.corner = false;
        for (int v : poly.corners)
            f.v = f.v + control(v).v * (1.0f / (float)poly.corners.size());
    }
    auto edgeAlpha = [&](const PolygonEdge &e) { return cornerAlpha(control(e.a).corner, control(e.b).corner); };
    for (const PolygonEdge &e : polygonEdges) {
        const SurfacePoint &a = control(e.a), &b = control(e.b);
        SurfacePoint &p = pts[(size_t)e.point];
        p.sharp = p.linear = p.corner = false;
        if (e.open() || a.sharp || b.sharp) {
            p.v = blend(a.v, b.v, edgeAlpha(e));
            p.linear = a.linear || b.linear;
        } else {
            const Values &f1 = pts[(size_t)polygons[(size_t)e.polygons[0]].facePoint].v;
            const Values &f2 = pts[(size_t)polygons[(size_t)e.polygons[1]].facePoint].v;
            p.v = ((a.v + b.v) * (1.0f - w) + (f1 + f2) * w) * 0.5f;
        }
    }
    for (const Polygon &poly : polygons) // every interior edge twice, once from each polygon
        for (int side : poly.sides) {
            const PolygonEdge &e = polygonEdges[(size_t)side];
            const Values mid = blend(control(e.a).v, control(e.b).v, edgeAlpha(e));
            for (int end : {e.a, e.b})
                if (e.open() || !control(end).boundary)
                    control(end).add(mid);
        }
    for (const Polygon &poly : polygons)
        for (int v : poly.corners)
            if (!control(v).boundary)
                control(v).add(pts[(size_t)poly.facePoint].v);
    for (auto [v, id] : controlPoint)
        movePoint(pts[(size_t)id], w);
    for (const PolygonEdge &e : polygonEdges)
        if (e.open() && control(e.a).linear && control(e.b).linear) {
            SurfacePoint &p = pts[(size_t)e.point];
            p.v = (control(e.a).v + control(e.b).v) * 0.5f;
            p.linear = false, p.sharp = true;
        }
    for (auto [v, id] : controlPoint)
        if (pts[(size_t)id].corner) {
            SurfacePoint &p = pts[(size_t)id];
            p.linear = p.corner = false, p.sharp = true;
        }
    for (int k = 0; k + 1 < levels; k++)
        refineLevel(topology[(size_t)k], pts, w);

    const Level &last = topology.back();
    for (int i = 0; i < last.vertexCount; i++)
        fill.vertices.push_back({pts[(size_t)i].v.position, pts[(size_t)i].v.color, pts[(size_t)i].v.uv});
    for (const auto &q : last.quads) {
        for (int c : {q[0], q[1], q[2], q[2], q[3], q[0]})
            fill.indices.push_back((uint32_t)c);
        for (int c : q)
            corners.push_back(pts[(size_t)c].v.position);
    }
}

void boundsOf(const std::vector<glm::vec2> &points, ShapeDrawing &d) {
    if (points.empty())
        return;
    d.minimum = d.maximum = points[0];
    for (glm::vec2 p : points)
        d.minimum = glm::min(d.minimum, p), d.maximum = glm::max(d.maximum, p);
}

// The GLU tessellator's callbacks: vertices travel as their index plus one (0 is "none").
struct Tessellation {
    std::vector<Vertex> *vertices;
    std::vector<uint32_t> indices;
};
void CALLBACK tessVertex(void *vertex, void *context) {
    static_cast<Tessellation *>(context)->indices.push_back((uint32_t)((uintptr_t)vertex - 1));
}
void CALLBACK tessCombine(GLdouble[3], void *sources[4], GLfloat weights[4], void **made, void *context) {
    Tessellation &t = *static_cast<Tessellation *>(context);
    Vertex v{glm::vec2(0), glm::vec4(0), glm::vec2(0)};
    for (int i = 0; i < 4; i++)
        if (sources[i]) {
            const Vertex &s = (*t.vertices)[(uintptr_t)sources[i] - 1];
            v.position += s.position * weights[i], v.color += s.color * weights[i], v.uv += s.uv * weights[i];
        }
    t.vertices->push_back(v);
    *made = (void *)(uintptr_t)t.vertices->size();
}
// (with an edge flag callback the tessellator emits plain triangles only)
void CALLBACK tessEdgeFlag(GLboolean) {}
void CALLBACK tessError(GLenum, void *) {}

} // namespace

glm::mat4 pivoted(glm::vec2 translation, glm::vec2 pivot, glm::vec3 degrees, const glm::mat4 &deformation) {
    glm::mat4 m(1);
    m = glm::translate(m, glm::vec3(translation, 0));
    m = glm::translate(m, glm::vec3(pivot, 0));
    m = glm::rotate(m, degrees.x * DEGREES_TO_RADIANS, glm::vec3(1, 0, 0));
    m = glm::rotate(m, degrees.y * DEGREES_TO_RADIANS, glm::vec3(0, 1, 0));
    m = glm::rotate(m, degrees.z * DEGREES_TO_RADIANS, glm::vec3(0, 0, 1));
    m = m * deformation;
    return glm::translate(m, glm::vec3(-pivot, 0));
}

bool smoothMeshOf(const plx::Object &o, SmoothMesh &m) {
    if (o.kind != "SmoothMeshShape")
        return false;
    m = SmoothMesh{};
    auto property = [&](const char *name) { return o.property(std::string("SmoothMeshShape.") + name); };
    for (const plx::Chunk &p : o.properties)
        if (p.name == "SmoothMeshShape.face") {
            const std::vector<int32_t> face = plx::countedArray<int32_t>(&p);
            m.faces.push_back(std::vector<int>(face.begin(), face.end()));
        }
    m.positions = plx::liveArray<glm::vec2>(property("vertexPositions"));
    m.texCoords = plx::liveArray<glm::vec2>(property("vertexTexCoords"));
    m.colors = plx::liveArray<glm::vec4>(property("vertexColors"));
    m.strokeColors = plx::liveArray<glm::vec4>(property("strokeColors"));
    m.strokeWidths = plx::liveArray<float>(property("strokeWidths"));
    m.vertexFlags = plx::countedArray<int32_t>(property("vertexFlags"));
    m.vertexParameters = plx::countedArray<float>(property("vertexParameters"));
    // Every per-vertex array takes the vertex count of the positions, new entries zero.
    const size_t n = m.positions.size();
    m.texCoords.resize(n, glm::vec2(0));
    m.colors.resize(n, glm::vec4(0));
    m.strokeColors.resize(n, glm::vec4(0));
    m.strokeWidths.resize(n, 0.0f);
    m.vertexFlags.resize(n, 0);
    m.vertexParameters.resize(n, 0.0f);

    m.texture = plx::live<int32_t>(property("texture"), -1);
    m.strokeTexture = plx::live<int32_t>(property("strokeTexture"), -1);
    m.textureTranslation = plx::live<glm::vec2>(property("textureTranslation"), glm::vec2(0));
    m.texturePivot = plx::live<glm::vec2>(property("texturePivot"), glm::vec2(0));
    m.textureRotation = plx::live<glm::vec3>(property("textureRotation"), glm::vec3(0));
    m.textureDeformation = plx::live<glm::mat4>(property("textureDeformation"), glm::mat4(1));
    m.textureOpacity = plx::live<float>(property("textureOpacity"), 1.0f);
    m.textureBrightness = plx::live<float>(property("textureBrightness"), 1.0f);
    m.textureContrast = plx::live<float>(property("textureContrast"), 1.0f);
    m.textureSaturation = plx::live<float>(property("textureSaturation"), 1.0f);
    m.strokeTextureOpacity = plx::live<float>(property("strokeTextureOpacity"), 1.0f);
    m.strokeTextureBrightness = plx::live<float>(property("strokeTextureBrightness"), 1.0f);
    m.strokeTextureContrast = plx::live<float>(property("strokeTextureContrast"), 1.0f);
    m.strokeTextureSaturation = plx::live<float>(property("strokeTextureSaturation"), 1.0f);
    m.strokeTextureStretch = plx::live<float>(property("strokeTextureStretch"), 1.0f);

    auto leafInt = [&](const char *name, int fallback) {
        const plx::Chunk *c = property(name);
        return c ? c->as<int32_t>() : fallback;
    };
    auto leafFloat = [&](const char *name, float fallback) {
        const plx::Chunk *c = property(name);
        return c ? c->as<float>() : fallback;
    };
    m.subdivisions = std::clamp(leafInt("subdivisions", m.subdivisions), 0, MAX_SUBDIVISIONS);
    m.smoothWeight = leafFloat("smoothWeight", m.smoothWeight);
    m.flags = leafInt("flags", 0);
    m.strokeJoint = leafInt("strokeJointType", 0);
    m.strokeCap = leafInt("strokeCapType", CAP_BUTT);
    m.strokeAlignment = leafInt("strokeAlignment", 0);
    m.strokePattern = leafInt("strokePattern", 0);
    m.strokeDash = leafFloat("strokeDash", 1.0f);
    m.strokeGap = leafFloat("strokeGap", 1.0f);
    return true;
}

ShapeDrawing build(const SmoothMesh &source) {
    ShapeDrawing d;
    SmoothMesh m = source; // (the handle rules move the live positions)
    for (std::vector<int> &face : m.faces) // (a face naming a vertex that does not exist is dropped)
        if (std::any_of(face.begin(), face.end(), [&](int v) { return v < 0 || v >= (int)m.positions.size(); }))
            face.clear();
    const std::vector<std::vector<int>> loops = outlineLoops(m.faces);
    applyHandleRules(m, loops);
    const int levels = std::clamp(m.subdivisions, 0, MAX_SUBDIVISIONS);
    const int k = 1 << levels; // curve points per outline segment
    d.filled = !(m.flags & MESH_NO_FILL);
    d.stroked = (m.flags & MESH_STROKE) != 0;

    d.fillLook = {m.texture, pivoted(m.textureTranslation, m.texturePivot, m.textureRotation, m.textureDeformation), m.textureOpacity,
                  m.textureBrightness, m.textureContrast, m.textureSaturation};
    d.strokeLook = {m.strokeTexture, glm::mat4(1), m.strokeTextureOpacity, m.strokeTextureBrightness, m.strokeTextureContrast,
                    m.strokeTextureSaturation};
    if (m.strokeTextureStretch > 0.0f)
        d.strokeLook.matrix[0][0] = 1.0f / m.strokeTextureStretch;

    std::vector<Curve> curves;
    for (const std::vector<int> &loop : loops) {
        curves.push_back(seedCurve(m, loop, k));
        refineCurve(curves.back(), levels, m.smoothWeight);
    }
    std::vector<glm::vec2> quadCorners;
    if (d.filled) {
        if (m.flags & MESH_OUTLINE)
            outlineFill(curves, d.fill);
        else if (levels == 0)
            fanFill(m, d.fill);
        else
            surfaceFill(m, levels, d.fill, quadCorners);
    }
    // spec: open question - dashed strokes (strokePattern) and extrusion (MESH_EXTRUDED) are only
    // summarised by the spec and unused by the shipped files; they are drawn as solid strokes and
    // without extrusion.
    if (d.stroked)
        for (const Curve &c : curves)
            strokeCurve(c, m, k, d.stroke);

    // The bounds (spec 2.14).
    std::vector<glm::vec2> extent;
    if (levels == 0)
        extent = m.positions;
    else if (d.stroked)
        for (const Vertex &v : d.stroke.vertices)
            extent.push_back(v.position);
    else if (!(m.flags & MESH_OUTLINE))
        extent = quadCorners;
    else
        for (const Curve &c : curves)
            for (const CurvePoint &p : c.points)
                extent.push_back(p.v.position);
    boundsOf(extent, d);
    return d;
}

bool buildSmoothMesh(const plx::Object &shape, ShapeDrawing &out) {
    SmoothMesh m;
    if (!smoothMeshOf(shape, m))
        return false;
    out = build(m);
    return true;
}

std::vector<std::vector<int>> outlineLoops(const std::vector<std::vector<int>> &faces) {
    // The free half-edges: those whose opposite no other face has.
    std::set<std::pair<int, int>> free;
    for (const std::vector<int> &f : faces) {
        if (f.size() >= 3) {
            for (size_t i = 0; i < f.size(); i++) {
                const int a = f[i], b = f[(i + 1) % f.size()];
                if (!free.erase({b, a}))
                    free.insert({a, b});
            }
        } else if (f.size() == 2) {
            free.insert({f[0], f[1]});
            free.insert({f[1], f[0]});
        }
    }
    // Where each vertex's free half-edges lead, in face order.
    std::map<int, std::deque<int>> leads;
    for (const std::vector<int> &f : faces)
        if (f.size() >= 2)
            for (size_t i = 0; i < f.size(); i++) {
                const int a = f[i], b = f[(i + 1) % f.size()];
                if (free.count({a, b}))
                    leads[a].push_back(b);
            }
    std::vector<std::vector<int>> loops;
    while (!leads.empty()) {
        int start = leads.begin()->first;
        if (faces.size() == 1 && !faces[0].empty() && leads.count(faces[0][0]))
            start = faces[0][0];
        std::vector<int> loop;
        int current = start;
        do {
            auto it = leads.find(current);
            if (it == leads.end())
                break;
            const int target = it->second.front();
            it->second.pop_front();
            if (it->second.empty())
                leads.erase(it);
            loop.push_back(current);
            current = target;
        } while (current != start);
        loops.push_back(std::move(loop));
    }
    return loops;
}

std::vector<uint32_t> tessellate(std::vector<Vertex> &vertices, const std::vector<std::vector<uint32_t>> &contours) {
    Tessellation t{&vertices, {}};
    std::vector<std::array<GLdouble, 3>> coordinates(vertices.size());
    for (size_t i = 0; i < vertices.size(); i++)
        coordinates[i] = {vertices[i].position.x, vertices[i].position.y, 0.0};
    GLUtesselator *tess = gluNewTess();
    if (!tess)
        return {};
    gluTessCallback(tess, GLU_TESS_VERTEX_DATA, reinterpret_cast<void(CALLBACK *)()>(tessVertex));
    gluTessCallback(tess, GLU_TESS_COMBINE_DATA, reinterpret_cast<void(CALLBACK *)()>(tessCombine));
    gluTessCallback(tess, GLU_TESS_EDGE_FLAG, reinterpret_cast<void(CALLBACK *)()>(tessEdgeFlag));
    gluTessCallback(tess, GLU_TESS_ERROR_DATA, reinterpret_cast<void(CALLBACK *)()>(tessError));
    gluTessProperty(tess, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
    gluTessBeginPolygon(tess, &t);
    for (const std::vector<uint32_t> &contour : contours) {
        gluTessBeginContour(tess);
        for (uint32_t i : contour)
            if (i < coordinates.size())
                gluTessVertex(tess, coordinates[i].data(), (void *)(uintptr_t)(i + 1));
        gluTessEndContour(tess);
    }
    gluTessEndPolygon(tess);
    gluDeleteTess(tess);
    t.indices.resize(t.indices.size() / 3 * 3);
    return t.indices;
}

glm::vec4 textured(glm::vec4 c, glm::vec4 s, const TextureLook &look) {
    glm::vec3 rgb(s);
    if (look.saturation != 1.0f) {
        const float grey = 0.3f * rgb.r + 0.59f * rgb.g + 0.11f * rgb.b;
        rgb += (1.0f - look.saturation) * (glm::vec3(grey) - rgb);
    }
    if (look.brightness != 1.0f)
        rgb *= look.brightness;
    if (look.contrast != 1.0f)
        rgb += (1.0f - look.contrast) * (glm::vec3(0.5f) - rgb);
    s = glm::vec4(glm::min(rgb, glm::vec3(1)), s.a);
    return c + look.opacity * (c * s - c);
}

glm::vec4 masked(glm::vec4 c, glm::vec4 m, int mode) {
    if (m.a > 0.0f)
        m = glm::vec4(glm::vec3(m) / m.a, m.a);
    switch (mode) {
    case 1:
        return glm::vec4(glm::mix(glm::vec3(c), glm::vec3(m), m.a), c.a);
    case 2:
        return c * m;
    case 3:
        return glm::vec4(glm::vec3(c), c.a * m.a);
    default:
        return c;
    }
}

} // namespace plasma
