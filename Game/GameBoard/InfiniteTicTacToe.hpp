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

        chrono::milliseconds initialTimeLimit;

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

        // RandomEvent lastEvent;
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
        
        // Вспомогательные методы реализации
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

        void startTimerForPlayer(Cell player) const; ////////
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

bool InfiniteTicTacToe::checkLine(const Position &start, int dx, int dy, int length, Cell player) const {
    for (int i = 0; i < length; i++) {
        Position pos(start.x + dx * i, start.y + dy * i);
        if (board.get(pos) != player) {
            return false;
        }
    }
    return true;
}

vector<Position> InfiniteTicTacToe::getWinningLine(const Position &start, int dx, int dy, int length, Cell player) const {
    vector<Position> line;
    for (int i = 0; i < length; i++) {
        line.emplace_back(start.x + dx * i, start.y + dy * i);
    }
    return line;
}

bool InfiniteTicTacToe::checkWin(const Position &lastMove) {
    if (lastMove == lastCheckedPos && !winLine.empty()) return true;
    
    Cell player = board.get(lastMove);
    if (player == Cell::EMPTY) return false;
    
    constexpr array<pair<int, int>, 4> directions = {{ {1, 0}, {0, 1}, {1, 1}, {1, -1} }};
    
    for (size_t dirIdx = 0; dirIdx < directions.size(); dirIdx++) {
        if (!checkDirections[dirIdx]) continue;
        
        int dx = directions[dirIdx].first;
        int dy = directions[dirIdx].second;
        
        for (int start = -winningLength + 1; start <= 0; start++) {
            Position startPos(lastMove.x + dx * start, lastMove.y + dy * start);
            if (checkLine(startPos, dx, dy, winningLength, player)) {
                winLine = getWinningLine(startPos, dx, dy, winningLength, player);
                lastCheckedPos = lastMove;
                
                if (mode == GameMode::CLASSIC || mode == GameMode::TIMED) {
                    gameWon = true;
                    winner = player;
                    gameEndedByScore = false;
                }
                return true;
            }
        }
    }
    
    lastCheckedPos = lastMove;
    return false;
}

void InfiniteTicTacToe::expandBoardIfNeeded(const Position &newPos) {
    if (board.shouldExpand(newPos)) {
        board.expandField();
        graphicsDirty = true;
    }
}

RandomEvent InfiniteTicTacToe::generateRandomEvent() {
    if (mode != GameMode::RANDOM_EVENTS) return RandomEvent::NOTHING;
    
    int roll = eventChance(rng);
    
    if (roll < 35) return RandomEvent::NOTHING;
    else if (roll < 50) return RandomEvent::SCORE_PLUS_10;
    else if (roll < 65) return RandomEvent::SCORE_MINUS_10;
    else if (roll < 70) return RandomEvent::SCORE_PLUS_25;
    else if (roll < 75) return RandomEvent::SCORE_MINUS_25;
    else if (roll < 90) return RandomEvent::BONUS_MOVE;
    else if (roll < 95) return RandomEvent::CLEAR_AREA;
    else return RandomEvent::SWAP_PLAYERS;
}

void InfiniteTicTacToe::handleRandomEvent(RandomEvent event) {
    // lastEvent = event;

    switch (event) {
        case RandomEvent::NOTHING:
            updateTotalScores();
            break;
        case RandomEvent::SCORE_PLUS_10:
            if (currentPlayer == Cell::X) playerXBonusScore += 10;
            else playerOBonusScore += 10;
            updateTotalScores();
            break;
        case RandomEvent::SCORE_MINUS_10:
            if (currentPlayer == Cell::X) playerXBonusScore -= 10;
            else playerOBonusScore -= 10;
            updateTotalScores();
            break;
        case RandomEvent::SCORE_PLUS_25:
            if (currentPlayer == Cell::X) playerXBonusScore += 25;
            else playerOBonusScore += 25;
            updateTotalScores();
            break;
        case RandomEvent::SCORE_MINUS_25:
            if (currentPlayer == Cell::X) playerXBonusScore -= 25;
            else playerOBonusScore -= 25;
            updateTotalScores();
            break;
        case RandomEvent::BONUS_MOVE:
            return;
        case RandomEvent::SWAP_PLAYERS:
            swapAllCells();
            graphicsDirty = true;
            calculateBaseScores();
            updateTotalScores();
            break;
        case RandomEvent::CLEAR_AREA:
            if (!moveHistory.empty()) {
                Position last = moveHistory.back();
                
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        Position pos(last.x + dx, last.y + dy);
                        board.erase(pos);
                    }
                }
                graphicsDirty = true;
                
                calculateBaseScores();
                updateTotalScores();
            }
            break;
    }
}

void InfiniteTicTacToe::swapAllCells() {
    auto occupied = board.getOccupiedPositions();
    for (const auto &pos : occupied) {
        Cell current = board.get(pos);
        if (current == Cell::X) {
            board.set(pos, Cell::O);
        } else if (current == Cell::O) {
            board.set(pos, Cell::X);
        }
    }
}

void InfiniteTicTacToe::updateTotalScores() {
    playerXScore = playerXBaseScore + playerXBonusScore;
    playerOScore = playerOBaseScore + playerOBonusScore;
    
    playerXScore = max(0, playerXScore);
    playerOScore = max(0, playerOScore);
    
    if (playerXScore >= targetScore) {
        gameWon = true;
        winner = Cell::X;
        gameEndedByScore = true;
    } else if (playerOScore >= targetScore) {
        gameWon = true;
        winner = Cell::O;
        gameEndedByScore = true;
    }
}

void InfiniteTicTacToe::calculateBaseScores() {
    playerXBaseScore = 0;
    playerOBaseScore = 0;
    
    visited.clear();
    
    vector<Position> xPositions = board.getOccupiedPositions(Cell::X);
    vector<Position> oPositions = board.getOccupiedPositions(Cell::O);
    
    for (const auto &pos : xPositions) {
        if (visited.find(pos.toPair()) == visited.end()) {
            playerXBaseScore += findMaxLineScore(pos, Cell::X);
        }
    }
    
    visited.clear();
    
    for (const auto &pos : oPositions) {
        if (visited.find(pos.toPair()) == visited.end()) {
            playerOBaseScore += findMaxLineScore(pos, Cell::O);
        }
    }
    
    if (mode == GameMode::SCORING) {
        playerXScore = playerXBaseScore;
        playerOScore = playerOBaseScore;
        
        if (playerXScore >= targetScore) {
            gameWon = true;
            winner = Cell::X;
            gameEndedByScore = true;
        } else if (playerOScore >= targetScore) {
            gameWon = true;
            winner = Cell::O;
            gameEndedByScore = true;
        }
    }
}

void InfiniteTicTacToe::calculateBoardScores() {
    if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) {
        calculateBaseScores();
        if (mode == GameMode::RANDOM_EVENTS) updateTotalScores();
    }
}

int InfiniteTicTacToe::findMaxLineScore(const Position &startPos, Cell player) {
    constexpr array<pair<int, int>, 4> directions = {{ {1, 0}, {0, 1}, {1, 1}, {1, -1} }};
    
    int maxLineLength = 0;
    vector<Position> maxLinePositions;
    
    for (const auto &dir : directions) {
        int dx = dir.first;
        int dy = dir.second;
        
        int forwardLength = countInDirection(startPos, dx, dy, player);
        int backwardLength = countInDirection(startPos, -dx, -dy, player);
        int totalLength = forwardLength + backwardLength - 1;
        
        if (totalLength > maxLineLength) {
            maxLineLength = totalLength;
            maxLinePositions.clear();
            
            for (int i = -(backwardLength - 1); i < forwardLength; i++) {
                Position pos(startPos.x + dx * i, startPos.y + dy * i);
                maxLinePositions.push_back(pos);
            }
        }
    }
    
    if (maxLineLength > 0) {
        for (const auto &pos : maxLinePositions) {
            visited[pos.toPair()] = true;
        }
        
        return maxLineLength * maxLineLength;
    }
    
    return 0;
}

int InfiniteTicTacToe::countInDirection(const Position &start, int dx, int dy, Cell player) const {
    int count = 0;
    Position current = start;
    
    while (board.get(current) == player) {
        count++;
        current = Position(current.x + dx, current.y + dy);
    }
    
    return count;
}

void InfiniteTicTacToe::updateGraphics() const {
    if (!graphicsDirty && !winLine.empty()) return;
    
    gridVertices.clear();
    xVertices.clear();
    oVertices.clear();
    highlightVertices.clear();
    
    int minX, maxX, minY, maxY;
    board.getBounds(minX, maxX, minY, maxY);
    
    int visibleMargin = 1;
    int visibleMinX = minX - visibleMargin;
    int visibleMaxX = maxX + visibleMargin;
    int visibleMinY = minY - visibleMargin;
    int visibleMaxY = maxY + visibleMargin;
    
    const float halfCell = cellSize * 0.5f;
    const float offset = cellSize * 0.35f;
    
    Color gridColor(100, 100, 100, 100);
    
    for (int x = visibleMinX; x <= visibleMaxX + 1; x++) {
        float pixelX = center.x + x * cellSize;
        Vertex v1, v2;
        v1.position = Vector2f(pixelX, center.y + visibleMinY * cellSize);
        v1.color = gridColor;
        v2.position = Vector2f(pixelX, center.y + (visibleMaxY + 1) * cellSize);
        v2.color = gridColor;
        gridVertices.append(v1);
        gridVertices.append(v2);
    }
    
    for (int y = visibleMinY; y <= visibleMaxY + 1; y++) {
        float pixelY = center.y + y * cellSize;
        Vertex v1, v2;
        v1.position = Vector2f(center.x + visibleMinX * cellSize, pixelY);
        v1.color = gridColor;
        v2.position = Vector2f(center.x + (visibleMaxX + 1) * cellSize, pixelY);
        v2.color = gridColor;
        gridVertices.append(v1);
        gridVertices.append(v2);
    }
    
    for (int x = visibleMinX; x <= visibleMaxX; x++) {
        for (int y = visibleMinY; y <= visibleMaxY; y++) {
            Position pos(x, y);
            Cell cell = board.get(pos);
            
            if (cell == Cell::EMPTY) continue;
            
            Vector2f pixelPos = pos.toPixel(cellSize, center);
            
            if (cell == Cell::X) {
                Color xColor = Color::Red;
                
                Vertex v1, v2;
                v1.position = Vector2f(pixelPos.x - offset, pixelPos.y - offset);
                v1.color = xColor;
                v2.position = Vector2f(pixelPos.x + offset, pixelPos.y + offset);
                v2.color = xColor;
                xVertices.append(v1);
                xVertices.append(v2);
                
                Vertex v3, v4;
                v3.position = Vector2f(pixelPos.x + offset, pixelPos.y - offset);
                v3.color = xColor;
                v4.position = Vector2f(pixelPos.x - offset, pixelPos.y + offset);
                v4.color = xColor;
                xVertices.append(v3);
                xVertices.append(v4);
            } 
            else if (cell == Cell::O) {
                Color oColor = Color::Blue;
                const int segments = 24;
                float radius = cellSize * 0.25f;
                
                for (int i = 0; i < segments; i++) {
                    float angle1 = 2 * 3.14159f * i / segments;
                    float angle2 = 2 * 3.14159f * (i + 1) / segments;
                    
                    Vertex v1, v2;
                    v1.position = Vector2f(
                        pixelPos.x + cos(angle1) * radius, 
                        pixelPos.y + sin(angle1) * radius
                    );
                    v1.color = oColor;
                    v2.position = Vector2f(
                        pixelPos.x + cos(angle2) * radius, 
                        pixelPos.y + sin(angle2) * radius
                    );
                    v2.color = oColor;
                    oVertices.append(v1);
                    oVertices.append(v2);
                }
            }
        }
    }
    
    if (!winLine.empty() && (mode == GameMode::CLASSIC || mode == GameMode::TIMED)) {
        Color highlightColor(255, 255, 0, 100);
        for (const auto &pos : winLine) {
            Vector2f pixelPos = pos.toPixel(cellSize, center);
            Vertex v1, v2, v3, v4;
            v1.position = Vector2f(pixelPos.x - halfCell + 2, pixelPos.y - halfCell + 2);
            v1.color = highlightColor;
            v2.position = Vector2f(pixelPos.x + halfCell - 2, pixelPos.y - halfCell + 2);
            v2.color = highlightColor;
            v3.position = Vector2f(pixelPos.x - halfCell + 2, pixelPos.y + halfCell - 2);
            v3.color = highlightColor;
            v4.position = Vector2f(pixelPos.x + halfCell - 2, pixelPos.y + halfCell - 2);
            v4.color = highlightColor;
            
            highlightVertices.append(v1);
            highlightVertices.append(v2);
            highlightVertices.append(v3);
            highlightVertices.append(v4);
        }
    }
    
    graphicsDirty = false;
}

void InfiniteTicTacToe::stopTimer() const{

    if (mode != GameMode::TIMED || !isTimerRunning) return;

    updateTimers();
    
    isTimerRunning = false;
    playerWithTimerRunning = Cell::EMPTY;
}

void InfiniteTicTacToe::updateTimers() const {

    if (mode != GameMode::TIMED || !isTimerRunning) return;
    
    auto now = chrono::steady_clock::now();
    auto elapsed = chrono::duration_cast<chrono::milliseconds>(now - turnStartTime);


    
    if (elapsed.count() > 0) {
        if (playerWithTimerRunning == Cell::X) {
            playerXTimeLeft -= elapsed;
            if (playerXTimeLeft.count() < 0) playerXTimeLeft = chrono::milliseconds(0);
        } 
        else if (playerWithTimerRunning == Cell::O) {
            playerOTimeLeft -= elapsed;
            if (playerOTimeLeft.count() < 0) playerOTimeLeft = chrono::milliseconds(0);
        }
    }
    
    turnStartTime = now;
}

InfiniteTicTacToe::InfiniteTicTacToe(GameMode mode, int winningLength, int targetScore, chrono::seconds timeLimit,
    Vector2f windowCenter, OpponentType oppType, BotDifficulty botDiff):
        currentPlayer(Cell::X), 
        mode(mode), 
        winningLength(winningLength), 
        targetScore(targetScore), 
        cellSize(40.0f), 
        center(windowCenter), 
        opponentType(oppType),
        botDifficulty(botDiff), 
        isBotTurn(false), 
        playerXScore(0), 
        playerOScore(0),
        playerXBaseScore(0), 
        playerOBaseScore(0), 
        playerXBonusScore(0), 
        playerOBonusScore(0),
        gameWon(false), 
        gameEndedByScore(false), 
        winner(Cell::EMPTY), 
        // lastEvent(RandomEvent::NOTHING),
        nextEvent(RandomEvent::NOTHING),
        rng(static_cast<unsigned int>(chrono::steady_clock::now().time_since_epoch().count())),
        eventChance(0, 100), 
        graphicsDirty(true), 
        gridVertices(PrimitiveType::Lines),
        xVertices(PrimitiveType::Lines), 
        oVertices(PrimitiveType::Lines), 
        highlightVertices(PrimitiveType::TriangleStrip),

        initialTimeLimit(chrono::duration_cast<chrono::milliseconds>(timeLimit)),
        playerXTimeLeft(chrono::duration_cast<chrono::milliseconds>(timeLimit)),         // Инициализируем время
        playerOTimeLeft(chrono::duration_cast<chrono::milliseconds>(timeLimit)),
        isTimerRunning(false),
        playerWithTimerRunning(Cell::EMPTY) {

    lastMoveTime = chrono::steady_clock::now();
    checkDirections.fill(true);

    // Запускаем таймер для первого игрока
    if (mode == GameMode::TIMED) {
        startTimerForPlayer(currentPlayer);
    }

    if (opponentType == OpponentType::PLAYER_VS_BOT) {
        bot = make_unique<TicTacToeBot>(botDiff, currentPlayer);
        isBotTurn = true;
    }
}

void InfiniteTicTacToe::startTimerForPlayer(Cell player) const{
    if (mode != GameMode::TIMED) return;
    
    if (isTimerRunning) {
        stopTimer();
    }

    turnStartTime = chrono::steady_clock::now();
    isTimerRunning = true;
    playerWithTimerRunning = player;
}

bool InfiniteTicTacToe::handleClick(const Vector2f &mousePos) {
    if (gameWon) return false;
    if (opponentType == OpponentType::PLAYER_VS_BOT && isBotTurn) return false;
    
    int gridX = static_cast<int>(floor((mousePos.x - center.x) / cellSize));
    int gridY = static_cast<int>(floor((mousePos.y - center.y) / cellSize));
    
    Position pos(gridX, gridY);
    
    if (board.get(pos) != Cell::EMPTY) return false;

    board.set(pos, currentPlayer);
    moveHistory.push_back(pos);
    lastMoveTime = chrono::steady_clock::now();
    bool wonByLine = checkWin(pos);
    
    expandBoardIfNeeded(pos);
    if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) calculateBoardScores();
    if (gameWon) {
        if (mode == GameMode::TIMED) {
            stopTimer();
        }
        graphicsDirty = true;
        return true;
    }
    
    if (mode == GameMode::RANDOM_EVENTS  && nextEvent != RandomEvent::NOTHING) {
        RandomEvent event = nextEvent;
        handleRandomEvent(event);

        nextEvent = RandomEvent::NOTHING;

        if (event == RandomEvent::BONUS_MOVE) {
            // nextEvent = generateRandomEvent();
            graphicsDirty = true;
            return false;
        }
    }
    
    if (mode == GameMode::TIMED) {
        stopTimer();
    }

    currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;

    if (mode == GameMode::RANDOM_EVENTS) {
            nextEvent = generateRandomEvent();
    }

    if (mode == GameMode::TIMED) {
        startTimerForPlayer(currentPlayer);
    }

    if (opponentType == OpponentType::PLAYER_VS_BOT) isBotTurn = true;
    graphicsDirty = true;
    
    return false;
}

void InfiniteTicTacToe::makeBotMove() {
    if (!bot || !isBotTurn || gameWon) return;

    Position botMove = bot->getBestMove(board, winningLength);

    board.set(botMove, currentPlayer);
    moveHistory.push_back(botMove);
    lastMoveTime = chrono::steady_clock::now();
    bool wonByLine = checkWin(botMove);
    graphicsDirty = true;
    expandBoardIfNeeded(botMove);
    if (gameWon) return;

    currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
    isBotTurn = false;
}

void InfiniteTicTacToe::draw(RenderWindow &window) const {
    updateGraphics();
    
    VertexArray axes(PrimitiveType::Lines, 4);
    axes[0].position = Vector2f(0, center.y);
    axes[0].color = Color(150, 150, 150, 150);
    axes[1].position = Vector2f(window.getSize().x, center.y);
    axes[1].color = Color(150, 150, 150, 150);
    axes[2].position = Vector2f(center.x, 0);
    axes[2].color = Color(150, 150, 150, 150);
    axes[3].position = Vector2f(center.x, window.getSize().y);
    axes[3].color = Color(150, 150, 150, 150);
    window.draw(axes);
    
    if (gridVertices.getVertexCount() > 0) window.draw(gridVertices);
    if (highlightVertices.getVertexCount() > 0) window.draw(highlightVertices);
    if (xVertices.getVertexCount() > 0) window.draw(xVertices);
    if (oVertices.getVertexCount() > 0) window.draw(oVertices);
    
    CircleShape centerPoint(5);
    centerPoint.setFillColor(Color::Green);
    centerPoint.setPosition(Vector2f(center.x - 2.5f, center.y - 2.5f));
    window.draw(centerPoint);
}

void InfiniteTicTacToe::drawUI(RenderWindow &window, const Font &font) const {
    RectangleShape infoPanel;
    infoPanel.setSize(Vector2f(280, 210));
    infoPanel.setFillColor(Color(40, 40, 40, 220));
    infoPanel.setPosition(Vector2f(10, 10));
    window.draw(infoPanel);
    
    Text currentPlayerText(font, L"Текущий: " + String(currentPlayer == Cell::X ? L"X" : L"O"), 20);
    currentPlayerText.setFillColor(currentPlayer == Cell::X ? Color::Red : Color::Blue);
    currentPlayerText.setPosition(Vector2f(20, 20));
    window.draw(currentPlayerText);
    
    String modeStr;
    switch (mode) {
        case GameMode::CLASSIC: modeStr = L"Классический"; break;
        case GameMode::TIMED: modeStr = L"С таймером"; break;
        case GameMode::SCORING: modeStr = L"Система очков"; break;
        case GameMode::RANDOM_EVENTS: modeStr = L"Случайные события"; break;
    }
    
    Text modeText(font, L"Режим: " + modeStr, 16);
    modeText.setFillColor(Color::White);
    modeText.setPosition(Vector2f(20, 50));
    window.draw(modeText);
    
    if (mode == GameMode::CLASSIC || mode == GameMode::TIMED) {
        Text lengthText(font, L"Линия: " + to_wstring(winningLength), 16);
        lengthText.setFillColor(Color::White);
        lengthText.setPosition(Vector2f(20, 75));
        window.draw(lengthText);
    }

    if (mode == GameMode::CLASSIC) {
        String opponentStr;
        if (opponentType == OpponentType::PLAYER_VS_PLAYER) {
            opponentStr = L"Игрок";
        } else {
            opponentStr = L"Бот";
            String difficultyStr;
            switch (botDifficulty) {
                case BotDifficulty::EASY: difficultyStr = L" (лёгкий)"; break;
                case BotDifficulty::MEDIUM: difficultyStr = L" (средний)"; break;
                case BotDifficulty::HARD: difficultyStr = L" (сложный)"; break;
            }
            opponentStr += difficultyStr;
        }
        
        Text opponentText(font, L"Противник: " + opponentStr, 14);
        opponentText.setFillColor(Color::Green);
        opponentText.setPosition(Vector2f(20, 100));
        window.draw(opponentText);
    }
    
    if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) {
        Text scoreText(font, L"Счет: X=" + to_wstring(playerXScore) + L" O=" + to_wstring(playerOScore), 16);
        scoreText.setFillColor(Color::White);
        scoreText.setPosition(Vector2f(20, 75));
        window.draw(scoreText);
        
        if (mode == GameMode::RANDOM_EVENTS) {
            Text baseText(font, L"Базовые: X=" + to_wstring(playerXBaseScore) + L" O=" + to_wstring(playerOBaseScore), 14);
            baseText.setFillColor(Color::Green);
            baseText.setPosition(Vector2f(20, 100));
            window.draw(baseText);
            
            Text bonusText(font, L"Бонусы: X=" + to_wstring(playerXBonusScore) + L" O=" + to_wstring(playerOBonusScore), 14);
            bonusText.setFillColor(Color::Cyan);
            bonusText.setPosition(Vector2f(20, 125));
            window.draw(bonusText);
            
            Text targetText(font, L"Цель: " + to_wstring(targetScore), 16);
            targetText.setFillColor(Color::Yellow);
            targetText.setPosition(Vector2f(20, 150));
            window.draw(targetText);
        } else {
            Text targetText(font, L"Цель: " + to_wstring(targetScore), 16);
            targetText.setFillColor(Color::Yellow);
            targetText.setPosition(Vector2f(20, 100));
            window.draw(targetText);
        }
    }
    
    if (mode == GameMode::TIMED) { 

    updateTimers();

    float xSeconds = playerXTimeLeft.count() / 1000.0f;
    float oSeconds = playerOTimeLeft.count() / 1000.0f;

    wstringstream xStream, oStream;
    xStream << fixed << setprecision(1) << xSeconds;
    oStream << fixed << setprecision(1) << oSeconds;
    
    Color xColor = (currentPlayer == Cell::X) ? Color::Yellow : Color(200, 200, 200); // - это светлосерый
    Color oColor = (currentPlayer == Cell::O) ? Color::Yellow : Color(200, 200, 200);
    
    // Если время < 10 секунд - красный
    if (playerXTimeLeft.count() < 10000) xColor = Color::Red;
    if (playerOTimeLeft.count() < 10000) oColor = Color::Red;

    Text xTimerText(font, L"X: " + xStream.str() + L"с", 18);
    xTimerText.setFillColor(xColor);
    xTimerText.setPosition(Vector2f(20, 100));
    window.draw(xTimerText);

    Text oTimerText(font, L"O: " + oStream.str() + L"с", 18);
    oTimerText.setFillColor(oColor);
    oTimerText.setPosition(Vector2f(20, 125));
    window.draw(oTimerText);

}
    
    if (mode == GameMode::RANDOM_EVENTS) {
        String eventStr;
        switch (nextEvent) {
            case RandomEvent::NOTHING: eventStr = L"Обычный ход"; break;
            case RandomEvent::SCORE_PLUS_10: eventStr = L"+10 очков"; break;
            case RandomEvent::SCORE_MINUS_10: eventStr = L"-10 очков"; break;
            case RandomEvent::SCORE_PLUS_25: eventStr = L"+25 очков"; break;
            case RandomEvent::SCORE_MINUS_25: eventStr = L"-25 очков"; break;
            case RandomEvent::BONUS_MOVE: eventStr = L"Бонусный ход!"; break;
            case RandomEvent::SWAP_PLAYERS: eventStr = L"Смена элементов!"; break;
            case RandomEvent::CLEAR_AREA: eventStr = L"Очистка области!"; break;
        }
        
        Text eventText(font, L"Cобытие этого хода: " + eventStr, 14);

        eventText.setFillColor(Color::Cyan);
        eventText.setPosition(Vector2f(20, 175));
        window.draw(eventText);
    }
    
    if (gameWon) {
        RectangleShape winPanel;
        winPanel.setSize(Vector2f(420, 160));
        winPanel.setFillColor(Color(0, 0, 0, 220));
        winPanel.setPosition(Vector2f(
            window.getSize().x / 2.0f - 210, 
            window.getSize().y / 2.0f - 80
        ));
        window.draw(winPanel);
        
        String winMessage;
        if (gameEndedByScore) {
            winMessage = L"Игрок " + String(winner == Cell::X ? L"X" : L"O") + 
                        L" достиг цели в " + to_wstring(targetScore) + L" очков!\n" +
                        L"Финальный счет: X=" + to_wstring(playerXScore) + 
                        L" O=" + to_wstring(playerOScore);
        } 
        else if (mode == GameMode::TIMED  && winLine.empty()) {

            winMessage = L" Игрок " + String(winner == Cell::X ? L"X" : L"O") + 
                        L" выиграл по времени!\n" +
                        L"У противника закончилось время.";
        }
        else {
            winMessage = L"Игрок " + String(winner == Cell::X ? L"X" : L"O") + 
                        L" собрал линию из " + to_wstring(winningLength) + L" элементов!";
        }
        
        Text winText(font, winMessage, 22);
        winText.setFillColor(winner == Cell::X ? Color::Red : Color::Blue);
        winText.setLineSpacing(1.2f);
        
        FloatRect textBounds = winText.getLocalBounds();
        winText.setPosition(Vector2f(
            window.getSize().x / 2.0f - textBounds.size.x / 2, 
            window.getSize().y / 2.0f - 50
        ));
        window.draw(winText);
        
        Text restartText(font, L"Нажмите Enter для новой игры\nили ESC для выхода в меню", 18);
        restartText.setFillColor(Color::White);
        textBounds = restartText.getLocalBounds();
        restartText.setPosition(Vector2f(
            window.getSize().x / 2.0f - textBounds.size.x / 2, 
            window.getSize().y / 2.0f + 20
        ));
        window.draw(restartText);
    }
}

bool InfiniteTicTacToe::isGameWon() const { return gameWon; }
bool InfiniteTicTacToe::isBotCurrentTurn() const { return isBotTurn; }
Cell InfiniteTicTacToe::getCurrentPlayer() const { return currentPlayer; }
pair<int, int> InfiniteTicTacToe::getScore() const { return {playerXScore, playerOScore}; }
GameMode InfiniteTicTacToe::getGameMode() const { return mode; }
int InfiniteTicTacToe::getTargetScore() const { return targetScore; }
chrono::seconds InfiniteTicTacToe::getTimeLimit() const { return chrono::duration_cast<chrono::seconds>(initialTimeLimit); }

void InfiniteTicTacToe::reset() {
    board.clear();
    moveHistory.clear();
    moveHistory.reserve(100);
    winLine.clear();
    currentPlayer = Cell::X;
    playerXScore = playerOScore = 0;
    playerXBaseScore = playerOBaseScore = 0;
    playerXBonusScore = playerOBonusScore = 0;
    gameWon = false;
    gameEndedByScore = false;
    winner = Cell::EMPTY;
    // lastEvent = RandomEvent::NOTHING;
    nextEvent = RandomEvent::NOTHING;
    lastMoveTime = chrono::steady_clock::now();
    graphicsDirty = true;
    checkDirections.fill(true);
    cellSize = 40.0f;
    visited.clear();

    if (mode == GameMode::TIMED) {

        playerXTimeLeft = initialTimeLimit;
        playerOTimeLeft = initialTimeLimit;
        isTimerRunning = false;
        playerWithTimerRunning = Cell::EMPTY;
        
        startTimerForPlayer(currentPlayer);
    }

    if (opponentType == OpponentType::PLAYER_VS_BOT) {
        if (!bot) {
            bot = make_unique<TicTacToeBot>(botDifficulty, currentPlayer);
        } else {
            bot->setSymbol(currentPlayer);
            bot->setDifficulty(botDifficulty);
        }
        isBotTurn = true;
    } else {
        isBotTurn = false;
    }
}

void InfiniteTicTacToe::reset(GameMode newMode, int newWinningLength, int newTargetScore, chrono::seconds newTimeLimit) {
    mode = newMode;
    winningLength = newWinningLength;
    targetScore = newTargetScore;

    initialTimeLimit = chrono::duration_cast<chrono::milliseconds>(newTimeLimit);

    playerXTimeLeft = newTimeLimit;
    playerOTimeLeft = newTimeLimit;

    // playerXTimeLeft = chrono::duration_cast<chrono::milliseconds>(newTimeLimit);
    // playerOTimeLeft = chrono::duration_cast<chrono::milliseconds>(newTimeLimit);

    reset();
}

void InfiniteTicTacToe::forceGraphicsUpdate() {
    graphicsDirty = true;
    updateGraphics();
}

void InfiniteTicTacToe::setCellSize(float size) {
    cellSize = size;
    graphicsDirty = true;
}

void InfiniteTicTacToe::setCenter(const Vector2f &newCenter) {
    center = newCenter;
    graphicsDirty = true;
}

bool InfiniteTicTacToe::isTimeUp() const {

    updateTimers();
    
    if (playerXTimeLeft.count() <= 0) {

        const_cast<InfiniteTicTacToe*>(this)->gameWon = true;
        const_cast<InfiniteTicTacToe*>(this)->winner = Cell::O;  // X проиграл по времени
        const_cast<InfiniteTicTacToe*>(this)->gameEndedByScore = false;

        return true;
    }
    if (playerOTimeLeft.count() <= 0) {

        const_cast<InfiniteTicTacToe*>(this)->gameWon = true;
        const_cast<InfiniteTicTacToe*>(this)->winner = Cell::X;  // O проиграл по времени
        const_cast<InfiniteTicTacToe*>(this)->gameEndedByScore = false;

        return true;
    }
    return false;
}
