#include "cube.h"
#include <algorithm>
#include <ctime>
#include <cstdlib>

Cube::Cube() {
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            state[UP][i][j] = WHITE;
            state[DOWN][i][j] = YELLOW;
            state[LEFT][i][j] = ORANGE;
            state[RIGHT][i][j] = RED;
            state[FRONT][i][j] = GREEN;
            state[BACK][i][j] = BLUE;
        }
    }
    std::srand(std::time(nullptr));
}

void Cube::rotateFaceGrid(Face face, bool clockwise) {
    Color temp[3][3];
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (clockwise) temp[j][2 - i] = state[face][i][j];
            else temp[2 - j][i] = state[face][i][j];
        }
    }
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            state[face][i][j] = temp[i][j];
        }
    }
}

void Cube::rotateFace(Face face, bool clockwise) {
    rotateFaceGrid(face, clockwise);
    Color temp[3];

    switch (face) {
        case UP:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][0][i];
                for (int i = 0; i < 3; ++i) state[FRONT][0][i] = state[RIGHT][0][i];
                for (int i = 0; i < 3; ++i) state[RIGHT][0][i] = state[BACK][0][i];
                for (int i = 0; i < 3; ++i) state[BACK][0][i] = state[LEFT][0][i];
                for (int i = 0; i < 3; ++i) state[LEFT][0][i] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][0][i];
                for (int i = 0; i < 3; ++i) state[FRONT][0][i] = state[LEFT][0][i];
                for (int i = 0; i < 3; ++i) state[LEFT][0][i] = state[BACK][0][i];
                for (int i = 0; i < 3; ++i) state[BACK][0][i] = state[RIGHT][0][i];
                for (int i = 0; i < 3; ++i) state[RIGHT][0][i] = temp[i];
            }
            break;
        case DOWN:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][2][i];
                for (int i = 0; i < 3; ++i) state[FRONT][2][i] = state[LEFT][2][i];
                for (int i = 0; i < 3; ++i) state[LEFT][2][i] = state[BACK][2][i];
                for (int i = 0; i < 3; ++i) state[BACK][2][i] = state[RIGHT][2][i];
                for (int i = 0; i < 3; ++i) state[RIGHT][2][i] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][2][i];
                for (int i = 0; i < 3; ++i) state[FRONT][2][i] = state[RIGHT][2][i];
                for (int i = 0; i < 3; ++i) state[RIGHT][2][i] = state[BACK][2][i];
                for (int i = 0; i < 3; ++i) state[BACK][2][i] = state[LEFT][2][i];
                for (int i = 0; i < 3; ++i) state[LEFT][2][i] = temp[i];
            }
            break;
        case LEFT:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][i][0];
                for (int i = 0; i < 3; ++i) state[FRONT][i][0] = state[UP][i][0];
                for (int i = 0; i < 3; ++i) state[UP][i][0] = state[BACK][2 - i][2];
                for (int i = 0; i < 3; ++i) state[BACK][2 - i][2] = state[DOWN][i][0];
                for (int i = 0; i < 3; ++i) state[DOWN][i][0] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][i][0];
                for (int i = 0; i < 3; ++i) state[FRONT][i][0] = state[DOWN][i][0];
                for (int i = 0; i < 3; ++i) state[DOWN][i][0] = state[BACK][2 - i][2];
                for (int i = 0; i < 3; ++i) state[BACK][2 - i][2] = state[UP][i][0];
                for (int i = 0; i < 3; ++i) state[UP][i][0] = temp[i];
            }
            break;
        case RIGHT:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][i][2];
                for (int i = 0; i < 3; ++i) state[FRONT][i][2] = state[DOWN][i][2];
                for (int i = 0; i < 3; ++i) state[DOWN][i][2] = state[BACK][2 - i][0];
                for (int i = 0; i < 3; ++i) state[BACK][2 - i][0] = state[UP][i][2];
                for (int i = 0; i < 3; ++i) state[UP][i][2] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[FRONT][i][2];
                for (int i = 0; i < 3; ++i) state[FRONT][i][2] = state[UP][i][2];
                for (int i = 0; i < 3; ++i) state[UP][i][2] = state[BACK][2 - i][0];
                for (int i = 0; i < 3; ++i) state[BACK][2 - i][0] = state[DOWN][i][2];
                for (int i = 0; i < 3; ++i) state[DOWN][i][2] = temp[i];
            }
            break;
        case FRONT:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[UP][2][i];
                for (int i = 0; i < 3; ++i) state[UP][2][i] = state[LEFT][2 - i][2];
                for (int i = 0; i < 3; ++i) state[LEFT][2 - i][2] = state[DOWN][0][2 - i];
                for (int i = 0; i < 3; ++i) state[DOWN][0][2 - i] = state[RIGHT][i][0];
                for (int i = 0; i < 3; ++i) state[RIGHT][i][0] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[UP][2][i];
                for (int i = 0; i < 3; ++i) state[UP][2][i] = state[RIGHT][i][0];
                for (int i = 0; i < 3; ++i) state[RIGHT][i][0] = state[DOWN][0][2 - i];
                for (int i = 0; i < 3; ++i) state[DOWN][0][2 - i] = state[LEFT][2 - i][2];
                for (int i = 0; i < 3; ++i) state[LEFT][2 - i][2] = temp[i];
            }
            break;
        case BACK:
            if (clockwise) {
                for (int i = 0; i < 3; ++i) temp[i] = state[UP][0][i];
                for (int i = 0; i < 3; ++i) state[UP][0][i] = state[RIGHT][i][2];
                for (int i = 0; i < 3; ++i) state[RIGHT][i][2] = state[DOWN][2][2 - i];
                for (int i = 0; i < 3; ++i) state[DOWN][2][2 - i] = state[LEFT][2 - i][0];
                for (int i = 0; i < 3; ++i) state[LEFT][2 - i][0] = temp[i];
            } else {
                for (int i = 0; i < 3; ++i) temp[i] = state[UP][0][i];
                for (int i = 0; i < 3; ++i) state[UP][0][i] = state[LEFT][2 - i][0];
                for (int i = 0; i < 3; ++i) state[LEFT][2 - i][0] = state[DOWN][2][2 - i];
                for (int i = 0; i < 3; ++i) state[DOWN][2][2 - i] = state[RIGHT][i][2];
                for (int i = 0; i < 3; ++i) state[RIGHT][i][2] = temp[i];
            }
            break;
    }
}

void Cube::scramble(int moves) {
    for (int i = 0; i < moves; ++i) {
        rotateFace(static_cast<Face>(std::rand() % 6), std::rand() % 2);
    }
}

bool Cube::isSolved() const {
    for (int f = 0; f < 6; ++f) {
        Color c = state[f][0][0];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                if (state[f][i][j] != c) return false;
            }
        }
    }
    return true;
}

Color Cube::getColor(Face face, int row, int col) const {
    return state[face][row][col];
}
