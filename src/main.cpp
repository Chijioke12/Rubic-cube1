#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>
#include <deque>
#include <cmath>
#include <cstdlib>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#define EXPORT_FN EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT_FN
#endif

// Authentic, vibrant Rubik's Cube colors
const Color C_UP    = { 252, 252, 252, 255 }; // White
const Color C_DOWN  = { 255, 215,   0, 255 }; // Yellow
const Color C_LEFT  = { 255, 105,   0, 255 }; // Orange
const Color C_RIGHT = { 220,  20,  25, 255 }; // Red
const Color C_FRONT = {   0, 165,  65, 255 }; // Green
const Color C_BACK  = {  10,  90, 225, 255 }; // Blue
const Color C_INNER = {  18,  18,  20, 255 }; // Matte Black Cubie Body

struct Cubie {
    Vector3 logicalPos;
    Matrix transform;
    Color colors[6]; // 0: UP(+Y), 1: DOWN(-Y), 2: LEFT(-X), 3: RIGHT(+X), 4: FRONT(+Z), 5: BACK(-Z)
};

struct Move {
    int axis;     // 0=X, 1=Y, 2=Z
    int slice;    // -1, 0, 1
    float angle;  // radians
    float speed;  // animation speed multiplier
};

std::vector<Cubie> cubies;
std::deque<Move> moveQueue;

Camera camera = { 0 };
float camAngleX = 45.0f * DEG2RAD;
float camAngleY = 28.0f * DEG2RAD;
float camRadius = 8.5f;

bool isAnimating = false;
float animProgress = 0.0f;
float animTarget = 0.0f;
float currentAnimSpeed = 6.0f;
Vector3 animAxis = {0,0,0};
int animSliceAxis = 0;
int animSliceValue = 0;

Vector2 lastMousePos = {0};
bool isSwiping = false;
Vector3 swipeStartPos = {0};
int swipeStartFace = -1; // 0=U, 1=D, 2=L, 3=R, 4=F, 5=B

inline bool isStickerColor(const Color& c) {
    return (c.r != C_INNER.r || c.g != C_INNER.g || c.b != C_INNER.b);
}

void InitCube() {
    cubies.clear();
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Cubie c;
                c.logicalPos = {(float)x, (float)y, (float)z};
                c.transform = MatrixTranslate(x, y, z);
                
                c.colors[0] = (y ==  1) ? C_UP    : C_INNER;
                c.colors[1] = (y == -1) ? C_DOWN  : C_INNER;
                c.colors[2] = (x == -1) ? C_LEFT  : C_INNER;
                c.colors[3] = (x ==  1) ? C_RIGHT : C_INNER;
                c.colors[4] = (z ==  1) ? C_FRONT : C_INNER;
                c.colors[5] = (z == -1) ? C_BACK  : C_INNER;
                
                cubies.push_back(c);
            }
        }
    }
}

void DrawCubie(Cubie& c) {
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(c.transform));

    // 1. Solid Matte Black Core Cubie (0.94f width gives a clean 0.06f spacing gap between cubies)
    const float bodySize = 0.94f;
    DrawCube(Vector3{0, 0, 0}, bodySize, bodySize, bodySize, C_INNER);

    // 2. Solid Raised Plastic Colored Sticker Tiles
    const float stSize  = 0.82f;
    const float stThick = 0.03f;
    // Outer plane coordinate: 0.47f + 0.015f = 0.485f (safely under 0.50f cell limit to avoid overlap)
    const float stPos   = (bodySize / 2.0f) + (stThick / 2.0f);

    // UP (+Y)
    if (isStickerColor(c.colors[0])) {
        DrawCube(Vector3{0, stPos, 0}, stSize, stThick, stSize, c.colors[0]);
    }
    // DOWN (-Y)
    if (isStickerColor(c.colors[1])) {
        DrawCube(Vector3{0, -stPos, 0}, stSize, stThick, stSize, c.colors[1]);
    }
    // LEFT (-X)
    if (isStickerColor(c.colors[2])) {
        DrawCube(Vector3{-stPos, 0, 0}, stThick, stSize, stSize, c.colors[2]);
    }
    // RIGHT (+X)
    if (isStickerColor(c.colors[3])) {
        DrawCube(Vector3{stPos, 0, 0}, stThick, stSize, stSize, c.colors[3]);
    }
    // FRONT (+Z)
    if (isStickerColor(c.colors[4])) {
        DrawCube(Vector3{0, 0, stPos}, stSize, stSize, stThick, c.colors[4]);
    }
    // BACK (-Z)
    if (isStickerColor(c.colors[5])) {
        DrawCube(Vector3{0, 0, -stPos}, stSize, stSize, stThick, c.colors[5]);
    }

    rlPopMatrix();
}

void StartRotation(int axis, int slice, float angle, float speed = 6.0f) {
    if (isAnimating) return;
    isAnimating = true;
    animProgress = 0.0f;
    animTarget = angle;
    currentAnimSpeed = speed;
    animSliceAxis = axis;
    animSliceValue = slice;
    animAxis = (axis == 0) ? Vector3{1,0,0} : ((axis == 1) ? Vector3{0,1,0} : Vector3{0,0,1});
}

void ScrambleCube() {
    moveQueue.clear();
    for (int i = 0; i < 20; i++) {
        Move m;
        m.axis = rand() % 3;
        m.slice = (rand() % 3) - 1; // -1, 0, or 1
        m.angle = ((rand() % 2 == 0) ? 90.0f : -90.0f) * DEG2RAD;
        m.speed = 18.0f; // Fast, fluid scramble sequence
        moveQueue.push_back(m);
    }
}

extern "C" {
    EXPORT_FN void scramble_cube() {
        ScrambleCube();
    }
}

void UpdateDrawFrame(void) {
#if defined(__EMSCRIPTEN__)
    int w = EM_ASM_INT({ return window.innerWidth; });
    int h = EM_ASM_INT({ return window.innerHeight; });
    if (w > 0 && h > 0 && (w != GetScreenWidth() || h != GetScreenHeight())) {
        SetWindowSize(w, h);
    }
#endif

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    // Process queued scramble animations sequentially
    if (!isAnimating && !moveQueue.empty()) {
        Move nextMove = moveQueue.front();
        moveQueue.pop_front();
        StartRotation(nextMove.axis, nextMove.slice, nextMove.angle, nextMove.speed);
    }

    if (isAnimating) {
        float step = currentAnimSpeed * dt;
        if (animProgress + step >= 1.0f) {
            step = 1.0f - animProgress;
        }
        
        float frameAngle = animTarget * step;
        Matrix rot = MatrixRotate(animAxis, frameAngle);

        for (auto& c : cubies) {
            float v = (animSliceAxis == 0) ? c.logicalPos.x : ((animSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - animSliceValue) < 0.1f) {
                c.transform = MatrixMultiply(c.transform, rot);
            }
        }
        
        animProgress += step;
        
        if (animProgress >= 1.0f) {
            Matrix finalRot = MatrixRotate(animAxis, animTarget);
            for (auto& c : cubies) {
                float v = (animSliceAxis == 0) ? c.logicalPos.x : ((animSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
                if (std::abs(v - animSliceValue) < 0.1f) {
                    c.logicalPos = Vector3Transform(c.logicalPos, finalRot);
                    c.logicalPos.x = roundf(c.logicalPos.x);
                    c.logicalPos.y = roundf(c.logicalPos.y);
                    c.logicalPos.z = roundf(c.logicalPos.z);
                }
            }
            isAnimating = false;
        }
    }

    if (!isAnimating && moveQueue.empty()) {
        bool touchPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || (GetTouchPointCount() > 0 && IsGestureDetected(GESTURE_TAP));
        bool touchDown = IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0;
        
        if (touchDown) {
            Vector2 mousePos = GetMousePosition();
            Vector2 delta = { mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
            
            if (touchPressed) {
                Ray ray = GetMouseRay(mousePos, camera);
                RayCollision col = GetRayCollisionBox(ray, BoundingBox{Vector3{-1.5f, -1.5f, -1.5f}, Vector3{1.5f, 1.5f, 1.5f}});
                if (col.hit) {
                    isSwiping = true;
                    swipeStartPos = col.point;
                    if (fabs(col.point.y - 1.5f) < 0.1f) swipeStartFace = 0; // U
                    else if (fabs(col.point.y - (-1.5f)) < 0.1f) swipeStartFace = 1; // D
                    else if (fabs(col.point.x - (-1.5f)) < 0.1f) swipeStartFace = 2; // L
                    else if (fabs(col.point.x - 1.5f) < 0.1f) swipeStartFace = 3; // R
                    else if (fabs(col.point.z - 1.5f) < 0.1f) swipeStartFace = 4; // F
                    else if (fabs(col.point.z - (-1.5f)) < 0.1f) swipeStartFace = 5; // B
                } else {
                    isSwiping = false;
                }
            }

            if (isSwiping) {
                Ray ray = GetMouseRay(mousePos, camera);
                RayCollision col = GetRayCollisionBox(ray, BoundingBox{Vector3{-1.5f, -1.5f, -1.5f}, Vector3{1.5f, 1.5f, 1.5f}});
                if (col.hit) {
                    float dx = col.point.x - swipeStartPos.x;
                    float dy = col.point.y - swipeStartPos.y;
                    float dz = col.point.z - swipeStartPos.z;

                    float thresh = 0.28f;
                    if (fabs(dx) > thresh || fabs(dy) > thresh || fabs(dz) > thresh) {
                        int axis = 0; int slice = 0; float ang = PI/2;
                        
                        if (swipeStartFace == 4) { // Front
                            if (fabs(dx) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dx>0)?-PI/2:PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dy>0)?PI/2:-PI/2; }
                        } else if (swipeStartFace == 5) { // Back
                            if (fabs(dx) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dx>0)?PI/2:-PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dy>0)?-PI/2:PI/2; }
                        } else if (swipeStartFace == 0) { // Up
                            if (fabs(dx) > fabs(dz)) { axis = 2; slice = round(swipeStartPos.z); ang = (dx>0)?PI/2:-PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dz>0)?-PI/2:PI/2; }
                        } else if (swipeStartFace == 1) { // Down
                            if (fabs(dx) > fabs(dz)) { axis = 2; slice = round(swipeStartPos.z); ang = (dx>0)?-PI/2:PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dz>0)?-PI/2:PI/2; }
                        } else if (swipeStartFace == 3) { // Right
                            if (fabs(dz) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dz>0)?PI/2:-PI/2; }
                            else { axis = 2; slice = round(swipeStartPos.z); ang = (dy>0)?-PI/2:PI/2; }
                        } else if (swipeStartFace == 2) { // Left
                            if (fabs(dz) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dz>0)?-PI/2:PI/2; }
                            else { axis = 2; slice = round(swipeStartPos.z); ang = (dy>0)?-PI/2:PI/2; }
                        }

                        StartRotation(axis, slice, ang, 6.0f);
                        isSwiping = false;
                    }
                } else {
                    isSwiping = false;
                }
            } else if (!touchPressed) { 
                camAngleX -= delta.x * 0.008f;
                camAngleY += delta.y * 0.008f;
                if (camAngleY > 80.0f * DEG2RAD) camAngleY = 80.0f * DEG2RAD;
                if (camAngleY < -80.0f * DEG2RAD) camAngleY = -80.0f * DEG2RAD;
            }
            lastMousePos = mousePos;
        } else {
            isSwiping = false;
        }
    }

    if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        isSwiping = false;
    }
    lastMousePos = GetMousePosition();

    camera.position.x = camRadius * cosf(camAngleY) * sinf(camAngleX);
    camera.position.z = camRadius * cosf(camAngleY) * cosf(camAngleX);
    camera.position.y = camRadius * sinf(camAngleY);

    BeginDrawing();
    ClearBackground(Color{12, 12, 14, 255});
    
    BeginMode3D(camera);
    // Explicitly enforce OpenGL depth test, depth write mask, and backface culling
    rlEnableDepthTest();
    rlEnableDepthMask();
    rlEnableBackfaceCulling();
    
    for (auto& c : cubies) {
        DrawCubie(c);
    }
    EndMode3D();

    EndDrawing();
}

int main() {
    int screenWidth = 800;
    int screenHeight = 800;
#if defined(__EMSCRIPTEN__)
    screenWidth = EM_ASM_INT({ return window.innerWidth; });
    screenHeight = EM_ASM_INT({ return window.innerHeight; });
#endif

    InitWindow(screenWidth, screenHeight, "Rubik's Cube 3D");
    
    camera.position = Vector3{ 6.0f, 5.0f, 6.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 35.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    InitCube();

#if defined(__EMSCRIPTEN__)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        UpdateDrawFrame();
    }
#endif

    CloseWindow();
    return 0;
}
