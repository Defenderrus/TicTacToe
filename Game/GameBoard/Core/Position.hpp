#pragma once

#include <SFML/System.hpp>
#include <cmath>
#include <functional>

using namespace std;
using namespace sf;


struct PositionHash {
    size_t operator()(const pair<int, int> &p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

struct Position {
    int x, y;
    Position(int x = 0, int y = 0);
    
    bool operator==(const Position &other) const;
    bool operator<(const Position &other) const;
    
    Vector2f toPixel(float cellSize, Vector2f center) const;
    Vector2f toCorner(float cellSize, Vector2f center) const;
    float distanceTo(const Position &other) const;
    pair<int, int> toPair() const;
};
