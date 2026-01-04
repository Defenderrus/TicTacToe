#pragma once

#include "Core/Position.hpp"
#include "Core/Cell.hpp"
#include <unordered_map>
#include <algorithm>
#include <vector>

using namespace std;


class GameBoard {
    private:
        unordered_map<pair<int, int>, Cell, PositionHash> cells;
        int minX, maxX, minY, maxY;

        void updateBounds(const Position &pos);

    public:
        GameBoard();
        
        Cell& operator[](const Position &pos);
        Cell get(const Position &pos) const;
        bool contains(const Position &pos) const;
        void set(const Position &pos, Cell cell);
        void erase(const Position &pos);
        void clear();
        size_t size() const;
        
        vector<Position> getOccupiedPositions() const;
        vector<Position> getOccupiedPositions(Cell cellType) const;
        void getBounds(int &minXOut, int &maxXOut, int &minYOut, int &maxYOut) const;
        
        double getFillPercentage() const;
        bool shouldExpand(const Position &newPos) const;
        void expandField();
};

void GameBoard::updateBounds(const Position &pos) {
    minX = min(minX, pos.x);
    maxX = max(maxX, pos.x);
    minY = min(minY, pos.y);
    maxY = max(maxY, pos.y);
}

GameBoard::GameBoard(): minX(-2), maxX(1), minY(-2), maxY(1) {
    cells.reserve(1024);
}

Cell& GameBoard::operator[](const Position &pos) {
    updateBounds(pos);
    return cells[pos.toPair()];
}

Cell GameBoard::get(const Position &pos) const {
    auto it = cells.find(pos.toPair());
    return it != cells.end() ? it->second : Cell::EMPTY;
}

bool GameBoard::contains(const Position &pos) const {
    return cells.find(pos.toPair()) != cells.end();
}

void GameBoard::set(const Position &pos, Cell cell) {
    if (cell == Cell::EMPTY) {
        cells.erase(pos.toPair());
    } else {
        updateBounds(pos);
        cells[pos.toPair()] = cell;
    }
}

void GameBoard::erase(const Position &pos) {
    cells.erase(pos.toPair());
}

void GameBoard::clear() {
    cells.clear();
    minX = -2; maxX = 1;
    minY = -2; maxY = 1;
}

size_t GameBoard::size() const {
    return cells.size();
}

vector<Position> GameBoard::getOccupiedPositions() const {
    vector<Position> result;
    result.reserve(cells.size());
    for (const auto &entry : cells) {
        result.emplace_back(entry.first.first, entry.first.second);
    }
    return result;
}

vector<Position> GameBoard::getOccupiedPositions(Cell cellType) const {
    vector<Position> result;
    for (const auto &entry : cells) {
        if (entry.second == cellType) {
            result.emplace_back(entry.first.first, entry.first.second);
        }
    }
    return result;
}

void GameBoard::getBounds(int &minXOut, int &maxXOut, int &minYOut, int &maxYOut) const {
    minXOut = minX;
    maxXOut = maxX;
    minYOut = minY;
    maxYOut = maxY;
}

double GameBoard::getFillPercentage() const {
    if (minX > maxX || minY > maxY) return 0.0;
    
    int width = maxX - minX + 1;
    int height = maxY - minY + 1;
    int totalCells = width * height;
    
    if (totalCells == 0) return 0.0;
    
    return (double)cells.size() / totalCells * 100.0;
}

bool GameBoard::shouldExpand(const Position &newPos) const {
    if (getFillPercentage() >= 85.0) return true;

    const int borderThreshold = 0;
    return (newPos.x <= minX + borderThreshold || newPos.x >= maxX - borderThreshold ||
            newPos.y <= minY + borderThreshold || newPos.y >= maxY - borderThreshold);
}

void GameBoard::expandField() {
    minX -= 1;
    maxX += 1;
    minY -= 1;
    maxY += 1;
}
