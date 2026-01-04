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

Position::Position(int x, int y): x(x), y(y) {}

bool Position::operator==(const Position &other) const {
    return x == other.x && y == other.y;
}

bool Position::operator<(const Position &other) const {
    if (x != other.x) return x < other.x;
    return y < other.y;
}

Vector2f Position::toPixel(float cellSize, Vector2f center) const {
    return Vector2f(center.x + (x + 0.5f) * cellSize, center.y + (y + 0.5f) * cellSize);
}

Vector2f Position::toCorner(float cellSize, Vector2f center) const {
    return Vector2f(center.x + x * cellSize, center.y + y * cellSize);
}

float Position::distanceTo(const Position &other) const {
    int dx = x - other.x;
    int dy = y - other.y;
    return sqrt(dx*dx + dy*dy);
}

pair<int, int> Position::toPair() const {
    return {x, y};
}
