#include <SDL.h>
#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <GLES2/gl2.h>
#define EXPORT_FN EMSCRIPTEN_KEEPALIVE
#else
#define GL_GLEXT_PROTOTYPES 1
#include <GL/gl.h>
#include <GL/glext.h>
#define EXPORT_FN
#endif

#include <vector>
#include <deque>
#include <cmath>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <algorithm>

// -------------------------------------------------------------
// Vector & Matrix Math (Column-Major 4x4 for OpenGL ES 2.0)
// -------------------------------------------------------------
const float PI_F = 3.14159265358979323846f;
const float DEG2RAD_F = PI_F / 180.0f;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;

    Vec3 operator+(const Vec3& o) const { return {x + o.x, y + o.y, z + o.z}; }
    Vec3 operator-(const Vec3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vec3 operator*(float s) const { return {x * s, y * s, z * s}; }
};

inline float Vec3Dot(const Vec3& a, const Vec3& b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline Vec3 Vec3Cross(const Vec3& a, const Vec3& b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

inline Vec3 Vec3Normalize(const Vec3& v) {
    float len = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    if (len > 1e-6f) return { v.x / len, v.y / len, v.z / len };
    return {0, 0, 0};
}

struct Mat4 {
    float m[16]; // column-major: m[col*4 + row]

    static Mat4 Identity() {
        Mat4 r = {0};
        r.m[0] = 1.0f; r.m[5] = 1.0f; r.m[10] = 1.0f; r.m[15] = 1.0f;
        return r;
    }

    static Mat4 Translate(float tx, float ty, float tz) {
        Mat4 r = Identity();
        r.m[12] = tx;
        r.m[13] = ty;
        r.m[14] = tz;
        return r;
    }

    static Mat4 Rotate(const Vec3& axis, float angleRad) {
        Mat4 r = Identity();
        Vec3 a = Vec3Normalize(axis);
        float c = std::cos(angleRad);
        float s = std::sin(angleRad);
        float t = 1.0f - c;

        r.m[0] = t * a.x * a.x + c;
        r.m[1] = t * a.x * a.y + s * a.z;
        r.m[2] = t * a.x * a.z - s * a.y;

        r.m[4] = t * a.x * a.y - s * a.z;
        r.m[5] = t * a.y * a.y + c;
        r.m[6] = t * a.y * a.z + s * a.x;

        r.m[8]  = t * a.x * a.z + s * a.y;
        r.m[9]  = t * a.y * a.z - s * a.x;
        r.m[10] = t * a.z * a.z + c;

        return r;
    }

    static Mat4 Multiply(const Mat4& a, const Mat4& b) {
        Mat4 r = {0};
        for (int c = 0; c < 4; ++c) {
            for (int ro = 0; ro < 4; ++ro) {
                float sum = 0.0f;
                for (int k = 0; k < 4; ++k) {
                    sum += a.m[k * 4 + ro] * b.m[c * 4 + k];
                }
                r.m[c * 4 + ro] = sum;
            }
        }
        return r;
    }

    static Mat4 Perspective(float fovyRad, float aspect, float zNear, float zFar) {
        Mat4 r = {0};
        float tanHalfFovy = std::tan(fovyRad / 2.0f);
        r.m[0]  = 1.0f / (aspect * tanHalfFovy);
        r.m[5]  = 1.0f / tanHalfFovy;
        r.m[10] = -(zFar + zNear) / (zFar - zNear);
        r.m[11] = -1.0f;
        r.m[14] = -(2.0f * zFar * zNear) / (zFar - zNear);
        return r;
    }

    static Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
        Vec3 f = Vec3Normalize(center - eye);
        Vec3 s = Vec3Normalize(Vec3Cross(f, up));
        Vec3 u = Vec3Cross(s, f);

        Mat4 r = Identity();
        r.m[0] = s.x;  r.m[4] = s.y;  r.m[8]  = s.z;  r.m[12] = -Vec3Dot(s, eye);
        r.m[1] = u.x;  r.m[5] = u.y;  r.m[9]  = u.z;  r.m[13] = -Vec3Dot(u, eye);
        r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z; r.m[14] =  Vec3Dot(f, eye);
        return r;
    }

    static Vec3 TransformPoint(const Mat4& m, const Vec3& p) {
        float x = m.m[0]*p.x + m.m[4]*p.y + m.m[8]*p.z  + m.m[12];
        float y = m.m[1]*p.x + m.m[5]*p.y + m.m[9]*p.z  + m.m[13];
        float z = m.m[2]*p.x + m.m[6]*p.y + m.m[10]*p.z + m.m[14];
        float w = m.m[3]*p.x + m.m[7]*p.y + m.m[11]*p.z + m.m[15];
        if (std::abs(w) > 1e-6f) {
            return { x / w, y / w, z / w };
        }
        return { x, y, z };
    }
};

Vec2 Project3DToScreen(const Vec3& pt, const Mat4& vp, int w, int h) {
    float x = vp.m[0]*pt.x + vp.m[4]*pt.y + vp.m[8]*pt.z + vp.m[12];
    float y = vp.m[1]*pt.x + vp.m[5]*pt.y + vp.m[9]*pt.z + vp.m[13];
    float w_clip = vp.m[3]*pt.x + vp.m[7]*pt.y + vp.m[11]*pt.z + vp.m[15];
    if (std::abs(w_clip) > 1e-6f) {
        x /= w_clip;
        y /= w_clip;
    }
    float sx = (x * 0.5f + 0.5f) * (float)w;
    float sy = (1.0f - (y * 0.5f + 0.5f)) * (float)h;
    return { sx, sy };
}

// -------------------------------------------------------------
// Shaders & GPU Resources
// -------------------------------------------------------------
const char* VS_SOURCE = 
    "attribute vec3 aPosition;\n"
    "attribute vec3 aNormal;\n"
    "attribute vec2 aTexCoord;\n"
    "uniform mat4 uMVP;\n"
    "uniform mat4 uModel;\n"
    "varying vec3 vNormal;\n"
    "varying vec2 vTexCoord;\n"
    "void main() {\n"
    "    vTexCoord = aTexCoord;\n"
    "    vNormal = normalize(mat3(uModel[0].xyz, uModel[1].xyz, uModel[2].xyz) * aNormal);\n"
    "    gl_Position = uMVP * vec4(aPosition, 1.0);\n"
    "}\n";

const char* FS_SOURCE = 
    "#ifdef GL_ES\n"
    "precision mediump float;\n"
    "#endif\n"
    "varying vec3 vNormal;\n"
    "varying vec2 vTexCoord;\n"
    "uniform sampler2D uTexture;\n"
    "void main() {\n"
    "    vec3 lightDir = normalize(vec3(0.4, 0.85, 0.6));\n"
    "    float diff = max(dot(vNormal, lightDir), 0.0);\n"
    "    vec3 ambient = vec3(0.72);\n"
    "    vec3 diffuse = vec3(0.28) * diff;\n"
    "    vec3 lighting = ambient + diffuse;\n"
    "    vec4 tex = texture2D(uTexture, vTexCoord);\n"
    "    gl_FragColor = vec4(tex.rgb * lighting, tex.a);\n"
    "}\n";

GLuint gProgram = 0;
GLint gLocPosition = -1;
GLint gLocNormal = -1;
GLint gLocTexCoord = -1;
GLint gLocMVP = -1;
GLint gLocModel = -1;
GLint gLocTexture = -1;

GLuint gVBO = 0;
GLuint gIBO = 0;
GLuint gTextures[7]; // 0:White, 1:Yellow, 2:Orange, 3:Red, 4:Green, 5:Blue, 6:Black

struct Vertex {
    float pos[3];
    float norm[3];
    float uv[2];
};

const float CUBIE_S = 0.492f;

Vertex gCubeVertices[24] = {
    // 0: +Y (UP)
    { {-CUBIE_S,  CUBIE_S, -CUBIE_S}, { 0, 1, 0}, {0, 0} },
    { {-CUBIE_S,  CUBIE_S,  CUBIE_S}, { 0, 1, 0}, {0, 1} },
    { { CUBIE_S,  CUBIE_S,  CUBIE_S}, { 0, 1, 0}, {1, 1} },
    { { CUBIE_S,  CUBIE_S, -CUBIE_S}, { 0, 1, 0}, {1, 0} },

    // 1: -Y (DOWN)
    { {-CUBIE_S, -CUBIE_S,  CUBIE_S}, { 0,-1, 0}, {0, 0} },
    { {-CUBIE_S, -CUBIE_S, -CUBIE_S}, { 0,-1, 0}, {0, 1} },
    { { CUBIE_S, -CUBIE_S, -CUBIE_S}, { 0,-1, 0}, {1, 1} },
    { { CUBIE_S, -CUBIE_S,  CUBIE_S}, { 0,-1, 0}, {1, 0} },

    // 2: -X (LEFT)
    { {-CUBIE_S,  CUBIE_S, -CUBIE_S}, {-1, 0, 0}, {0, 0} },
    { {-CUBIE_S, -CUBIE_S, -CUBIE_S}, {-1, 0, 0}, {0, 1} },
    { {-CUBIE_S, -CUBIE_S,  CUBIE_S}, {-1, 0, 0}, {1, 1} },
    { {-CUBIE_S,  CUBIE_S,  CUBIE_S}, {-1, 0, 0}, {1, 0} },

    // 3: +X (RIGHT)
    { { CUBIE_S,  CUBIE_S,  CUBIE_S}, { 1, 0, 0}, {0, 0} },
    { { CUBIE_S, -CUBIE_S,  CUBIE_S}, { 1, 0, 0}, {0, 1} },
    { { CUBIE_S, -CUBIE_S, -CUBIE_S}, { 1, 0, 0}, {1, 1} },
    { { CUBIE_S,  CUBIE_S, -CUBIE_S}, { 1, 0, 0}, {1, 0} },

    // 4: +Z (FRONT)
    { {-CUBIE_S,  CUBIE_S,  CUBIE_S}, { 0, 0, 1}, {0, 0} },
    { {-CUBIE_S, -CUBIE_S,  CUBIE_S}, { 0, 0, 1}, {0, 1} },
    { { CUBIE_S, -CUBIE_S,  CUBIE_S}, { 0, 0, 1}, {1, 1} },
    { { CUBIE_S,  CUBIE_S,  CUBIE_S}, { 0, 0, 1}, {1, 0} },

    // 5: -Z (BACK)
    { { CUBIE_S,  CUBIE_S, -CUBIE_S}, { 0, 0,-1}, {0, 0} },
    { { CUBIE_S, -CUBIE_S, -CUBIE_S}, { 0, 0,-1}, {0, 1} },
    { {-CUBIE_S, -CUBIE_S, -CUBIE_S}, { 0, 0,-1}, {1, 1} },
    { {-CUBIE_S,  CUBIE_S, -CUBIE_S}, { 0, 0,-1}, {1, 0} }
};

unsigned short gCubeIndices[36] = {
    0,1,2,  0,2,3,       // UP
    4,5,6,  4,6,7,       // DOWN
    8,9,10, 8,10,11,     // LEFT
    12,13,14, 12,14,15,  // RIGHT
    16,17,18, 16,18,19,  // FRONT
    20,21,22, 20,22,23   // BACK
};

struct RGBA { unsigned char r, g, b, a; };

const RGBA COLOR_PALETTE[6] = {
    { 255, 255, 255, 255 }, // 0: White (U)
    { 255, 215,   0, 255 }, // 1: Yellow (D)
    { 255, 105,   0, 255 }, // 2: Orange (L)
    { 220,  20,  25, 255 }, // 3: Red (R)
    {   0, 165,  65, 255 }, // 4: Green (F)
    {  10,  90, 225, 255 }  // 5: Blue (B)
};
const RGBA COLOR_CORE = { 18, 18, 20, 255 }; // Matte Black Core

void CreateProceduralTextures() {
    const int TEX_SIZE = 256;
    std::vector<RGBA> pixels(TEX_SIZE * TEX_SIZE);

    for (int t = 0; t < 6; ++t) {
        RGBA stickerCol = COLOR_PALETTE[t];
        int margin = 14;
        int cornerRadius = 24;
        int minPos = margin;
        int maxPos = TEX_SIZE - 1 - margin;

        for (int y = 0; y < TEX_SIZE; ++y) {
            for (int x = 0; x < TEX_SIZE; ++x) {
                bool inside = true;
                if (x < minPos || x > maxPos || y < minPos || y > maxPos) {
                    inside = false;
                } else {
                    int cx = (x < minPos + cornerRadius) ? (minPos + cornerRadius) :
                             (x > maxPos - cornerRadius) ? (maxPos - cornerRadius) : x;
                    int cy = (y < minPos + cornerRadius) ? (minPos + cornerRadius) :
                             (y > maxPos - cornerRadius) ? (maxPos - cornerRadius) : y;
                    int dx = x - cx;
                    int dy = y - cy;
                    if (dx * dx + dy * dy > cornerRadius * cornerRadius) {
                        inside = false;
                    }
                }

                pixels[y * TEX_SIZE + x] = inside ? stickerCol : COLOR_CORE;
            }
        }

        glGenTextures(1, &gTextures[t]);
        glBindTexture(GL_TEXTURE_2D, gTextures[t]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    for (int i = 0; i < TEX_SIZE * TEX_SIZE; ++i) {
        pixels[i] = COLOR_CORE;
    }
    glGenTextures(1, &gTextures[6]);
    glBindTexture(GL_TEXTURE_2D, gTextures[6]);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, TEX_SIZE, TEX_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

// -------------------------------------------------------------
// Rubik's Cube Model & State
// -------------------------------------------------------------
struct Cubie {
    Vec3 logicalPos;
    Mat4 baseTransform;
    int texIndices[6]; // 0:U, 1:D, 2:L, 3:R, 4:F, 5:B
};

struct Move {
    int axis;     // 0=X, 1=Y, 2=Z
    int slice;    // -1, 0, 1
    float angle;  // radians
    float speed;
};

std::vector<Cubie> gCubies;
std::deque<Move> gMoveQueue;

bool gIsAnimating = false;
float gAnimProgress = 0.0f;
float gAnimTarget = 0.0f;
float gCurrentAnimSpeed = 8.0f;
Vec3 gAnimAxis = {0,0,0};
int gAnimSliceAxis = 0;
int gAnimSliceValue = 0;

void InitCubeModel() {
    gCubies.clear();
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                Cubie c;
                c.logicalPos = {(float)x, (float)y, (float)z};
                c.baseTransform = Mat4::Translate((float)x, (float)y, (float)z);

                c.texIndices[0] = (y ==  1) ? 0 : 6; // UP (+Y)
                c.texIndices[1] = (y == -1) ? 1 : 6; // DOWN (-Y)
                c.texIndices[2] = (x == -1) ? 2 : 6; // LEFT (-X)
                c.texIndices[3] = (x ==  1) ? 3 : 6; // RIGHT (+X)
                c.texIndices[4] = (z ==  1) ? 4 : 6; // FRONT (+Z)
                c.texIndices[5] = (z == -1) ? 5 : 6; // BACK (-Z)

                gCubies.push_back(c);
            }
        }
    }
}

void StartRotation(int axis, int slice, float angle, float speed = 8.0f) {
    if (gIsAnimating) return;
    gIsAnimating = true;
    gAnimProgress = 0.0f;
    gAnimTarget = angle;
    gCurrentAnimSpeed = speed;
    gAnimSliceAxis = axis;
    gAnimSliceValue = slice;
    gAnimAxis = (axis == 0) ? Vec3{1,0,0} : ((axis == 1) ? Vec3{0,1,0} : Vec3{0,0,1});
}

void ScrambleCube() {
    gMoveQueue.clear();
    for (int i = 0; i < 22; ++i) {
        Move m;
        m.axis = rand() % 3;
        m.slice = (rand() % 3) - 1; // -1, 0, or 1
        m.angle = ((rand() % 2 == 0) ? 90.0f : -90.0f) * DEG2RAD_F;
        m.speed = 18.0f;
        gMoveQueue.push_back(m);
    }
}

void ResetCube() {
    gMoveQueue.clear();
    gIsAnimating = false;
    InitCubeModel();
}

void TurnFace(int face, int dir) {
    // face: 0:U, 1:D, 2:L, 3:R, 4:F, 5:B
    // dir: 1 (CW) or -1 (CCW)
    Move m;
    float sign = (float)dir;
    if (face == 0) { // U (top: +Y, slice 1)
        m.axis = 1; m.slice = 1; m.angle = -sign * (PI_F / 2.0f);
    } else if (face == 1) { // D (bottom: -Y, slice -1)
        m.axis = 1; m.slice = -1; m.angle = sign * (PI_F / 2.0f);
    } else if (face == 2) { // L (left: -X, slice -1)
        m.axis = 0; m.slice = -1; m.angle = sign * (PI_F / 2.0f);
    } else if (face == 3) { // R (right: +X, slice 1)
        m.axis = 0; m.slice = 1; m.angle = -sign * (PI_F / 2.0f);
    } else if (face == 4) { // F (front: +Z, slice 1)
        m.axis = 2; m.slice = 1; m.angle = -sign * (PI_F / 2.0f);
    } else if (face == 5) { // B (back: -Z, slice -1)
        m.axis = 2; m.slice = -1; m.angle = sign * (PI_F / 2.0f);
    }
    m.speed = 9.0f;
    gMoveQueue.push_back(m);
}

// -------------------------------------------------------------
// Interactive Raycasting & Pointer Controls
// -------------------------------------------------------------
SDL_Window* gWindow = nullptr;
SDL_GLContext gGLContext = nullptr;

float gCamAngleX = 45.0f * DEG2RAD_F;
float gCamAngleY = 28.0f * DEG2RAD_F;
float gCamRadius = 8.5f;

bool gIsPointerDown = false;
bool gIsSwipingCube = false;
Vec2 gTouchStartScreen = {0, 0};
Vec2 gLastPointerPos = {0, 0};

Vec3 gSwipeStartHitPt = {0, 0, 0};
int gSwipeStartFace = -1; // 0:U, 1:D, 2:L, 3:R, 4:F, 5:B
struct IntVec3 { int x, y, z; };
IntVec3 gTouchedCubie = {0, 0, 0};

void RotateView(int dir) {
    // 0: Left, 1: Right, 2: Up, 3: Down
    if (dir == 0) gCamAngleX -= 45.0f * DEG2RAD_F;
    else if (dir == 1) gCamAngleX += 45.0f * DEG2RAD_F;
    else if (dir == 2) {
        gCamAngleY += 25.0f * DEG2RAD_F;
        if (gCamAngleY > 80.0f * DEG2RAD_F) gCamAngleY = 80.0f * DEG2RAD_F;
    } else if (dir == 3) {
        gCamAngleY -= 25.0f * DEG2RAD_F;
        if (gCamAngleY < -80.0f * DEG2RAD_F) gCamAngleY = -80.0f * DEG2RAD_F;
    }
}

extern "C" {
    EXPORT_FN void scramble_cube() {
        ScrambleCube();
    }
    EXPORT_FN void reset_cube() {
        ResetCube();
    }
    EXPORT_FN void turn_face(int face, int dir) {
        TurnFace(face, dir);
    }
    EXPORT_FN void rotate_view(int dir) {
        RotateView(dir);
    }
}

struct Ray {
    Vec3 origin;
    Vec3 dir;
};

Ray GetPointerRay(float screenX, float screenY, int screenW, int screenH, const Mat4& proj, const Mat4& view) {
    (void)proj;
    (void)view;
    float ndcX = (2.0f * screenX) / (float)(screenW > 0 ? screenW : 1) - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / (float)(screenH > 0 ? screenH : 1);

    Vec3 eye = {
        gCamRadius * std::cos(gCamAngleY) * std::sin(gCamAngleX),
        gCamRadius * std::sin(gCamAngleY),
        gCamRadius * std::cos(gCamAngleY) * std::cos(gCamAngleX)
    };

    Vec3 forward = Vec3Normalize({ -eye.x, -eye.y, -eye.z });
    Vec3 right = Vec3Normalize(Vec3Cross(forward, {0, 1, 0}));
    Vec3 up = Vec3Cross(right, forward);

    float fovFactor = std::tan(35.0f * DEG2RAD_F * 0.5f);
    float aspect = (float)screenW / (float)(screenH > 0 ? screenH : 1);

    Vec3 rayDir = Vec3Normalize(forward + right * (ndcX * aspect * fovFactor) + up * (ndcY * fovFactor));
    return { eye, rayDir };
}

bool RayIntersectAABB(const Ray& ray, const Vec3& bmin, const Vec3& bmax, float& tOut, Vec3& ptOut) {
    float tmin = (bmin.x - ray.origin.x) / (std::abs(ray.dir.x) > 1e-6f ? ray.dir.x : 1e-6f);
    float tmax = (bmax.x - ray.origin.x) / (std::abs(ray.dir.x) > 1e-6f ? ray.dir.x : 1e-6f);
    if (tmin > tmax) std::swap(tmin, tmax);

    float tymin = (bmin.y - ray.origin.y) / (std::abs(ray.dir.y) > 1e-6f ? ray.dir.y : 1e-6f);
    float tymax = (bmax.y - ray.origin.y) / (std::abs(ray.dir.y) > 1e-6f ? ray.dir.y : 1e-6f);
    if (tymin > tymax) std::swap(tymin, tymax);

    if ((tmin > tymax) || (tymin > tmax)) return false;
    if (tymin > tmin) tmin = tymin;
    if (tymax < tmax) tmax = tymax;

    float tzmin = (bmin.z - ray.origin.z) / (std::abs(ray.dir.z) > 1e-6f ? ray.dir.z : 1e-6f);
    float tzmax = (bmax.z - ray.origin.z) / (std::abs(ray.dir.z) > 1e-6f ? ray.dir.z : 1e-6f);
    if (tzmin > tzmax) std::swap(tzmin, tzmax);

    if ((tmin > tzmax) || (tzmin > tmax)) return false;
    if (tzmin > tmin) tmin = tzmin;
    if (tzmax < tmax) tmax = tzmax;

    if (tmin < 0) return false;
    tOut = tmin;
    ptOut = ray.origin + ray.dir * tmin;
    return true;
}

void HandlePointerDown(float px, float py, int screenW, int screenH) {
    gIsPointerDown = true;
    gTouchStartScreen = {px, py};
    gLastPointerPos = {px, py};

    float aspect = (float)screenW / (float)(screenH > 0 ? screenH : 1);
    float baseRadius = 8.5f;
    gCamRadius = (aspect < 1.0f) ? (baseRadius / (aspect * 1.05f)) : baseRadius;

    Mat4 proj = Mat4::Perspective(35.0f * DEG2RAD_F, aspect, 0.1f, 50.0f);
    Vec3 eye = {
        gCamRadius * std::cos(gCamAngleY) * std::sin(gCamAngleX),
        gCamRadius * std::sin(gCamAngleY),
        gCamRadius * std::cos(gCamAngleY) * std::cos(gCamAngleX)
    };
    Mat4 view = Mat4::LookAt(eye, {0, 0, 0}, {0, 1, 0});
    Ray ray = GetPointerRay(px, py, screenW, screenH, proj, view);

    float tHit;
    Vec3 hitPt;
    if (RayIntersectAABB(ray, {-1.52f, -1.52f, -1.52f}, {1.52f, 1.52f, 1.52f}, tHit, hitPt)) {
        gIsSwipingCube = true;
        gSwipeStartHitPt = hitPt;

        // Identify which of the 6 faces was touched
        float distU = std::abs(hitPt.y - 1.5f);
        float distD = std::abs(hitPt.y - (-1.5f));
        float distL = std::abs(hitPt.x - (-1.5f));
        float distR = std::abs(hitPt.x - 1.5f);
        float distF = std::abs(hitPt.z - 1.5f);
        float distB = std::abs(hitPt.z - (-1.5f));

        float minDist = distU;
        gSwipeStartFace = 0; // U
        if (distD < minDist) { minDist = distD; gSwipeStartFace = 1; }
        if (distL < minDist) { minDist = distL; gSwipeStartFace = 2; }
        if (distR < minDist) { minDist = distR; gSwipeStartFace = 3; }
        if (distF < minDist) { minDist = distF; gSwipeStartFace = 4; }
        if (distB < minDist) { minDist = distB; gSwipeStartFace = 5; }

        // Find cubie discrete coordinate (-1, 0, or 1)
        int cx = (hitPt.x < -0.5f) ? -1 : ((hitPt.x > 0.5f) ? 1 : 0);
        int cy = (hitPt.y < -0.5f) ? -1 : ((hitPt.y > 0.5f) ? 1 : 0);
        int cz = (hitPt.z < -0.5f) ? -1 : ((hitPt.z > 0.5f) ? 1 : 0);
        gTouchedCubie = {cx, cy, cz};
    } else {
        gIsSwipingCube = false;
    }
}

void HandlePointerMove(float px, float py, int screenW, int screenH) {
    if (!gIsPointerDown) return;
    float dx = px - gLastPointerPos.x;
    float dy = py - gLastPointerPos.y;

    if (gIsSwipingCube && !gIsAnimating && gMoveQueue.empty()) {
        float totalDx = px - gTouchStartScreen.x;
        float totalDy = py - gTouchStartScreen.y;
        float dragDistSq = totalDx * totalDx + totalDy * totalDy;

        // When pointer moves at least ~16 screen pixels from start
        if (dragDistSq >= 256.0f) {
            float aspect = (float)screenW / (float)(screenH > 0 ? screenH : 1);
            float baseRadius = 8.5f;
            float camRad = (aspect < 1.0f) ? (baseRadius / (aspect * 1.05f)) : baseRadius;
            Mat4 proj = Mat4::Perspective(35.0f * DEG2RAD_F, aspect, 0.1f, 50.0f);
            Vec3 eye = {
                camRad * std::cos(gCamAngleY) * std::sin(gCamAngleX),
                camRad * std::sin(gCamAngleY),
                camRad * std::cos(gCamAngleY) * std::cos(gCamAngleX)
            };
            Mat4 view = Mat4::LookAt(eye, {0, 0, 0}, {0, 1, 0});
            Mat4 viewProj = Mat4::Multiply(proj, view);

            // Project 3D Face Tangents to Screen Space
            Vec3 tangA = {0,0,0};
            Vec3 tangB = {0,0,0};

            if (gSwipeStartFace == 0 || gSwipeStartFace == 1) { // UP (+Y) / DOWN (-Y)
                tangA = {1.0f, 0.0f, 0.0f}; // +X
                tangB = {0.0f, 0.0f, 1.0f}; // +Z
            } else if (gSwipeStartFace == 2 || gSwipeStartFace == 3) { // LEFT (-X) / RIGHT (+X)
                tangA = {0.0f, 1.0f, 0.0f}; // +Y
                tangB = {0.0f, 0.0f, 1.0f}; // +Z
            } else if (gSwipeStartFace == 4 || gSwipeStartFace == 5) { // FRONT (+Z) / BACK (-Z)
                tangA = {1.0f, 0.0f, 0.0f}; // +X
                tangB = {0.0f, 1.0f, 0.0f}; // +Y
            }

            Vec2 scrOrig = Project3DToScreen(gSwipeStartHitPt, viewProj, screenW, screenH);
            Vec2 scrA    = Project3DToScreen(gSwipeStartHitPt + tangA * 0.5f, viewProj, screenW, screenH);
            Vec2 scrB    = Project3DToScreen(gSwipeStartHitPt + tangB * 0.5f, viewProj, screenW, screenH);

            Vec2 dirA = { scrA.x - scrOrig.x, scrA.y - scrOrig.y };
            Vec2 dirB = { scrB.x - scrOrig.x, scrB.y - scrOrig.y };

            float lenA = std::sqrt(dirA.x * dirA.x + dirA.y * dirA.y);
            float lenB = std::sqrt(dirB.x * dirB.x + dirB.y * dirB.y);
            if (lenA < 1e-4f) lenA = 1.0f;
            if (lenB < 1e-4f) lenB = 1.0f;

            float dotA = (totalDx * dirA.x + totalDy * dirA.y) / lenA;
            float dotB = (totalDx * dirB.x + totalDy * dirB.y) / lenB;

            int axis = 0;
            int slice = 0;
            float angle = 0.0f;

            if (std::abs(dotA) >= std::abs(dotB)) {
                // Dominant movement along Tangent A
                float sign = (dotA > 0.0f) ? 1.0f : -1.0f;

                if (gSwipeStartFace == 4) { // FRONT (+Z): tangA is +X -> turns around Y axis
                    axis = 1; slice = gTouchedCubie.y; angle = sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 5) { // BACK (-Z): tangA is +X -> turns around Y axis
                    axis = 1; slice = gTouchedCubie.y; angle = -sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 0) { // UP (+Y): tangA is +X -> turns around Z axis
                    axis = 2; slice = gTouchedCubie.z; angle = -sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 1) { // DOWN (-Y): tangA is +X -> turns around Z axis
                    axis = 2; slice = gTouchedCubie.z; angle = sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 3) { // RIGHT (+X): tangA is +Y -> turns around Z axis
                    axis = 2; slice = gTouchedCubie.z; angle = sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 2) { // LEFT (-X): tangA is +Y -> turns around Z axis
                    axis = 2; slice = gTouchedCubie.z; angle = -sign * (PI_F / 2.0f);
                }
            } else {
                // Dominant movement along Tangent B
                float sign = (dotB > 0.0f) ? 1.0f : -1.0f;

                if (gSwipeStartFace == 4) { // FRONT (+Z): tangB is +Y -> turns around X axis
                    axis = 0; slice = gTouchedCubie.x; angle = -sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 5) { // BACK (-Z): tangB is +Y -> turns around X axis
                    axis = 0; slice = gTouchedCubie.x; angle = sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 0) { // UP (+Y): tangB is +Z -> turns around X axis
                    axis = 0; slice = gTouchedCubie.x; angle = sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 1) { // DOWN (-Y): tangB is +Z -> turns around X axis
                    axis = 0; slice = gTouchedCubie.x; angle = -sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 3) { // RIGHT (+X): tangB is +Z -> turns around Y axis
                    axis = 1; slice = gTouchedCubie.y; angle = -sign * (PI_F / 2.0f);
                } else if (gSwipeStartFace == 2) { // LEFT (-X): tangB is +Z -> turns around Y axis
                    axis = 1; slice = gTouchedCubie.y; angle = sign * (PI_F / 2.0f);
                }
            }

            StartRotation(axis, slice, angle, 9.0f);
            gIsSwipingCube = false; // Swipe action consumed
        }
    } else if (!gIsSwipingCube) {
        // Smooth camera orbit
        gCamAngleX -= dx * 0.007f;
        gCamAngleY += dy * 0.007f;
        if (gCamAngleY > 80.0f * DEG2RAD_F) gCamAngleY = 80.0f * DEG2RAD_F;
        if (gCamAngleY < -80.0f * DEG2RAD_F) gCamAngleY = -80.0f * DEG2RAD_F;
    }

    gLastPointerPos = {px, py};
}

void HandlePointerUp() {
    gIsPointerDown = false;
    gIsSwipingCube = false;
}

// -------------------------------------------------------------
// Main Loop & Frame Rendering
// -------------------------------------------------------------
Uint32 gLastTime = 0;

void RenderFrame() {
    int w, h;
    SDL_GL_GetDrawableSize(gWindow, &w, &h);
    if (w <= 0 || h <= 0) {
        w = 800; h = 800;
    }
    glViewport(0, 0, w, h);

    Uint32 now = SDL_GetTicks();
    float dt = (now - gLastTime) / 1000.0f;
    gLastTime = now;
    if (dt > 0.05f) dt = 0.05f;

    // Process move queue
    if (!gIsAnimating && !gMoveQueue.empty()) {
        Move nextMove = gMoveQueue.front();
        gMoveQueue.pop_front();
        StartRotation(nextMove.axis, nextMove.slice, nextMove.angle, nextMove.speed);
    }

    // Advance animation
    if (gIsAnimating) {
        gAnimProgress += gCurrentAnimSpeed * dt;
        if (gAnimProgress >= 1.0f) {
            gAnimProgress = 1.0f;
            Mat4 finalRot = Mat4::Rotate(gAnimAxis, gAnimTarget);

            for (auto& c : gCubies) {
                float v = (gAnimSliceAxis == 0) ? c.logicalPos.x : ((gAnimSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
                if (std::abs(v - (float)gAnimSliceValue) < 0.1f) {
                    c.logicalPos = Mat4::TransformPoint(finalRot, c.logicalPos);
                    c.logicalPos.x = std::round(c.logicalPos.x);
                    c.logicalPos.y = std::round(c.logicalPos.y);
                    c.logicalPos.z = std::round(c.logicalPos.z);

                    c.baseTransform = Mat4::Multiply(finalRot, c.baseTransform);
                    c.baseTransform.m[12] = c.logicalPos.x;
                    c.baseTransform.m[13] = c.logicalPos.y;
                    c.baseTransform.m[14] = c.logicalPos.z;

                    for (int i = 0; i < 16; ++i) {
                        if (i != 12 && i != 13 && i != 14 && i != 15) {
                            if (std::abs(c.baseTransform.m[i]) < 0.05f) c.baseTransform.m[i] = 0.0f;
                            else if (c.baseTransform.m[i] > 0.95f) c.baseTransform.m[i] = 1.0f;
                            else if (c.baseTransform.m[i] < -0.95f) c.baseTransform.m[i] = -1.0f;
                        }
                    }
                }
            }
            gIsAnimating = false;
        }
    }

    // Camera calculation
    float aspect = (float)w / (float)h;
    float baseRadius = 8.5f;
    if (aspect < 1.0f) {
        gCamRadius = baseRadius / (aspect * 1.05f);
    } else {
        gCamRadius = baseRadius;
    }

    Vec3 eye = {
        gCamRadius * std::cos(gCamAngleY) * std::sin(gCamAngleX),
        gCamRadius * std::sin(gCamAngleY),
        gCamRadius * std::cos(gCamAngleY) * std::cos(gCamAngleX)
    };
    Mat4 proj = Mat4::Perspective(35.0f * DEG2RAD_F, aspect, 0.1f, 50.0f);
    Mat4 view = Mat4::LookAt(eye, {0, 0, 0}, {0, 1, 0});
    Mat4 viewProj = Mat4::Multiply(proj, view);

    glClearColor(0.04f, 0.04f, 0.045f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(gProgram);

    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glEnableVertexAttribArray(gLocPosition);
    glVertexAttribPointer(gLocPosition, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, pos));

    glEnableVertexAttribArray(gLocNormal);
    glVertexAttribPointer(gLocNormal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, norm));

    glEnableVertexAttribArray(gLocTexCoord);
    glVertexAttribPointer(gLocTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
    glActiveTexture(GL_TEXTURE0);
    glUniform1i(gLocTexture, 0);

    Mat4 animRot = Mat4::Identity();
    if (gIsAnimating) {
        animRot = Mat4::Rotate(gAnimAxis, gAnimTarget * gAnimProgress);
    }

    for (const auto& c : gCubies) {
        Mat4 model = c.baseTransform;
        if (gIsAnimating) {
            float v = (gAnimSliceAxis == 0) ? c.logicalPos.x : ((gAnimSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - (float)gAnimSliceValue) < 0.1f) {
                model = Mat4::Multiply(animRot, c.baseTransform);
            }
        }

        Mat4 mvp = Mat4::Multiply(viewProj, model);
        glUniformMatrix4fv(gLocMVP, 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(gLocModel, 1, GL_FALSE, model.m);

        for (int f = 0; f < 6; ++f) {
            glBindTexture(GL_TEXTURE_2D, gTextures[c.texIndices[f]]);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, (void*)(f * 6 * sizeof(unsigned short)));
        }
    }

    SDL_GL_SwapWindow(gWindow);
}

void ProcessEvents() {
    int screenW = 800, screenH = 800;
    SDL_GL_GetDrawableSize(gWindow, &screenW, &screenH);
    int winW = screenW, winH = screenH;
    SDL_GetWindowSize(gWindow, &winW, &winH);

    float scaleX = (winW > 0) ? ((float)screenW / (float)winW) : 1.0f;
    float scaleY = (winH > 0) ? ((float)screenH / (float)winH) : 1.0f;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
#if !defined(__EMSCRIPTEN__)
            exit(0);
#endif
        } else if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) {
            HandlePointerDown((float)e.button.x * scaleX, (float)e.button.y * scaleY, screenW, screenH);
        } else if (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) {
            HandlePointerUp();
        } else if (e.type == SDL_MOUSEMOTION && gIsPointerDown) {
            HandlePointerMove((float)e.motion.x * scaleX, (float)e.motion.y * scaleY, screenW, screenH);
        } else if (e.type == SDL_FINGERDOWN) {
            HandlePointerDown(e.tfinger.x * screenW, e.tfinger.y * screenH, screenW, screenH);
        } else if (e.type == SDL_FINGERUP) {
            HandlePointerUp();
        } else if (e.type == SDL_FINGERMOTION && gIsPointerDown) {
            HandlePointerMove(e.tfinger.x * screenW, e.tfinger.y * screenH, screenW, screenH);
        }
    }
}

void MainLoopCallback() {
    ProcessEvents();
    RenderFrame();
}

static GLuint CompileShader(GLenum type, const char* source) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &source, nullptr);
    glCompileShader(s);
    GLint status = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &status);
    if (!status) {
        char log[1024];
        glGetShaderInfoLog(s, sizeof(log), nullptr, log);
        printf("Shader compile error: %s\n", log);
    }
    return s;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init Error: %s\n", SDL_GetError());
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    int winWidth = 800;
    int winHeight = 800;
#if defined(__EMSCRIPTEN__)
    winWidth = EM_ASM_INT({ return window.innerWidth; });
    winHeight = EM_ASM_INT({ return window.innerHeight; });
#endif

    gWindow = SDL_CreateWindow(
        "Rubik's Cube 3D",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        winWidth, winHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!gWindow) {
        printf("SDL_CreateWindow Error: %s\n", SDL_GetError());
        return 1;
    }

    gGLContext = SDL_GL_CreateContext(gWindow);
    if (!gGLContext) {
        printf("SDL_GL_CreateContext Error: %s\n", SDL_GetError());
        return 1;
    }

    GLuint vs = CompileShader(GL_VERTEX_SHADER, VS_SOURCE);
    GLuint fs = CompileShader(GL_FRAGMENT_SHADER, FS_SOURCE);

    gProgram = glCreateProgram();
    glAttachShader(gProgram, vs);
    glAttachShader(gProgram, fs);
    glLinkProgram(gProgram);

    GLint linkStatus = 0;
    glGetProgramiv(gProgram, GL_LINK_STATUS, &linkStatus);
    if (!linkStatus) {
        char log[1024];
        glGetProgramInfoLog(gProgram, sizeof(log), nullptr, log);
        printf("Program link error: %s\n", log);
    }

    gLocPosition = glGetAttribLocation(gProgram, "aPosition");
    gLocNormal   = glGetAttribLocation(gProgram, "aNormal");
    gLocTexCoord = glGetAttribLocation(gProgram, "aTexCoord");
    gLocMVP      = glGetUniformLocation(gProgram, "uMVP");
    gLocModel    = glGetUniformLocation(gProgram, "uModel");
    gLocTexture  = glGetUniformLocation(gProgram, "uTexture");

    glGenBuffers(1, &gVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gCubeVertices), gCubeVertices, GL_STATIC_DRAW);

    glGenBuffers(1, &gIBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(gCubeIndices), gCubeIndices, GL_STATIC_DRAW);

    CreateProceduralTextures();
    InitCubeModel();

    gLastTime = SDL_GetTicks();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(MainLoopCallback, 0, 1);
#else
    while (true) {
        MainLoopCallback();
        SDL_Delay(16);
    }
#endif

    return 0;
}
