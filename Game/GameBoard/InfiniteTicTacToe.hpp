#pragma once

#include "GameBoard.hpp"
#include "AI/TicTacToeBot.hpp"
#include "../GameStates.hpp"
#include <SFML/Graphics.hpp>
#include <algorithm>
#include <memory>
#include <chrono>
#include <random>
#include <cmath>
#include <sstream>
#include <iomanip>

using namespace std;
using namespace sf;


class InfiniteTicTacToe {
    private:
        // Игровое состояние
        GameBoard board;
        GameMode mode;
        Cell currentPlayer;
        int winningLength;
        int targetScore;
        mutable chrono::milliseconds initialTimeLimit;
        mutable chrono::milliseconds playerXTimeLeft;
        mutable chrono::milliseconds playerOTimeLeft;
        mutable chrono::steady_clock::time_point turnStartTime;
        mutable bool isTimerRunning;
        mutable Cell playerWithTimerRunning;
        
        // Графика
        float cellSize;
        Vector2f center;
        
        // Игровая логика
        OpponentType opponentType;
        unique_ptr<TicTacToeBot> bot;
        BotDifficulty botDifficulty;
        chrono::steady_clock::time_point lastMoveTime;
        
        // Счет и события
        int playerXScore;
        int playerOScore;
        int playerXBaseScore;
        int playerOBaseScore;
        int playerXBonusScore;
        int playerOBonusScore;
        
        mt19937 rng;
        uniform_int_distribution<int> eventChance;
        vector<Position> moveHistory;
        vector<Position> winLine;
        RandomEvent nextEvent;
        
        bool isBotTurn;
        bool gameWon;
        bool gameEndedByScore;
        Cell winner;
        
        // Проверка победы
        mutable array<bool, 4> checkDirections;
        Position lastCheckedPos;
        
        // Графический кэш
        mutable bool graphicsDirty;
        mutable VertexArray gridVertices;
        mutable VertexArray xVertices;
        mutable VertexArray oVertices;
        mutable VertexArray highlightVertices;
        
        // Вспомогательные методы
        mutable unordered_map<pair<int, int>, bool, PositionHash> visited;
        bool checkLine(const Position &start, int dx, int dy, int length, Cell player) const;
        vector<Position> getWinningLine(const Position &start, int dx, int dy, int length, Cell player) const;
        bool checkWin(const Position &lastMove);
        void expandBoardIfNeeded(const Position &newPos);
        RandomEvent generateRandomEvent();
        void handleRandomEvent(RandomEvent event);
        void swapAllCells();
        void updateTotalScores();
        void calculateBaseScores();
        void calculateBoardScores();
        int findMaxLineScore(const Position &startPos, Cell player);
        int countInDirection(const Position &start, int dx, int dy, Cell player) const;
        void updateGraphics() const;

        void startTimerForPlayer(Cell player) const;
        void stopTimer() const;
        void updateTimers() const;

    public:
        InfiniteTicTacToe(GameMode mode = GameMode::CLASSIC, int winningLength = 5, int targetScore = 100,
                          chrono::seconds timeLimit = chrono::seconds(10), Vector2f windowCenter = Vector2f(400, 300),
                          OpponentType oppType = OpponentType::PLAYER_VS_PLAYER, BotDifficulty botDiff = BotDifficulty::MEDIUM);
        
        // Основные методы
        bool handleClick(const Vector2f &mousePos);
        void makeBotMove();
        void draw(RenderWindow &window) const;
        void drawUI(RenderWindow &window, const Font &font) const;
        
        // Геттеры
        bool isGameWon() const;
        bool isBotCurrentTurn() const;
        Cell getCurrentPlayer() const;
        pair<int, int> getScore() const;
        GameMode getGameMode() const;
        int getTargetScore() const;
        chrono::seconds getTimeLimit() const;
        bool isTimeUp() const;
        
        // Сброс и настройка
        void reset();
        void reset(GameMode newMode, int newWinningLength, int newTargetScore, chrono::seconds newTimeLimit);
        void forceGraphicsUpdate();
        void setCellSize(float size);
        void setCenter(const Vector2f &newCenter);
};
