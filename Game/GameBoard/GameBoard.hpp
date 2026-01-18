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
