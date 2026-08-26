#ifndef CUBE_H
#define CUBE_H

#include <vector>
#include <string>

enum Color {
    WHITE = 0,
    YELLOW = 1,
    RED = 2,
    ORANGE = 3,
    BLUE = 4,
    GREEN = 5
};

enum Face {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3,
    FRONT = 4,
    BACK = 5
};

class Cube {
public:
    Cube();
    void rotateFace(Face face, bool clockwise);
    void scramble(int moves = 20);
    bool isSolved() const;
    Color getColor(Face face, int row, int col) const;

private:
    Color state[6][3][3];
    void rotateFaceGrid(Face face, bool clockwise);
};

#endif
