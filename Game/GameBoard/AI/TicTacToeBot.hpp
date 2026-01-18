#pragma once

#include "../GameBoard.hpp"
#include "../../GameStates.hpp"
#include <optional>
#include <vector>
#include <algorithm>
#include <random>
#include <chrono>
#include <climits>
#include <array>
#include <unordered_set>

using namespace std;


class TicTacToeBot {
    private:
        BotDifficulty difficulty;
        Cell botSymbol;
        Cell opponentSymbol;
        int searchDepth;
        int maxMovesToConsider;
        
        // Методы оценки
        int evaluateLine(const GameBoard &board, const Position &start, int dx, int dy, Cell player, int lineLength) const;
        int evaluatePosition(const GameBoard &board, int lineLength) const;
        int evaluateAllLines(const GameBoard &board, Cell player, int lineLength) const;
        int evaluateCenterControl(const GameBoard &board) const;
        
        // Поиск ходов
        optional<Position> checkImmediateWinOrBlock(const GameBoard &board, int lineLength);
        optional<Position> findWinningMove(const GameBoard &board, Cell player, int lineLength) const;
        bool checkWinForPlayer(const GameBoard &board, Cell player, int lineLength) const;
        
        // Минимакс
        pair<int, Position> minimax(const GameBoard &board, int depth, int alpha, int beta, bool maximizingPlayer, int lineLength);
        
        // Вспомогательные методы
        int evaluateMove(const GameBoard &board, const Position &move) const;
        vector<Position> getPotentialMoves(const GameBoard &board) const;

    public:
        TicTacToeBot(BotDifficulty diff = BotDifficulty::MEDIUM, Cell symbol = Cell::O);
        
        Position getBestMove(const GameBoard &board, int lineLength);
        void setDifficulty(BotDifficulty diff);
        void setSymbol(Cell symbol);
        BotDifficulty getDifficulty() const;
};
