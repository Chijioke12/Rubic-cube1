#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <SDL_opengles2.h>
#include <emscripten.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#define EMSCRIPTEN_KEEPALIVE
#endif
#include "cube.h"
#include "math_utils.h"
#include <vector>
#include <iostream>

Cube cube;
SDL_Window* window = nullptr;
bool running = true;
float rotX = -0.5f, rotY = 0.5f;
GLuint program, vbo;

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void rotate_cube_face(int face, int clockwise) {
        cube.rotateFace((Face)face, clockwise != 0);
    }
    
    EMSCRIPTEN_KEEPALIVE
    void scramble_cube() {
        cube.scramble();
    }
}

const char* vShaderSource = 
    "attribute vec3 aPos;"
    "attribute vec4 aColor;"
    "varying vec4 vColor;"
    "uniform mat4 uMVP;"
    "void main() {"
    "   gl_Position = uMVP * vec4(aPos, 1.0);"
    "   vColor = aColor;"
    "}";

const char* fShaderSource = 
    "precision mediump float;"
    "varying vec4 vColor;"
    "void main() {"
    "   gl_FragColor = vColor;"
    "}";

GLuint compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    return shader;
}

void initGL() {
    program = glCreateProgram();
    glAttachShader(program, compileShader(GL_VERTEX_SHADER, vShaderSource));
    glAttachShader(program, compileShader(GL_FRAGMENT_SHADER, fShaderSource));
    glLinkProgram(program);
    glUseProgram(program);
    glGenBuffers(1, &vbo);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
}

void drawQuad(std::vector<float>& vertices, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Color c, float brightness) {
    SDL_Color sc;
    switch(c) {
        case WHITE: sc = {255, 255, 255, 255}; break;
        case YELLOW: sc = {255, 255, 0, 255}; break;
        case RED: sc = {255, 0, 0, 255}; break;
        case ORANGE: sc = {255, 165, 0, 255}; break;
        case BLUE: sc = {0, 0, 255, 255}; break;
        case GREEN: sc = {0, 255, 0, 255}; break;
    }
    float r = (sc.r/255.0f) * brightness, g = (sc.g/255.0f) * brightness, b = (sc.b/255.0f) * brightness;
    // CCW order for front-face culling: BL->BR->TR and BL->TR->TL
    float quad[] = {
        p1.x, p1.y, p1.z, r, g, b, 1.0f,
        p4.x, p4.y, p4.z, r, g, b, 1.0f,
        p3.x, p3.y, p3.z, r, g, b, 1.0f,
        p1.x, p1.y, p1.z, r, g, b, 1.0f,
        p3.x, p3.y, p3.z, r, g, b, 1.0f,
        p2.x, p2.y, p2.z, r, g, b, 1.0f
    };
    vertices.insert(vertices.end(), quad, quad + 42);
}

void render() {
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4 model = Matrix4::rotateX(rotX) * Matrix4::rotateY(rotY);
    Matrix4 view = Matrix4::translate(0, 0, -8);
    Matrix4 proj = Matrix4::perspective(0.78f, 800.0f/600.0f, 0.1f, 100.0f);
    Matrix4 mvp = proj * view * model;

    GLint mvpLoc = glGetUniformLocation(program, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.m);

    std::vector<float> vertices;
    float g = 0.05f; // gap
    for (int f = 0; f < 6; f++) {
        float b = 1.0f; // brightness
        if (f == UP) b = 1.0f;
        if (f == DOWN) b = 0.6f;
        if (f == LEFT) b = 0.8f;
        if (f == RIGHT) b = 0.8f;
        if (f == FRONT) b = 0.9f;
        if (f == BACK) b = 0.7f;

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                Color c = cube.getColor((Face)f, i, j);
                
                float lx0 = (j - 1.5f) + g;
                float lx1 = (j - 0.5f) - g;
                float ly1 = (1.5f - i) - g;
                float ly0 = (0.5f - i) + g;

                Vec3 p1, p2, p3, p4;

                if (f == FRONT) {
                    p1 = {lx0, ly1, 1.5f};
                    p2 = {lx1, ly1, 1.5f};
                    p3 = {lx1, ly0, 1.5f};
                    p4 = {lx0, ly0, 1.5f};
                } else if (f == BACK) {
                    p1 = {-lx0, ly1, -1.5f};
                    p2 = {-lx1, ly1, -1.5f};
                    p3 = {-lx1, ly0, -1.5f};
                    p4 = {-lx0, ly0, -1.5f};
                } else if (f == LEFT) {
                    p1 = {-1.5f, ly1, lx0};
                    p2 = {-1.5f, ly1, lx1};
                    p3 = {-1.5f, ly0, lx1};
                    p4 = {-1.5f, ly0, lx0};
                } else if (f == RIGHT) {
                    p1 = {1.5f, ly1, -lx0};
                    p2 = {1.5f, ly1, -lx1};
                    p3 = {1.5f, ly0, -lx1};
                    p4 = {1.5f, ly0, -lx0};
                } else if (f == UP) {
                    p1 = {lx0, 1.5f, -ly1};
                    p2 = {lx1, 1.5f, -ly1};
                    p3 = {lx1, 1.5f, -ly0};
                    p4 = {lx0, 1.5f, -ly0};
                } else if (f == DOWN) {
                    p1 = {lx0, -1.5f, ly1};
                    p2 = {lx1, -1.5f, ly1};
                    p3 = {lx1, -1.5f, ly0};
                    p4 = {lx0, -1.5f, ly0};
                }

                drawQuad(vertices, p1, p2, p3, p4, c, b);
            }
        }
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_DYNAMIC_DRAW);
    
    GLint posLoc = glGetAttribLocation(program, "aPos");
    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);

    GLint colLoc = glGetAttribLocation(program, "aColor");
    glEnableVertexAttribArray(colLoc);
    glVertexAttribPointer(colLoc, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));

    glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 7);
}

struct Hit {
    bool hit;
    Face face;
    Vec3 point;
    float t;
};

Matrix4 getModelMatrix() {
    return Matrix4::rotateX(rotX) * Matrix4::rotateY(rotY);
}

Hit raycast(float touchX, float touchY, float screenW, float screenH) {
    float aspect = screenW / screenH;
    float ndcX = touchX * 2.0f - 1.0f;
    float ndcY = 1.0f - touchY * 2.0f;
    float fov = 0.78f;
    float tanHalfFov = tan(fov / 2.0f);
    Vec3 rayDirView = { ndcX * aspect * tanHalfFov, ndcY * tanHalfFov, -1.0f };
    float len = sqrt(rayDirView.x*rayDirView.x + rayDirView.y*rayDirView.y + rayDirView.z*rayDirView.z);
    rayDirView.x /= len; rayDirView.y /= len; rayDirView.z /= len;

    Matrix4 model = getModelMatrix();
    Matrix4 invRot = {0};
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) invRot.m[i*4+j] = model.m[j*4+i];

    Vec3 rayOrigLocal = {0, 0, 8.0f};
    Vec3 ro = {
        rayOrigLocal.x * invRot.m[0] + rayOrigLocal.y * invRot.m[4] + rayOrigLocal.z * invRot.m[8],
        rayOrigLocal.x * invRot.m[1] + rayOrigLocal.y * invRot.m[5] + rayOrigLocal.z * invRot.m[9],
        rayOrigLocal.x * invRot.m[2] + rayOrigLocal.y * invRot.m[6] + rayOrigLocal.z * invRot.m[10]
    };

    Vec3 rd = {
        rayDirView.x * invRot.m[0] + rayDirView.y * invRot.m[4] + rayDirView.z * invRot.m[8],
        rayDirView.x * invRot.m[1] + rayDirView.y * invRot.m[5] + rayDirView.z * invRot.m[9],
        rayDirView.x * invRot.m[2] + rayDirView.y * invRot.m[6] + rayDirView.z * invRot.m[10]
    };

    Hit bestHit = {false, UP, {0,0,0}, 9999.0f};

    auto checkFace = [&](Face f, float nx, float ny, float nz, float dist) {
        float denom = rd.x * nx + rd.y * ny + rd.z * nz;
        if (fabs(denom) < 0.0001f) return;
        float t = ((nx*dist - ro.x)*nx + (ny*dist - ro.y)*ny + (nz*dist - ro.z)*nz) / denom;
        if (t > 0 && t < bestHit.t) {
            Vec3 p = {ro.x + rd.x * t, ro.y + rd.y * t, ro.z + rd.z * t};
            bool inBounds = true;
            if (nx == 0 && (p.x < -1.5f || p.x > 1.5f)) inBounds = false;
            if (ny == 0 && (p.y < -1.5f || p.y > 1.5f)) inBounds = false;
            if (nz == 0 && (p.z < -1.5f || p.z > 1.5f)) inBounds = false;
            if (inBounds) {
                bestHit = {true, f, p, t};
            }
        }
    };

    checkFace(RIGHT, 1, 0, 0, 1.5f);
    checkFace(LEFT, -1, 0, 0, 1.5f);
    checkFace(UP, 0, 1, 0, 1.5f);
    checkFace(DOWN, 0, -1, 0, 1.5f);
    checkFace(FRONT, 0, 0, 1, 1.5f);
    checkFace(BACK, 0, 0, -1, 1.5f);

    return bestHit;
}

float touchStartX = 0.0f;
float touchStartY = 0.0f;
bool isSwiping = false;
bool isCubeHit = false;
Hit initialHit;

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = false;
        
        if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
            rotY += event.motion.xrel * 0.01f;
            rotX += event.motion.yrel * 0.01f;
        }

        if (event.type == SDL_FINGERDOWN) {
            touchStartX = event.tfinger.x;
            touchStartY = event.tfinger.y;
            isSwiping = false;
            
            int w, h;
            SDL_GetWindowSize(window, &w, &h);
            initialHit = raycast(touchStartX, touchStartY, (float)w, (float)h);
            isCubeHit = initialHit.hit;
        }
        
        if (event.type == SDL_FINGERMOTION) {
            isSwiping = true;
            if (!isCubeHit) {
                rotY += event.tfinger.dx * 5.0f;
                rotX += event.tfinger.dy * 5.0f;
            }
        }

        if (event.type == SDL_FINGERUP) {
            if (isCubeHit && isSwiping) {
                float dx = event.tfinger.x - touchStartX;
                float dy = event.tfinger.y - touchStartY;
                if (sqrt(dx*dx + dy*dy) > 0.05f) {
                    bool cw = false;
                    Face rotF = UP;
                    
                    if (initialHit.face == FRONT) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.y > 0) ? UP : DOWN;
                            cw = (initialHit.point.y > 0) ? (dx < 0) : (dx > 0);
                        } else {
                            rotF = (initialHit.point.x > 0) ? RIGHT : LEFT;
                            cw = (initialHit.point.x > 0) ? (dy > 0) : (dy < 0);
                        }
                    } else if (initialHit.face == UP) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.z > 0) ? FRONT : BACK;
                            cw = (initialHit.point.z > 0) ? (dx < 0) : (dx > 0);
                        } else {
                            rotF = (initialHit.point.x > 0) ? RIGHT : LEFT;
                            cw = (initialHit.point.x > 0) ? (dy < 0) : (dy > 0);
                        }
                    } else if (initialHit.face == RIGHT) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.y > 0) ? UP : DOWN;
                            cw = (initialHit.point.y > 0) ? (dx > 0) : (dx < 0);
                        } else {
                            rotF = (initialHit.point.z > 0) ? FRONT : BACK;
                            cw = (initialHit.point.z > 0) ? (dy > 0) : (dy < 0);
                        }
                    } else if (initialHit.face == LEFT) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.y > 0) ? UP : DOWN;
                            cw = (initialHit.point.y > 0) ? (dx < 0) : (dx > 0);
                        } else {
                            rotF = (initialHit.point.z > 0) ? FRONT : BACK;
                            cw = (initialHit.point.z > 0) ? (dy < 0) : (dy > 0);
                        }
                    } else if (initialHit.face == DOWN) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.z > 0) ? FRONT : BACK;
                            cw = (initialHit.point.z > 0) ? (dx > 0) : (dx < 0);
                        } else {
                            rotF = (initialHit.point.x > 0) ? RIGHT : LEFT;
                            cw = (initialHit.point.x > 0) ? (dy < 0) : (dy > 0);
                        }
                    } else if (initialHit.face == BACK) {
                        if (fabs(dx) > fabs(dy)) {
                            rotF = (initialHit.point.y > 0) ? UP : DOWN;
                            cw = (initialHit.point.y > 0) ? (dx > 0) : (dx < 0);
                        } else {
                            rotF = (initialHit.point.x > 0) ? RIGHT : LEFT;
                            cw = (initialHit.point.x > 0) ? (dy > 0) : (dy < 0);
                        }
                    }
                    cube.rotateFace(rotF, cw);
                }
            }
            isCubeHit = false;
        }

        if (event.type == SDL_KEYDOWN) {
            bool clockwise = !(SDL_GetModState() & KMOD_SHIFT);
            switch (event.key.keysym.sym) {
                case SDLK_u: cube.rotateFace(UP, clockwise); break;
                case SDLK_d: cube.rotateFace(DOWN, clockwise); break;
                case SDLK_l: cube.rotateFace(LEFT, clockwise); break;
                case SDLK_r: cube.rotateFace(RIGHT, clockwise); break;
                case SDLK_f: cube.rotateFace(FRONT, clockwise); break;
                case SDLK_b: cube.rotateFace(BACK, clockwise); break;
                case SDLK_s: cube.scramble(); break;
            }
        }
    }
}

void mainLoop() {
    handleInput();
    render();
    SDL_GL_SwapWindow(window);
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    void scramble_cube() {
        cube.scramble();
    }
}
#endif

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    window = SDL_CreateWindow("3D Rubik's Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    SDL_GL_CreateContext(window);
    initGL();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    while (running) { mainLoop(); SDL_Delay(16); }
#endif
    return 0;
}

