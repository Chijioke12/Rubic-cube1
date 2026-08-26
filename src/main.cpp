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

// Rubik's Cube Vibrant Colors
const Color C_UP    = { 255, 255, 255, 255 }; // 0: White (Top)
const Color C_DOWN  = { 255, 215,   0, 255 }; // 1: Yellow (Bottom)
const Color C_LEFT  = { 255, 105,   0, 255 }; // 2: Orange (Left)
const Color C_RIGHT = { 220,  20,  25, 255 }; // 3: Red (Right)
const Color C_FRONT = {   0, 165,  65, 255 }; // 4: Green (Front)
const Color C_BACK  = {  10,  90, 225, 255 }; // 5: Blue (Back)
const Color C_CORE  = {  18,  18,  20, 255 }; // 6: Matte Black (Internal / Border)

Texture2D faceTextures[7]; // 0..5 = Colors with rounded borders, 6 = Solid Black

struct Cubie {
    Vector3 logicalPos;
    Matrix baseTransform;
    int texIndices[6]; // 0: UP(+Y), 1: DOWN(-Y), 2: LEFT(-X), 3: RIGHT(+X), 4: FRONT(+Z), 5: BACK(-Z)
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

void GenerateProceduralTextures() {
    Color palette[6] = { C_UP, C_DOWN, C_LEFT, C_RIGHT, C_FRONT, C_BACK };
    
    // Generate 6 high-res sticker textures with beautiful rounded corners and matte black borders
    for (int i = 0; i < 6; i++) {
        Image img = GenImageColor(256, 256, C_CORE);
        
        int margin = 14;
        int size = 256 - (margin * 2); // 228
        int radius = 20;

        // Draw cross rectangles for rounded box
        ImageDrawRectangle(&img, margin + radius, margin, size - 2 * radius, size, palette[i]);
        ImageDrawRectangle(&img, margin, margin + radius, size, size - 2 * radius, palette[i]);
        
        // Draw 4 rounded corner circles
        ImageDrawCircle(&img, margin + radius, margin + radius, radius, palette[i]);
        ImageDrawCircle(&img, margin + size - radius, margin + radius, radius, palette[i]);
        ImageDrawCircle(&img, margin + radius, margin + size - radius, radius, palette[i]);
        ImageDrawCircle(&img, margin + size - radius, margin + size - radius, radius, palette[i]);

        faceTextures[i] = LoadTextureFromImage(img);
        SetTextureFilter(faceTextures[i], TEXTURE_FILTER_BILINEAR);
        SetTextureWrap(faceTextures[i], TEXTURE_WRAP_CLAMP);
        UnloadImage(img);
    }
    
    // Generate 1 solid dark texture for internal unexposed cubie faces
    Image coreImg = GenImageColor(256, 256, C_CORE);
    faceTextures[6] = LoadTextureFromImage(coreImg);
    SetTextureFilter(faceTextures[6], TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(faceTextures[6], TEXTURE_WRAP_CLAMP);
    UnloadImage(coreImg);
}

void InitCube() {
    cubies.clear();
    for (int x = -1; x <= 1; x++) {
        for (int y = -1; y <= 1; y++) {
            for (int z = -1; z <= 1; z++) {
                Cubie c;
                c.logicalPos = {(float)x, (float)y, (float)z};
                c.baseTransform = MatrixTranslate(x, y, z);
                
                c.texIndices[0] = (y ==  1) ? 0 : 6; // UP (+Y)
                c.texIndices[1] = (y == -1) ? 1 : 6; // DOWN (-Y)
                c.texIndices[2] = (x == -1) ? 2 : 6; // LEFT (-X)
                c.texIndices[3] = (x ==  1) ? 3 : 6; // RIGHT (+X)
                c.texIndices[4] = (z ==  1) ? 4 : 6; // FRONT (+Z)
                c.texIndices[5] = (z == -1) ? 5 : 6; // BACK (-Z)
                
                cubies.push_back(c);
            }
        }
    }
}

// Draw a single cubie with 6 textured quad faces
void DrawTexturedCubie(const Cubie& c, const Matrix& renderTransform) {
    rlPushMatrix();
    rlMultMatrixf(MatrixToFloat(renderTransform));

    const float s = 0.49f; // Clean half-size, tight 0.02f cubie seam

    // 0: UP (+Y) face
    rlSetTexture(faceTextures[c.texIndices[0]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-s,  s, -s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-s,  s,  s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f( s,  s,  s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f( s,  s, -s);
    rlEnd();

    // 1: DOWN (-Y) face
    rlSetTexture(faceTextures[c.texIndices[1]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(240, 240, 240, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-s, -s,  s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-s, -s, -s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f( s, -s, -s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f( s, -s,  s);
    rlEnd();

    // 2: LEFT (-X) face
    rlSetTexture(faceTextures[c.texIndices[2]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(245, 245, 245, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-s,  s, -s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-s, -s, -s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(-s, -s,  s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(-s,  s,  s);
    rlEnd();

    // 3: RIGHT (+X) face
    rlSetTexture(faceTextures[c.texIndices[3]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(250, 250, 250, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f( s,  s,  s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f( s, -s,  s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f( s, -s, -s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f( s,  s, -s);
    rlEnd();

    // 4: FRONT (+Z) face
    rlSetTexture(faceTextures[c.texIndices[4]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(255, 255, 255, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f(-s,  s,  s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f(-s, -s,  s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f( s, -s,  s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f( s,  s,  s);
    rlEnd();

    // 5: BACK (-Z) face
    rlSetTexture(faceTextures[c.texIndices[5]].id);
    rlBegin(RL_QUADS);
    rlColor4ub(240, 240, 240, 255);
    rlTexCoord2f(0.0f, 0.0f); rlVertex3f( s,  s, -s);
    rlTexCoord2f(0.0f, 1.0f); rlVertex3f( s, -s, -s);
    rlTexCoord2f(1.0f, 1.0f); rlVertex3f(-s, -s, -s);
    rlTexCoord2f(1.0f, 0.0f); rlVertex3f(-s,  s, -s);
    rlEnd();

    rlSetTexture(0);
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
        m.speed = 18.0f; // Rapid, fluid scramble sequence
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

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float aspect = screenW / (screenH > 0 ? screenH : 1.0f);

    // Responsive Camera Framing: Automatically scale to fit portrait mobile & desktop screens
    float baseRadius = 8.5f;
    if (aspect < 1.0f) {
        camRadius = baseRadius / (aspect * 1.05f);
        camera.fovy = 38.0f;
    } else {
        camRadius = baseRadius;
        camera.fovy = 35.0f;
    }

    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    // Process queued scramble animations sequentially
    if (!isAnimating && !moveQueue.empty()) {
        Move nextMove = moveQueue.front();
        moveQueue.pop_front();
        StartRotation(nextMove.axis, nextMove.slice, nextMove.angle, nextMove.speed);
    }

    // Update animation progress
    if (isAnimating) {
        animProgress += currentAnimSpeed * dt;
        
        if (animProgress >= 1.0f) {
            animProgress = 1.0f;
            
            // Finalize rotation in discrete steps on baseTransform & logicalPos
            Matrix finalRot = MatrixRotate(animAxis, animTarget);
            
            for (auto& c : cubies) {
                float v = (animSliceAxis == 0) ? c.logicalPos.x : ((animSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
                if (std::abs(v - animSliceValue) < 0.1f) {
                    // Update logical coordinate
                    c.logicalPos = Vector3Transform(c.logicalPos, finalRot);
                    c.logicalPos.x = roundf(c.logicalPos.x);
                    c.logicalPos.y = roundf(c.logicalPos.y);
                    c.logicalPos.z = roundf(c.logicalPos.z);

                    // Update resting base transform
                    c.baseTransform = MatrixMultiply(finalRot, c.baseTransform);
                    
                    // Snap translation components to integer grid to eliminate float drift
                    c.baseTransform.m12 = c.logicalPos.x;
                    c.baseTransform.m13 = c.logicalPos.y;
                    c.baseTransform.m14 = c.logicalPos.z;

                    // Re-orthonormalize 3x3 rotational submatrix
                    float* m = (float*)&c.baseTransform;
                    for (int i = 0; i < 16; i++) {
                        if (i != 12 && i != 13 && i != 14 && i != 15) {
                            if (fabsf(m[i]) < 0.05f) m[i] = 0.0f;
                            else if (m[i] > 0.95f) m[i] = 1.0f;
                            else if (m[i] < -0.95f) m[i] = -1.0f;
                        }
                    }
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
    rlEnableDepthTest();
    rlEnableDepthMask();
    // Disable backface culling to ensure no polygons are culled across perspective orientations
    rlDisableBackfaceCulling();
    
    Matrix animRotMatrix = MatrixIdentity();
    if (isAnimating) {
        animRotMatrix = MatrixRotate(animAxis, animTarget * animProgress);
    }

    for (const auto& c : cubies) {
        Matrix renderMat = c.baseTransform;
        if (isAnimating) {
            float v = (animSliceAxis == 0) ? c.logicalPos.x : ((animSliceAxis == 1) ? c.logicalPos.y : c.logicalPos.z);
            if (std::abs(v - animSliceValue) < 0.1f) {
                renderMat = MatrixMultiply(animRotMatrix, c.baseTransform);
            }
        }
        DrawTexturedCubie(c, renderMat);
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

    GenerateProceduralTextures();
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
