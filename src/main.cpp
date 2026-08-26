#ifdef __EMSCRIPTEN__
#include <SDL.h>
#include <SDL_opengles2.h>
#include <emscripten.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#endif
#include "cube.h"
#include "math_utils.h"
#include <vector>
#include <iostream>

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

Cube cube;
bool running = true;
float rotX = -0.5f, rotY = 0.5f;
GLuint program, vbo;

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
}

void drawQuad(std::vector<float>& vertices, Vec3 p1, Vec3 p2, Vec3 p3, Vec3 p4, Color c) {
    SDL_Color sc;
    switch(c) {
        case WHITE: sc = {255, 255, 255, 255}; break;
        case YELLOW: sc = {255, 255, 0, 255}; break;
        case RED: sc = {255, 0, 0, 255}; break;
        case ORANGE: sc = {255, 165, 0, 255}; break;
        case BLUE: sc = {0, 0, 255, 255}; break;
        case GREEN: sc = {0, 255, 0, 255}; break;
    }
    float r = sc.r/255.0f, g = sc.g/255.0f, b = sc.b/255.0f;
    float quad[] = {
        p1.x, p1.y, p1.z, r, g, b, 1.0f,
        p2.x, p2.y, p2.z, r, g, b, 1.0f,
        p3.x, p3.y, p3.z, r, g, b, 1.0f,
        p1.x, p1.y, p1.z, r, g, b, 1.0f,
        p3.x, p3.y, p3.z, r, g, b, 1.0f,
        p4.x, p4.y, p4.z, r, g, b, 1.0f
    };
    vertices.insert(vertices.end(), quad, quad + 42);
}

void render() {
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Matrix4 model = Matrix4::rotateX(rotX) * Matrix4::rotateY(rotY);
    Matrix4 view = Matrix4::translate(0, 0, -8);
    Matrix4 proj = Matrix4::perspective(0.78f, 800.0f/600.0f, 0.1f, 100.0f);
    Matrix4 mvp = proj * view * model;

    GLint mvpLoc = glGetUniformLocation(program, "uMVP");
    glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, mvp.m);

    std::vector<float> vertices;
    float s = 1.0f;
    for (int f = 0; f < 6; f++) {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                float y = 1.5f - i, x = j - 1.5f;
                float y2 = y - 1.0f, x2 = x + 1.0f;
                Color c = cube.getColor((Face)f, i, j);
                if (f == UP) drawQuad(vertices, {x, 1.5, -y}, {x2, 1.5, -y}, {x2, 1.5, -y2}, {x, 1.5, -y2}, c);
                if (f == DOWN) drawQuad(vertices, {x, -1.5, y}, {x2, -1.5, y}, {x2, -1.5, y2}, {x, -1.5, y2}, c);
                if (f == FRONT) drawQuad(vertices, {x, y, 1.5}, {x2, y, 1.5}, {x2, y2, 1.5}, {x, y2, 1.5}, c);
                if (f == BACK) drawQuad(vertices, {-x, y, -1.5}, {-x2, y, -1.5}, {-x2, y2, -1.5}, {-x, y2, -1.5}, c);
                if (f == LEFT) drawQuad(vertices, {-1.5, y, -x}, {-1.5, y, -x2}, {-1.5, y2, -x2}, {-1.5, y2, -x}, c);
                if (f == RIGHT) drawQuad(vertices, {1.5, y, x}, {1.5, y, x2}, {1.5, y2, x2}, {1.5, y2, x}, c);
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

void handleInput() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) running = false;
        if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON_LMASK)) {
            rotY += event.motion.xrel * 0.01f;
            rotX += event.motion.yrel * 0.01f;
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
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_Window* window = SDL_CreateWindow("3D Rubik's Cube", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL);
    SDL_GL_CreateContext(window);
    initGL();
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(mainLoop, 0, 1);
#else
    while (running) { mainLoop(); SDL_GL_SwapWindow(window); SDL_Delay(16); }
#endif
    return 0;
}

