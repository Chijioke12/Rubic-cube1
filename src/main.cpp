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

    static Mat4 Transpose3x3(const Mat4& a) {
        Mat4 r = Identity();
        r.m[0] = a.m[0]; r.m[1] = a.m[4]; r.m[2] = a.m[8];
        r.m[4] = a.m[1]; r.m[5] = a.m[5]; r.m[6] = a.m[9];
        r.m[8] = a.m[2]; r.m[9] = a.m[6]; r.m[10] = a.m[10];
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

    static Vec3 TransformVector3x3(const Mat4& m, const Vec3& v) {
        return {
            m.m[0]*v.x + m.m[4]*v.y + m.m[8]*v.z,
            m.m[1]*v.x + m.m[5]*v.y + m.m[9]*v.z,
            m.m[2]*v.x + m.m[6]*v.y + m.m[10]*v.z
        };
    }
};

inline void OrthonormalizeMatrix(Mat4& m) {
    Vec3 x = { m.m[0], m.m[1], m.m[2] };
    Vec3 y = { m.m[4], m.m[5], m.m[6] };
    x = Vec3Normalize(x);
    Vec3 z = Vec3Normalize(Vec3Cross(x, y));
    y = Vec3Normalize(Vec3Cross(z, x));
    m.m[0] = x.x; m.m[1] = x.y; m.m[2] = x.z;
    m.m[4] = y.x; m.m[5] = y.y; m.m[6] = y.z;
    m.m[8] = z.x; m.m[9] = z.y; m.m[10] = z.z;
}

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
    "    vec3 lightDir = normalize(vec3(0.35, 0.85, 0.7));\n"
    "    float diff = max(dot(vNormal, lightDir), 0.0);\n"
    "    vec3 ambient = vec3(0.72);\n"
    "    vec3 diffuse = vec3(0.32) * diff;\n"
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
    int axis;     // 0=X, 1=Y, 2=Z in Cube Local Space
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

// Entire Cube Orientation Matrix (Limitless 3D Free Rotation)
Mat4 gCubeOrientation = Mat4::Identity();
float gCamRadius = 8.5f;

void InitDefaultOrientation() {
    // Initial isometric view (30 deg pitch, 45 deg yaw)
    Mat4 rotX = Mat4::Rotate({1, 0, 0}, 28.0f * DEG2RAD_F);
    Mat4 rotY = Mat4::Rotate({0, 1, 0}, 45.0f * DEG2RAD_F);
    gCubeOrientation = Mat4::Multiply(rotX, rotY);
}

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

void StartRotation(int axis, int slice, float angle, float speed = 9.0f) {
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
    InitDefaultOrientation();
}

void ZoomCamera(float delta) {
    gCamRadius -= delta;
    if (gCamRadius < 4.5f) gCamRadius = 4.5f;
    if (gCamRadius > 20.0f) gCamRadius = 20.0f;
}

extern "C" {
    EXPORT_FN void scramble_cube() {
        ScrambleCube();
    }
    EXPORT_FN void reset_cube() {
        ResetCube();
    }
    EXPORT_FN void zoom_cube(float delta) {
        ZoomCamera(delta);
    }
}

// -------------------------------------------------------------
// Interactive Raycasting & Pointer Controls
// -------------------------------------------------------------
SDL_Window* gWindow = nullptr;
SDL_GLContext gGLContext = nullptr;

bool gIsPointerDown = false;
bool gTouchStartedOnCube = false;
Vec2 gTouchStartScreen = {0, 0};
Vec2 gLastPointerPos = {0, 0};

Vec3 gSwipeStartHitPtLocal = {0, 0, 0};
int gSwipeStartFace = -1; // 0:U, 1:D, 2:L, 3:R, 4:F, 5:B
struct IntVec3 { int x, y, z; };
IntVec3 gTouchedCubie = {0, 0, 0};

struct Ray {
    Vec3 origin;
    Vec3 dir;
};

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
    float ndcX = (2.0f * px) / (float)(screenW > 0 ? screenW : 1) - 1.0f;
    float ndcY = 1.0f - (2.0f * py) / (float)(screenH > 0 ? screenH : 1);

    float fovFactor = std::tan(35.0f * DEG2RAD_F * 0.5f);
    Vec3 rayDirCamera = Vec3Normalize({ ndcX * aspect * fovFactor, ndcY * fovFactor, -1.0f });
    Vec3 rayOriginCamera = { 0, 0, gCamRadius };

    // Transform camera ray into Cube Local Space
    Mat4 invCubeRot = Mat4::Transpose3x3(gCubeOrientation);
    Vec3 rayOriginLocal = Mat4::TransformPoint(invCubeRot, rayOriginCamera);
    Vec3 rayDirLocal = Vec3Normalize(Mat4::TransformVector3x3(invCubeRot, rayDirCamera));

    Ray rayLocal = { rayOriginLocal, rayDirLocal };

    float tHit = 0.0f;
    Vec3 hitPtLocal;
    // Bounding box of the 3x3 cube in local space is [-1.52, 1.52]
    if (RayIntersectAABB(rayLocal, {-1.52f, -1.52f, -1.52f}, {1.52f, 1.52f, 1.52f}, tHit, hitPtLocal)) {
        gTouchStartedOnCube = true;
        gSwipeStartHitPtLocal = hitPtLocal;

        // Identify which of the 6 faces was touched in local space
        float distU = std::abs(hitPtLocal.y - 1.5f);
        float distD = std::abs(hitPtLocal.y - (-1.5f));
        float distL = std::abs(hitPtLocal.x - (-1.5f));
        float distR = std::abs(hitPtLocal.x - 1.5f);
        float distF = std::abs(hitPtLocal.z - 1.5f);
        float distB = std::abs(hitPtLocal.z - (-1.5f));

        float minDist = distU;
        gSwipeStartFace = 0; // U (+Y)
        if (distD < minDist) { minDist = distD; gSwipeStartFace = 1; } // D (-Y)
        if (distL < minDist) { minDist = distL; gSwipeStartFace = 2; } // L (-X)
        if (distR < minDist) { minDist = distR; gSwipeStartFace = 3; } // R (+X)
        if (distF < minDist) { minDist = distF; gSwipeStartFace = 4; } // F (+Z)
        if (distB < minDist) { minDist = distB; gSwipeStartFace = 5; } // B (-Z)

        // Find touched cubie coordinates (-1, 0, or 1) in local space
        int cx = (hitPtLocal.x < -0.5f) ? -1 : ((hitPtLocal.x > 0.5f) ? 1 : 0);
        int cy = (hitPtLocal.y < -0.5f) ? -1 : ((hitPtLocal.y > 0.5f) ? 1 : 0);
        int cz = (hitPtLocal.z < -0.5f) ? -1 : ((hitPtLocal.z > 0.5f) ? 1 : 0);
        gTouchedCubie = {cx, cy, cz};
    } else {
        gTouchStartedOnCube = false;
    }
}

void HandlePointerMove(float px, float py, int screenW, int screenH) {
    if (!gIsPointerDown) return;
    float dx = px - gLastPointerPos.x;
    float dy = py - gLastPointerPos.y;

    if (gTouchStartedOnCube) {
        // We started touching the cube: this gesture is strictly for turning a face/slice!
        if (!gIsAnimating && gMoveQueue.empty()) {
            float totalDx = px - gTouchStartScreen.x;
            float totalDy = py - gTouchStartScreen.y;
            float dragDistSq = totalDx * totalDx + totalDy * totalDy;

            // When pointer moves at least ~14 screen pixels from start
            if (dragDistSq >= 196.0f) {
                float aspect = (float)screenW / (float)(screenH > 0 ? screenH : 1);
                Mat4 proj = Mat4::Perspective(35.0f * DEG2RAD_F, aspect, 0.1f, 50.0f);
                Mat4 view = Mat4::LookAt({0, 0, gCamRadius}, {0, 0, 0}, {0, 1, 0});
                Mat4 viewProj = Mat4::Multiply(proj, view);

                // Local face tangent vectors
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

                // Transform local points to world space with gCubeOrientation, then project to screen
                Vec3 worldOrig = Mat4::TransformPoint(gCubeOrientation, gSwipeStartHitPtLocal);
                Vec3 worldA    = Mat4::TransformPoint(gCubeOrientation, gSwipeStartHitPtLocal + tangA * 0.5f);
                Vec3 worldB    = Mat4::TransformPoint(gCubeOrientation, gSwipeStartHitPtLocal + tangB * 0.5f);

                Vec2 scrOrig = Project3DToScreen(worldOrig, viewProj, screenW, screenH);
                Vec2 scrA    = Project3DToScreen(worldA, viewProj, screenW, screenH);
                Vec2 scrB    = Project3DToScreen(worldB, viewProj, screenW, screenH);

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

                    if (gSwipeStartFace == 4) { // FRONT (+Z): tangA is +X -> turn around local Y
                        axis = 1; slice = gTouchedCubie.y; angle = sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 5) { // BACK (-Z): tangA is +X -> turn around local Y
                        axis = 1; slice = gTouchedCubie.y; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 0) { // UP (+Y): tangA is +X -> turn around local Z
                        axis = 2; slice = gTouchedCubie.z; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 1) { // DOWN (-Y): tangA is +X -> turn around local Z
                        axis = 2; slice = gTouchedCubie.z; angle = sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 3) { // RIGHT (+X): tangA is +Y -> turn around local Z
                        axis = 2; slice = gTouchedCubie.z; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 2) { // LEFT (-X): tangA is +Y -> turn around local Z
                        axis = 2; slice = gTouchedCubie.z; angle = sign * (PI_F / 2.0f);
                    }
                } else {
                    // Dominant movement along Tangent B
                    float sign = (dotB > 0.0f) ? 1.0f : -1.0f;

                    if (gSwipeStartFace == 4) { // FRONT (+Z): tangB is +Y -> turn around local X
                        axis = 0; slice = gTouchedCubie.x; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 5) { // BACK (-Z): tangB is +Y -> turn around local X
                        axis = 0; slice = gTouchedCubie.x; angle = sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 0) { // UP (+Y): tangB is +Z -> turn around local X
                        axis = 0; slice = gTouchedCubie.x; angle = sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 1) { // DOWN (-Y): tangB is +Z -> turn around local X
                        axis = 0; slice = gTouchedCubie.x; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 3) { // RIGHT (+X): tangB is +Z -> turn around local Y
                        axis = 1; slice = gTouchedCubie.y; angle = -sign * (PI_F / 2.0f);
                    } else if (gSwipeStartFace == 2) { // LEFT (-X): tangB is +Z -> turn around local Y
                        axis = 1; slice = gTouchedCubie.y; angle = sign * (PI_F / 2.0f);
                    }
                }

                StartRotation(axis, slice, angle, 9.0f);
                gTouchStartedOnCube = false; // Swipe consumed!
            }
        }
    } else {
        // Limitless, free 3D rotation of the entire cube in space (no gimbal lock, no bounds)
        Mat4 rotX = Mat4::Rotate({1.0f, 0.0f, 0.0f}, dy * 0.007f);
        Mat4 rotY = Mat4::Rotate({0.0f, 1.0f, 0.0f}, dx * 0.007f);
        Mat4 dRot = Mat4::Multiply(rotX, rotY);
        gCubeOrientation = Mat4::Multiply(dRot, gCubeOrientation);
        OrthonormalizeMatrix(gCubeOrientation);
    }

    gLastPointerPos = {px, py};
}

void HandlePointerUp() {
    gIsPointerDown = false;
    gTouchStartedOnCube = false;
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

    // Advance slice rotation animation
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

    // Camera setup
    float aspect = (float)w / (float)h;
    Mat4 proj = Mat4::Perspective(35.0f * DEG2RAD_F, aspect, 0.1f, 50.0f);
    Mat4 view = Mat4::LookAt({0, 0, gCamRadius}, {0, 0, 0}, {0, 1, 0});
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
        Mat4 cubieLocalModel = c.baseTransform;
        if (gIsAnimating) {
            float v = (gAnimSliceAxis == 0) ? c.logicalPos.x : ((gAnimSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - (float)gAnimSliceValue) < 0.1f) {
                cubieLocalModel = Mat4::Multiply(animRot, c.baseTransform);
            }
        }

        // Apply entire cube orientation
        Mat4 worldModel = Mat4::Multiply(gCubeOrientation, cubieLocalModel);
        Mat4 mvp = Mat4::Multiply(viewProj, worldModel);

        glUniformMatrix4fv(gLocMVP, 1, GL_FALSE, mvp.m);
        glUniformMatrix4fv(gLocModel, 1, GL_FALSE, worldModel.m);

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
        } else if (e.type == SDL_MOUSEWHEEL) {
            ZoomCamera((float)e.wheel.y * 0.6f);
        } else if (e.type == SDL_MULTIGESTURE && e.mgesture.numFingers >= 2) {
            ZoomCamera(e.mgesture.dDist * 16.0f);
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
    InitDefaultOrientation();

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
