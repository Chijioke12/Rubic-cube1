#include "raylib.h"
#include "rlgl.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <iostream>

#if defined(__EMSCRIPTEN__)
#include <emscripten/emscripten.h>
#endif

// Define standard colors
const Color C_UP = YELLOW;
const Color C_DOWN = WHITE;
const Color C_LEFT = BLUE;
const Color C_RIGHT = GREEN;
const Color C_FRONT = RED;
const Color C_BACK = ORANGE;
const Color C_INNER = BLACK;

struct Cubie {
    Vector3 logicalPos; // -1, 0, 1 for each axis
    Matrix transform;
    Color colors[6]; // UP, DOWN, LEFT, RIGHT, FRONT, BACK
};

std::vector<Cubie> cubies;

Camera camera = { 0 };

bool isAnimating = false;
float animProgress = 0.0f;
float animTarget = 0.0f;
Vector3 animAxis = {0,0,0};
int animSliceAxis = 0; // 0=X, 1=Y, 2=Z
int animSliceValue = 0; 

Vector2 lastMousePos = {0};
bool isSwiping = false;
Vector3 swipeStartPos = {0};
int swipeStartFace = -1; // -1 = none, 0=U, 1=D, 2=L, 3=R, 4=F, 5=B

void InitCube() {
    cubies.clear();
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Cubie c;
                c.logicalPos = {(float)x, (float)y, (float)z};
                c.transform = MatrixIdentity();
                c.transform = MatrixTranslate(x, y, z);
                
                c.colors[0] = (y == 1) ? C_UP : C_INNER;
                c.colors[1] = (y == -1) ? C_DOWN : C_INNER;
                c.colors[2] = (x == -1) ? C_LEFT : C_INNER;
                c.colors[3] = (x == 1) ? C_RIGHT : C_INNER;
                c.colors[4] = (z == 1) ? C_FRONT : C_INNER;
                c.colors[5] = (z == -1) ? C_BACK : C_INNER;

                cubies.push_back(c);
            }
        }
    }
}

void DrawCubie(Cubie& c) {
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(c.transform));

    float s = 0.47f; // Half size with gap

    rlBegin(RL_QUADS);
    // FRONT
    rlColor4ub(c.colors[4].r, c.colors[4].g, c.colors[4].b, 255);
    rlVertex3f(-s, -s,  s); rlVertex3f( s, -s,  s); rlVertex3f( s,  s,  s); rlVertex3f(-s,  s,  s);
    // BACK
    rlColor4ub(c.colors[5].r, c.colors[5].g, c.colors[5].b, 255);
    rlVertex3f(-s,  s, -s); rlVertex3f( s,  s, -s); rlVertex3f( s, -s, -s); rlVertex3f(-s, -s, -s);
    // UP
    rlColor4ub(c.colors[0].r, c.colors[0].g, c.colors[0].b, 255);
    rlVertex3f(-s,  s,  s); rlVertex3f( s,  s,  s); rlVertex3f( s,  s, -s); rlVertex3f(-s,  s, -s);
    // DOWN
    rlColor4ub(c.colors[1].r, c.colors[1].g, c.colors[1].b, 255);
    rlVertex3f(-s, -s, -s); rlVertex3f( s, -s, -s); rlVertex3f( s, -s,  s); rlVertex3f(-s, -s,  s);
    // RIGHT
    rlColor4ub(c.colors[3].r, c.colors[3].g, c.colors[3].b, 255);
    rlVertex3f( s, -s, -s); rlVertex3f( s,  s, -s); rlVertex3f( s,  s,  s); rlVertex3f( s, -s,  s);
    // LEFT
    rlColor4ub(c.colors[2].r, c.colors[2].g, c.colors[2].b, 255);
    rlVertex3f(-s, -s,  s); rlVertex3f(-s,  s,  s); rlVertex3f(-s,  s, -s); rlVertex3f(-s, -s, -s);
    
    rlEnd();
    rlPopMatrix();
}

void StartRotation(int axis, int slice, float angle) {
    if (isAnimating) return;
    isAnimating = true;
    animProgress = 0.0f;
    animTarget = angle;
    animSliceAxis = axis;
    animSliceValue = slice;
    animAxis = (axis == 0) ? Vector3{1,0,0} : ((axis == 1) ? Vector3{0,1,0} : Vector3{0,0,1});
}

void ScrambleCube() {
    for (int i=0; i<20; i++) {
        int axis = GetRandomValue(0, 2);
        int slice = GetRandomValue(-1, 1);
        float angle = (GetRandomValue(0, 1) == 0 ? 90.0f : -90.0f) * DEG2RAD;
        
        Matrix rot = MatrixRotate((axis==0)?Vector3{1,0,0}:(axis==1)?Vector3{0,1,0}:Vector3{0,0,1}, angle);
        for (auto& c : cubies) {
            float v = (axis == 0) ? c.logicalPos.x : ((axis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - slice) < 0.1f) {
                c.logicalPos = Vector3Transform(c.logicalPos, rot);
                c.logicalPos.x = roundf(c.logicalPos.x);
                c.logicalPos.y = roundf(c.logicalPos.y);
                c.logicalPos.z = roundf(c.logicalPos.z);
                c.transform = MatrixMultiply(c.transform, rot);
            }
        }
    }
}

extern "C" {
#if defined(__EMSCRIPTEN__)
    EMSCRIPTEN_KEEPALIVE
#endif
    void scramble_cube() {
        ScrambleCube();
    }
}

void UpdateDrawFrame(void) {
    float dt = GetFrameTime();

    if (isAnimating) {
        float step = 10.0f * dt; // speed
        if (animProgress + step >= 1.0f) {
            step = 1.0f - animProgress;
            isAnimating = false;
        }
        animProgress += step;

        float frameAngle = animTarget * step;
        Matrix rot = MatrixRotate(animAxis, frameAngle);

        for (auto& c : cubies) {
            float v = (animSliceAxis == 0) ? c.logicalPos.x : ((animSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - animSliceValue) < 0.1f) {
                c.transform = MatrixMultiply(c.transform, rot);
                if (!isAnimating) { // finish
                    c.logicalPos = Vector3Transform(c.logicalPos, MatrixRotate(animAxis, animTarget));
                    c.logicalPos.x = roundf(c.logicalPos.x);
                    c.logicalPos.y = roundf(c.logicalPos.y);
                    c.logicalPos.z = roundf(c.logicalPos.z);
                }
            }
        }
    }

    if (!isAnimating) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) || GetTouchPointCount() > 0) {
            Vector2 mousePos = GetMousePosition();
            Vector2 delta = { mousePos.x - lastMousePos.x, mousePos.y - lastMousePos.y };
            
            // Simple camera rotation for demonstration, if not hitting cube
            // Let's do Raycast
            Ray ray = GetMouseRay(mousePos, camera);
            
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Raycast vs bounding box [-1.5, 1.5]
                RayCollision col = GetRayCollisionBox(ray, BoundingBox{Vector3{-1.5f, -1.5f, -1.5f}, Vector3{1.5f, 1.5f, 1.5f}});
                if (col.hit) {
                    isSwiping = true;
                    swipeStartPos = col.point;
                    if (fabs(col.point.y - 1.5f) < 0.01f) swipeStartFace = 0; // U
                    else if (fabs(col.point.y - (-1.5f)) < 0.01f) swipeStartFace = 1; // D
                    else if (fabs(col.point.x - (-1.5f)) < 0.01f) swipeStartFace = 2; // L
                    else if (fabs(col.point.x - 1.5f) < 0.01f) swipeStartFace = 3; // R
                    else if (fabs(col.point.z - 1.5f) < 0.01f) swipeStartFace = 4; // F
                    else if (fabs(col.point.z - (-1.5f)) < 0.01f) swipeStartFace = 5; // B
                }
            }

            if (isSwiping) {
                RayCollision col = GetRayCollisionBox(ray, BoundingBox{Vector3{-1.5f, -1.5f, -1.5f}, Vector3{1.5f, 1.5f, 1.5f}});
                if (col.hit) {
                    float dx = col.point.x - swipeStartPos.x;
                    float dy = col.point.y - swipeStartPos.y;
                    float dz = col.point.z - swipeStartPos.z;

                    float thresh = 0.3f;
                    if (fabs(dx) > thresh || fabs(dy) > thresh || fabs(dz) > thresh) {
                        // Determine slice and angle
                        int axis = 0; int slice = 0; float ang = PI/2;
                        
                        if (swipeStartFace == 4) { // Front
                            if (fabs(dx) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dx>0)?-PI/2:PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dy>0)?PI/2:-PI/2; }
                        } else if (swipeStartFace == 0) { // Up
                            if (fabs(dx) > fabs(dz)) { axis = 2; slice = round(swipeStartPos.z); ang = (dx>0)?-PI/2:PI/2; }
                            else { axis = 0; slice = round(swipeStartPos.x); ang = (dz>0)?-PI/2:PI/2; }
                        } else if (swipeStartFace == 3) { // Right
                            if (fabs(dz) > fabs(dy)) { axis = 1; slice = round(swipeStartPos.y); ang = (dz>0)?PI/2:-PI/2; }
                            else { axis = 2; slice = round(swipeStartPos.z); ang = (dy>0)?-PI/2:PI/2; }
                        } else {
                            // Simplify by defaulting if not specifically handled
                            axis = 0; slice = 0; ang = PI/2;
                        }

                        StartRotation(axis, slice, ang);
                        isSwiping = false;
                    }
                }
            } else {
                // Orbit camera
                UpdateCameraPro(&camera, Vector3{0,0,0}, Vector3{delta.x*0.5f, delta.y*0.5f, 0.0f}, 0.0f);
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

    BeginDrawing();
    ClearBackground(Color{10, 10, 10, 255});
    BeginMode3D(camera);
    for (auto& c : cubies) {
        DrawCubie(c);
    }
    EndMode3D();
    EndDrawing();
}

int main() {
    InitWindow(800, 800, "Rubik's Cube");
    
    // Setup camera
    camera.position = Vector3{ 5.0f, 5.0f, 5.0f };
    camera.target = Vector3{ 0.0f, 0.0f, 0.0f };
    camera.up = Vector3{ 0.0f, 1.0f, 0.0f };
    camera.fovy = 45.0f;
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
