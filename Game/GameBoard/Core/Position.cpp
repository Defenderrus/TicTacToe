#include "Position.hpp"


Position::Position(int x, int y): x(x), y(y) {}

bool Position::operator==(const Position &other) const { return x == other.x && y == other.y; }
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

pair<int, int> Position::toPair() const { return {x, y}; }
