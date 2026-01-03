#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <cmath>
#include <array>
#include <unordered_map>
#include <set>

using namespace std;
using namespace sf;

// Перечисления и структуры
enum class Cell { EMPTY, X, O };
enum class GameMode { CLASSIC, TIMED, SCORING, RANDOM_EVENTS };
enum class RandomEvent { NOTHING, SCORE_PLUS_10, SCORE_MINUS_10, SCORE_PLUS_25, SCORE_MINUS_25, BONUS_MOVE, SWAP_PLAYERS, CLEAR_AREA };
enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER, SCORE_SELECTION, TIME_SELECTION };

// Хэш-функция для Position
struct PositionHash {
    size_t operator()(const pair<int, int> &p) const {
        return hash<int>()(p.first) ^ (hash<int>()(p.second) << 1);
    }
};

struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    
    bool operator==(const Position &other) const { return x == other.x && y == other.y; }
    bool operator<(const Position &other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
    
    Vector2f toPixel(float cellSize, Vector2f center) const {
        return Vector2f(center.x + (x + 0.5f) * cellSize, center.y + (y + 0.5f) * cellSize);
    }
    
    Vector2f toCorner(float cellSize, Vector2f center) const {
        return Vector2f(center.x + x * cellSize, center.y + y * cellSize);
    }
    
    float distanceTo(const Position &other) const {
        int dx = x - other.x;
        int dy = y - other.y;
        return sqrt(dx*dx + dy*dy);
    }
    
    pair<int, int> toPair() const { return {x, y}; }
};

// Хэш-таблица для игрового поля
class GameBoard {
    private:
        unordered_map<pair<int, int>, Cell, PositionHash> cells;
        int minX, maxX, minY, maxY;

        void updateBounds(const Position &pos) {
            minX = min(minX, pos.x);
            maxX = max(maxX, pos.x);
            minY = min(minY, pos.y);
            maxY = max(maxY, pos.y);
        }
    public:
        GameBoard(): minX(-2), maxX(1), minY(-2), maxY(1) { cells.reserve(1024); }
        
        Cell& operator[](const Position &pos) {
            updateBounds(pos);
            return cells[pos.toPair()];
        }
        
        Cell get(const Position &pos) const {
            auto it = cells.find(pos.toPair());
            return it != cells.end() ? it->second : Cell::EMPTY;
        }
        
        bool contains(const Position &pos) const {
            return cells.find(pos.toPair()) != cells.end();
        }
        
        void set(const Position &pos, Cell cell) {
            if (cell == Cell::EMPTY) {
                cells.erase(pos.toPair());
            } else {
                updateBounds(pos);
                cells[pos.toPair()] = cell;
            }
        }
        
        void erase(const Position &pos) {
            cells.erase(pos.toPair());
        }
        
        void clear() {
            cells.clear();
            minX = -2; maxX = 1;
            minY = -2; maxY = 1;
        }
        
        size_t size() const { return cells.size(); }
        
        vector<Position> getOccupiedPositions() const {
            vector<Position> result;
            result.reserve(cells.size());
            for (const auto &entry : cells) {
                result.emplace_back(entry.first.first, entry.first.second);
            }
            return result;
        }
        
        vector<Position> getOccupiedPositions(Cell cellType) const {
            vector<Position> result;
            for (const auto &entry : cells) {
                if (entry.second == cellType) {
                    result.emplace_back(entry.first.first, entry.first.second);
                }
            }
            return result;
        }
        
        void getBounds(int &minXOut, int &maxXOut, int &minYOut, int &maxYOut) const {
            minXOut = minX;
            maxXOut = maxX;
            minYOut = minY;
            maxYOut = maxY;
        }
        
        // Подсчет процента заполнения
        double getFillPercentage() const {
            if (minX > maxX || minY > maxY) return 0.0;
            
            int width = maxX - minX + 1;
            int height = maxY - minY + 1;
            int totalCells = width * height;
            
            if (totalCells == 0) return 0.0;
            
            return (double)cells.size() / totalCells * 100.0;
        }
        
        // Проверка на необходимость расширения (только при >85% или у границы)
        bool shouldExpand(const Position &newPos) const {
            if (getFillPercentage() >= 85.0) return true;

            const int borderThreshold = 0;
            return (newPos.x <= minX + borderThreshold || newPos.x >= maxX - borderThreshold ||
                    newPos.y <= minY + borderThreshold || newPos.y >= maxY - borderThreshold);
        }
        
        // Метод для расширения поля
        void expandField() {
            minX -= 1;
            maxX += 1;
            minY -= 1;
            maxY += 1;
        }
};

// Класс кнопки
class Button {
    private:
        RectangleShape shape;
        Text text;
        Color normalColor;
        Color hoverColor;
        bool wasPressed;

        void updateTextPosition() {
            FloatRect textBounds = text.getLocalBounds();
            Vector2f pos = shape.getPosition();
            Vector2f size = shape.getSize();
            text.setPosition(Vector2f(pos.x + (size.x - textBounds.size.x) / 2, pos.y + (size.y - textBounds.size.y) / 2 - 5));
        }
    public:
        Button(const Vector2f &position, const Vector2f &size, const Font &font, const String &label, unsigned int charSize = 20):
            wasPressed(false), text(font, label, charSize) {

            shape.setSize(size);
            shape.setPosition(position);
            shape.setFillColor(Color(70, 70, 70));
            shape.setOutlineThickness(2);
            shape.setOutlineColor(Color(100, 100, 100));

            text.setFillColor(Color::White);
            updateTextPosition();
            
            normalColor = Color(70, 70, 70);
            hoverColor = Color(100, 100, 100);
        }
        
        void update(const Vector2f &mousePos) {
            bool isHovered = shape.getGlobalBounds().contains(mousePos);
            shape.setFillColor(isHovered ? hoverColor : normalColor);
            shape.setOutlineColor(isHovered ? Color(150, 150, 150) : Color(100, 100, 100));
        }
        
        void draw(RenderWindow &window) const {
            window.draw(shape);
            window.draw(text);
        }
        
        bool isClicked(const Vector2f &mousePos, bool mousePressed) {
            bool currentlyHovered = shape.getGlobalBounds().contains(mousePos);
            
            if (currentlyHovered && mousePressed && !wasPressed) {
                wasPressed = true;
                return true;
            }
            
            if (!mousePressed) {
                wasPressed = false;
            }
            
            return false;
        }
        
        void resetState() {
            wasPressed = false;
            shape.setFillColor(normalColor);
            shape.setOutlineColor(Color(100, 100, 100));
        }
        
        void setPosition(const Vector2f &position) {
            shape.setPosition(position);
            updateTextPosition();
        }
        
        void setLabel(const String &newLabel) {
            text.setString(newLabel);
            updateTextPosition();
        }
        
        void setSize(const Vector2f &newSize) {
            shape.setSize(newSize);
            updateTextPosition();
        }
        
        FloatRect getGlobalBounds() const {
            return shape.getGlobalBounds();
        }
};

// Класс игры
class InfiniteTicTacToe {
    private:
        GameBoard board;
        Cell currentPlayer;
        GameMode mode;
        int winningLength;
        int targetScore;
        chrono::seconds moveTimeLimit;
        
        // Графика
        float cellSize;
        Vector2f center;
        
        // Игровая логика
        chrono::steady_clock::time_point lastMoveTime;
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
        RandomEvent lastEvent;
        bool gameWon;
        bool gameEndedByScore;
        Cell winner;
        
        // Проверка победы
        mutable array<bool, 4> checkDirections;
        Position lastCheckedPos;
        
        // Кэш для графики
        mutable bool graphicsDirty;
        mutable VertexArray gridVertices;
        mutable VertexArray xVertices;
        mutable VertexArray oVertices;
        mutable VertexArray highlightVertices;
        
        // Для отслеживания посещенных позиций при поиске линий
        mutable unordered_map<pair<int, int>, bool, PositionHash> visited;
        
        // Функции проверки победы
        bool checkLine(const Position &start, int dx, int dy, int length, Cell player) const {
            for (int i = 0; i < length; i++) {
                Position pos(start.x + dx * i, start.y + dy * i);
                if (board.get(pos) != player) {
                    return false;
                }
            }
            return true;
        }
        
        vector<Position> getWinningLine(const Position &start, int dx, int dy, int length, Cell player) const {
            vector<Position> line;
            for (int i = 0; i < length; i++) {
                line.emplace_back(start.x + dx * i, start.y + dy * i);
            }
            return line;
        }
        
        bool checkWin(const Position &lastMove) {
            if (lastMove == lastCheckedPos && !winLine.empty()) {
                return true;
            }
            
            Cell player = board.get(lastMove);
            if (player == Cell::EMPTY) return false;
            
            // Направления: горизонталь, вертикаль, диагонали
            constexpr array<pair<int, int>, 4> directions = {{
                {1, 0}, {0, 1}, {1, 1}, {1, -1}
            }};
            
            for (size_t dirIdx = 0; dirIdx < directions.size(); dirIdx++) {
                if (!checkDirections[dirIdx]) continue;
                
                int dx = directions[dirIdx].first;
                int dy = directions[dirIdx].second;
                
                for (int start = -winningLength + 1; start <= 0; start++) {
                    Position startPos(lastMove.x + dx * start, lastMove.y + dy * start);
                    if (checkLine(startPos, dx, dy, winningLength, player)) {
                        winLine = getWinningLine(startPos, dx, dy, winningLength, player);
                        lastCheckedPos = lastMove;
                        
                        // В классическом режиме и режиме с таймером - победа по линии
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
        
        // Автоматическое расширение поля
        void expandBoardIfNeeded(const Position &newPos) {
            if (board.shouldExpand(newPos)) {
                board.expandField();
                graphicsDirty = true;
            }
        }
        
        // Случайные события
        RandomEvent generateRandomEvent() {
            if (mode != GameMode::RANDOM_EVENTS) return RandomEvent::NOTHING;
            
            int roll = eventChance(rng);
            
            // Вероятности событий
            if (roll < 35) return RandomEvent::NOTHING;
            else if (roll < 50) return RandomEvent::SCORE_PLUS_10;
            else if (roll < 65) return RandomEvent::SCORE_MINUS_10;
            else if (roll < 70) return RandomEvent::SCORE_PLUS_25;
            else if (roll < 75) return RandomEvent::SCORE_MINUS_25;
            else if (roll < 90) return RandomEvent::BONUS_MOVE;
            else if (roll < 95) return RandomEvent::CLEAR_AREA;
            else return RandomEvent::SWAP_PLAYERS;
        }
        
        void handleRandomEvent(RandomEvent event) {
            lastEvent = event;

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
                    // Не меняем игрока - бонусный ход
                    return;
                case RandomEvent::SWAP_PLAYERS:
                    // Смена игроков
                    swapAllCells();
                    graphicsDirty = true;
                    calculateBaseScores();
                    updateTotalScores();
                    break;
                case RandomEvent::CLEAR_AREA:
                    if (!moveHistory.empty()) {
                        Position last = moveHistory.back();
                        
                        // Удаляем область 3x3
                        for (int dx = -1; dx <= 1; dx++) {
                            for (int dy = -1; dy <= 1; dy++) {
                                Position pos(last.x + dx, last.y + dy);
                                board.erase(pos);
                            }
                        }
                        graphicsDirty = true;
                        
                        // Пересчитываем очки после очистки
                        calculateBaseScores();
                        updateTotalScores();
                    }
                    break;
            }
        }
        
        // Смена всех клеток на противоположные
        void swapAllCells() {
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
        
        // Обновление общих очков
        void updateTotalScores() {
            // Обновляем общие очки: базовые + бонусы
            playerXScore = playerXBaseScore + playerXBonusScore;
            playerOScore = playerOBaseScore + playerOBonusScore;
            
            // Не допускаем отрицательный общий счет
            playerXScore = max(0, playerXScore);
            playerOScore = max(0, playerOScore);
            
            // Проверяем достижение целевого счета
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
        
        // Подсчет базовых очков за изолированные линии
        void calculateBaseScores() {
            // Сбрасываем базовые очки
            playerXBaseScore = 0;
            playerOBaseScore = 0;
            
            // Очищаем visited
            visited.clear();
            
            // Получаем все занятые позиции
            vector<Position> xPositions = board.getOccupiedPositions(Cell::X);
            vector<Position> oPositions = board.getOccupiedPositions(Cell::O);
            
            // Находим все изолированные линии для X
            for (const auto &pos : xPositions) {
                if (visited.find(pos.toPair()) == visited.end()) {
                    playerXBaseScore += findMaxLineScore(pos, Cell::X);
                }
            }
            
            // Очищаем visited
            visited.clear();
            
            // Находим все изолированные линии для O
            for (const auto &pos : oPositions) {
                if (visited.find(pos.toPair()) == visited.end()) {
                    playerOBaseScore += findMaxLineScore(pos, Cell::O);
                }
            }
            
            // Для режима на очки обновляем общий счет
            if (mode == GameMode::SCORING) {
                playerXScore = playerXBaseScore;
                playerOScore = playerOBaseScore;
                
                // Проверяем достижение цели
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
        
        // Подсчет очков
        void calculateBoardScores() {
            if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) {
                calculateBaseScores();
                if (mode == GameMode::RANDOM_EVENTS) updateTotalScores();
            }
        }
        
        // Найти максимальную линию и вернуть очки за нее
        int findMaxLineScore(const Position &startPos, Cell player) {
            constexpr array<pair<int, int>, 4> directions = {{ {1, 0}, {0, 1}, {1, 1}, {1, -1} }};
            
            int maxLineLength = 0;
            vector<Position> maxLinePositions;
            
            // Для каждого направления ищем максимальную непрерывную линию
            for (const auto &dir : directions) {
                int dx = dir.first;
                int dy = dir.second;
                
                // Ищем в обе стороны
                int forwardLength = countInDirection(startPos, dx, dy, player);
                int backwardLength = countInDirection(startPos, -dx, -dy, player);
                int totalLength = forwardLength + backwardLength - 1;
                
                if (totalLength > maxLineLength) {
                    maxLineLength = totalLength;
                    maxLinePositions.clear();
                    
                    // Собираем позиции линии
                    for (int i = -(backwardLength - 1); i < forwardLength; i++) {
                        Position pos(startPos.x + dx * i, startPos.y + dy * i);
                        maxLinePositions.push_back(pos);
                    }
                }
            }
            
            if (maxLineLength > 0) {
                // Помечаем все клетки этой линии как посещенные
                for (const auto &pos : maxLinePositions) {
                    visited[pos.toPair()] = true;
                }
                
                return maxLineLength * maxLineLength;
            }
            
            return 0;
        }
        
        int countInDirection(const Position &start, int dx, int dy, Cell player) const {
            int count = 0;
            Position current = start;
            
            while (board.get(current) == player) {
                count++;
                current = Position(current.x + dx, current.y + dy);
            }
            
            return count;
        }
        
        // Обновление графики
        void updateGraphics() const {
            if (!graphicsDirty && !winLine.empty()) return;
            
            gridVertices.clear();
            xVertices.clear();
            oVertices.clear();
            highlightVertices.clear();
            
            // Определяем видимую область на основе границ поля
            int minX, maxX, minY, maxY;
            board.getBounds(minX, maxX, minY, maxY);
            
            // Добавляем отступ для видимой области
            int visibleMargin = 1;
            int visibleMinX = minX - visibleMargin;
            int visibleMaxX = maxX + visibleMargin;
            int visibleMinY = minY - visibleMargin;
            int visibleMaxY = maxY + visibleMargin;
            
            // Ограничиваем максимальную видимую область
            const int maxVisible = 30;
            if (visibleMaxX - visibleMinX > maxVisible) {
                visibleMinX = -maxVisible/2;
                visibleMaxX = maxVisible/2;
            }
            if (visibleMaxY - visibleMinY > maxVisible) {
                visibleMinY = -maxVisible/2;
                visibleMaxY = maxVisible/2;
            }
            
            const float halfCell = cellSize * 0.5f;
            const float offset = cellSize * 0.35f;
            
            // Сетка
            Color gridColor(100, 100, 100, 100);
            
            // Вертикальные линии
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
            
            // Горизонтальные линии
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
            
            // Крестики и нолики
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
            
            // Подсветка победной линии (только для классического режима и режима с таймером)
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
    public:
        InfiniteTicTacToe(GameMode mode = GameMode::CLASSIC, int winningLength = 5, int targetScore = 300,
                        chrono::seconds timeLimit = chrono::seconds(10), Vector2f windowCenter = Vector2f(400, 300)):
                        currentPlayer(Cell::X), mode(mode), winningLength(winningLength), targetScore(targetScore),
                        moveTimeLimit(timeLimit), cellSize(40.0f), center(windowCenter), playerXScore(0), playerOScore(0),
                        playerXBaseScore(0), playerOBaseScore(0), playerXBonusScore(0), playerOBonusScore(0),
                        gameWon(false), gameEndedByScore(false), winner(Cell::EMPTY), lastEvent(RandomEvent::NOTHING),
                        rng(static_cast<unsigned int>(chrono::steady_clock::now().time_since_epoch().count())),
                        eventChance(0, 100), graphicsDirty(true), gridVertices(PrimitiveType::Lines),
                        xVertices(PrimitiveType::Lines), oVertices(PrimitiveType::Lines), highlightVertices(PrimitiveType::TriangleStrip) {
            lastMoveTime = chrono::steady_clock::now();
            checkDirections.fill(true);
        }
        
        // Обработка клика мыши
        bool handleClick(const Vector2f &mousePos) {
            if (gameWon) return false;
            
            int gridX = static_cast<int>(floor((mousePos.x - center.x) / cellSize));
            int gridY = static_cast<int>(floor((mousePos.y - center.y) / cellSize));
            
            Position pos(gridX, gridY);
            
            if (board.get(pos) != Cell::EMPTY) return false;
            
            if (mode == GameMode::TIMED) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastMoveTime);
                if (elapsed > moveTimeLimit) {
                    currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
                    lastMoveTime = now;
                    return false;
                }
            }
            
            board.set(pos, currentPlayer);
            moveHistory.push_back(pos);
            lastMoveTime = chrono::steady_clock::now();
            bool wonByLine = checkWin(pos);
            
            // Расширяем поле если нужно
            expandBoardIfNeeded(pos);
            
            // Для режимов с очками пересчитываем
            if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) {
                calculateBoardScores();
            }
            
            if (gameWon) {
                graphicsDirty = true;
                return true;
            }
            
            // Случайные события
            if (mode == GameMode::RANDOM_EVENTS) {
                RandomEvent event = generateRandomEvent();
                handleRandomEvent(event);
                if (event == RandomEvent::BONUS_MOVE) {
                    // Не меняем игрока при бонусном ходе
                    graphicsDirty = true;
                    return false;
                }
            }
            
            currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
            graphicsDirty = true;
            
            return false;
        }
        
        // Отрисовка
        void draw(RenderWindow &window) const {
            updateGraphics();
            
            // Ось координат
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
            
            // Сетка
            if (gridVertices.getVertexCount() > 0) window.draw(gridVertices);
            
            // Подсветка
            if (highlightVertices.getVertexCount() > 0) window.draw(highlightVertices);
            
            // Крестики
            if (xVertices.getVertexCount() > 0) window.draw(xVertices);
            
            // Нолики
            if (oVertices.getVertexCount() > 0) window.draw(oVertices);
            
            // Центр координат
            CircleShape centerPoint(5);
            centerPoint.setFillColor(Color::Green);
            centerPoint.setPosition(Vector2f(center.x - 2.5f, center.y - 2.5f));
            window.draw(centerPoint);
        }
        
        // Отрисовка интерфейса
        void drawUI(RenderWindow &window, const Font &font) const {
            // Панель информации
            RectangleShape infoPanel;
            infoPanel.setSize(Vector2f(280, 210));
            infoPanel.setFillColor(Color(40, 40, 40, 220));
            infoPanel.setPosition(Vector2f(10, 10));
            window.draw(infoPanel);
            
            // Текущий игрок
            Text currentPlayerText(font, L"Текущий: " + String(currentPlayer == Cell::X ? L"X" : L"O"), 20);
            currentPlayerText.setFillColor(currentPlayer == Cell::X ? Color::Red : Color::Blue);
            currentPlayerText.setPosition(Vector2f(20, 20));
            window.draw(currentPlayerText);
            
            // Режим игры
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
            
            // Длина линии для победы (только для классического режима и режима с таймером)
            if (mode == GameMode::CLASSIC || mode == GameMode::TIMED) {
                Text lengthText(font, L"Линия: " + to_wstring(winningLength), 16);
                lengthText.setFillColor(Color::White);
                lengthText.setPosition(Vector2f(20, 75));
                window.draw(lengthText);
            }
            
            // Счет (для режимов с очками)
            if (mode == GameMode::SCORING || mode == GameMode::RANDOM_EVENTS) {
                Text scoreText(font, L"Счет: X=" + to_wstring(playerXScore) + L" O=" + to_wstring(playerOScore), 16);
                scoreText.setFillColor(Color::White);
                scoreText.setPosition(Vector2f(20, 75));
                window.draw(scoreText);
                
                // В режиме со случайными событиями показываем базовые и бонусные очки отдельно
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
            
            // Таймер
            if (mode == GameMode::TIMED) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastMoveTime);
                auto remaining = moveTimeLimit > elapsed ? moveTimeLimit - elapsed : chrono::seconds(0);
                
                Text timerText(font, L"Время: " + to_wstring(remaining.count()) + L"с", 16);
                timerText.setFillColor(remaining.count() < 5 ? Color::Red : Color::Yellow);
                timerText.setPosition(Vector2f(20, 100));
                window.draw(timerText);
                
                Text limitText(font, L"Лимит: " + to_wstring(moveTimeLimit.count()) + L"с", 16);
                limitText.setFillColor(Color::White);
                limitText.setPosition(Vector2f(20, 125));
                window.draw(limitText);
            }
            
            // Последнее событие
            if (mode == GameMode::RANDOM_EVENTS) {
                String eventStr;
                switch (lastEvent) {
                    case RandomEvent::NOTHING: eventStr = L"Обычный ход"; break;
                    case RandomEvent::SCORE_PLUS_10: eventStr = L"+10 очков"; break;
                    case RandomEvent::SCORE_MINUS_10: eventStr = L"-10 очков"; break;
                    case RandomEvent::SCORE_PLUS_25: eventStr = L"+25 очков"; break;
                    case RandomEvent::SCORE_MINUS_25: eventStr = L"-25 очков"; break;
                    case RandomEvent::BONUS_MOVE: eventStr = L"Бонусный ход!"; break;
                    case RandomEvent::SWAP_PLAYERS: eventStr = L"Смена элементов!"; break;
                    case RandomEvent::CLEAR_AREA: eventStr = L"Очистка области!"; break;
                }
                
                Text eventText(font, L"Событие: " + eventStr, 14);
                eventText.setFillColor(Color::Cyan);
                eventText.setPosition(Vector2f(20, 175));
                window.draw(eventText);
            }
            
            // Сообщение о победе или окончании игры
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
                } else {
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
        
        // Геттеры
        bool isGameWon() const { return gameWon; }
        Cell getCurrentPlayer() const { return currentPlayer; }
        pair<int, int> getScore() const { return {playerXScore, playerOScore}; }
        GameMode getGameMode() const { return mode; }
        int getTargetScore() const { return targetScore; }
        chrono::seconds getTimeLimit() const { return moveTimeLimit; }
        
        // Сброс игры
        void reset() {
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
            lastEvent = RandomEvent::NOTHING;
            lastMoveTime = chrono::steady_clock::now();
            graphicsDirty = true;
            checkDirections.fill(true);
            cellSize = 40.0f;
            visited.clear();
        }
        
        void reset(GameMode newMode, int newWinningLength, int newTargetScore, chrono::seconds newTimeLimit) {
            mode = newMode;
            winningLength = newWinningLength;
            targetScore = newTargetScore;
            moveTimeLimit = newTimeLimit;
            reset();
        }
        
        void forceGraphicsUpdate() {
            graphicsDirty = true;
            updateGraphics();
        }
        
        void setCellSize(float size) {
            cellSize = size;
            graphicsDirty = true;
        }
        
        void setCenter(const Vector2f &newCenter) {
            center = newCenter;
            graphicsDirty = true;
        }
};

// Главный класс игры
class Game {
    private:
        RenderWindow window;
        GameState currentState;
        unique_ptr<InfiniteTicTacToe> game;
        Font font;
        
        // Кнопки меню
        vector<Button> menuButtons;
        vector<Button> pauseButtons;
        vector<Button> scoreButtons;
        vector<Button> timeButtons;
        
        // Выбранный режим и параметры
        GameMode selectedMode;
        int selectedLength;
        int selectedScoreTarget;
        chrono::seconds selectedTimeLimit;
        
        // Камера
        View gameView;
        View uiView;
        Vector2f viewCenter;
        float zoomLevel;
        
        // Оптимизация ввода
        Clock inputClock;
        float inputCooldown = 0.1f;
        
        // Оптимизация отрисовки
        Clock frameClock;
        float targetFPS = 60.0f;
        float frameTime = 1.0f / targetFPS;
        
        // Размеры окна
        Vector2u windowSize;
        
        void setupMenu() {
            menuButtons.clear();
            
            const float buttonWidth = 320;
            const float buttonHeight = 50;
            const float startY = 150;
            const float spacing = 60;
            
            menuButtons.reserve(5);
            menuButtons.emplace_back(Vector2f(240, startY), Vector2f(buttonWidth, buttonHeight), font, L"Классический режим", 22);
            menuButtons.emplace_back(Vector2f(240, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"Режим с таймером", 22);
            menuButtons.emplace_back(Vector2f(240, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"Режим на очки", 22);
            menuButtons.emplace_back(Vector2f(240, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Режим со случайными событиями", 20);
            menuButtons.emplace_back(Vector2f(240, startY + spacing * 4), Vector2f(buttonWidth, buttonHeight), font, L"Выход", 22);
        }
        
        void setupPauseMenu() {
            pauseButtons.clear();
            pauseButtons.reserve(3);
            
            const float buttonWidth = 240;
            const float buttonHeight = 50;
            const float startY = 250;
            
            pauseButtons.emplace_back(Vector2f(280, startY), Vector2f(buttonWidth, buttonHeight), font, L"Продолжить", 22);
            pauseButtons.emplace_back(Vector2f(280, startY + 60), Vector2f(buttonWidth, buttonHeight), font, L"Новая игра", 22);
            pauseButtons.emplace_back(Vector2f(280, startY + 120), Vector2f(buttonWidth, buttonHeight), font, L"В главное меню", 22);
        }
        
        void setupScoreSelection() {
            scoreButtons.clear();
            scoreButtons.reserve(4);
            
            const float buttonWidth = 240;
            const float buttonHeight = 50;
            const float startY = 200;
            const float spacing = 60;
            
            scoreButtons.emplace_back(Vector2f(280, startY), Vector2f(buttonWidth, buttonHeight), font, L"300 очков", 22);
            scoreButtons.emplace_back(Vector2f(280, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"500 очков", 22);
            scoreButtons.emplace_back(Vector2f(280, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"1000 очков", 22);
            scoreButtons.emplace_back(Vector2f(280, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Назад", 22);
        }
        
        void setupTimeSelection() {
            timeButtons.clear();
            timeButtons.reserve(4);
            
            const float buttonWidth = 240;
            const float buttonHeight = 50;
            const float startY = 200;
            const float spacing = 60;
            
            timeButtons.emplace_back(Vector2f(280, startY), Vector2f(buttonWidth, buttonHeight), font, L"5 секунд", 22);
            timeButtons.emplace_back(Vector2f(280, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"10 секунд", 22);
            timeButtons.emplace_back(Vector2f(280, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"20 секунд", 22);
            timeButtons.emplace_back(Vector2f(280, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Назад", 22);
        }
        
        void updateButtonsForWindowSize() {
            Vector2u newSize = window.getSize();
            if (newSize == windowSize) return;
            
            windowSize = newSize;
            viewCenter = Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f);
            
            uiView.setSize(Vector2f(windowSize));
            uiView.setCenter(Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f));
            
            gameView.setSize(Vector2f(windowSize));
            gameView.setCenter(viewCenter);
            
            if (!menuButtons.empty()) {
                float centerX = windowSize.x / 2.0f;
                
                menuButtons[0].setPosition(Vector2f(centerX - 160, 150));
                menuButtons[1].setPosition(Vector2f(centerX - 160, 210));
                menuButtons[2].setPosition(Vector2f(centerX - 160, 270));
                menuButtons[3].setPosition(Vector2f(centerX - 160, 330));
                menuButtons[4].setPosition(Vector2f(centerX - 160, 390));
            }
            
            if (!pauseButtons.empty()) {
                float centerX = windowSize.x / 2.0f;
                
                pauseButtons[0].setPosition(Vector2f(centerX - 120, 250));
                pauseButtons[1].setPosition(Vector2f(centerX - 120, 310));
                pauseButtons[2].setPosition(Vector2f(centerX - 120, 370));
            }
            
            if (!scoreButtons.empty()) {
                float centerX = windowSize.x / 2.0f;
                
                scoreButtons[0].setPosition(Vector2f(centerX - 120, 200));
                scoreButtons[1].setPosition(Vector2f(centerX - 120, 260));
                scoreButtons[2].setPosition(Vector2f(centerX - 120, 320));
                scoreButtons[3].setPosition(Vector2f(centerX - 120, 380));
            }
            
            if (!timeButtons.empty()) {
                float centerX = windowSize.x / 2.0f;
                
                timeButtons[0].setPosition(Vector2f(centerX - 120, 200));
                timeButtons[1].setPosition(Vector2f(centerX - 120, 260));
                timeButtons[2].setPosition(Vector2f(centerX - 120, 320));
                timeButtons[3].setPosition(Vector2f(centerX - 120, 380));
            }
            
            if (game) {
                game->setCenter(viewCenter);
            }
        }
        
        void startGame(GameMode mode, int length, int scoreTarget = 300, chrono::seconds timeLimit = chrono::seconds(10)) {
            selectedMode = mode;
            selectedLength = length;
            selectedScoreTarget = scoreTarget;
            selectedTimeLimit = timeLimit;
            
            game = make_unique<InfiniteTicTacToe>(mode, length, scoreTarget, timeLimit, viewCenter);
            currentState = GameState::PLAYING;
            
            gameView.setCenter(viewCenter);
            gameView.setSize(Vector2f(window.getSize()));
            zoomLevel = 1.0f;
        }
        
        bool checkInputCooldown() {
            if (inputClock.getElapsedTime().asSeconds() >= inputCooldown) {
                inputClock.restart();
                return true;
            }
            return false;
        }
    public:
        Game(): window(VideoMode({800, 600}), L"Крестики-Нолики", Style::Default), currentState(GameState::MENU), 
                viewCenter(400, 300), zoomLevel(1.0f), selectedLength(5), selectedScoreTarget(300),
                selectedTimeLimit(chrono::seconds(10)), windowSize(800, 600) {
            window.setFramerateLimit(static_cast<unsigned int>(targetFPS));
            
            if (!font.openFromFile("arial.ttf")) {
                if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
                    if (!font.openFromFile("C:/Windows/Fonts/tahoma.ttf")) {
                        cerr << L"Не удалось загрузить шрифт!" << endl;
                    }
                }
            }
            
            uiView = View(Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f), Vector2f(windowSize));
            gameView = View(viewCenter, Vector2f(windowSize));
            
            setupMenu();
            setupPauseMenu();
            setupScoreSelection();
            setupTimeSelection();
        }
        
        void run() {
            while (window.isOpen()) {
                processEvents();
                update();
                render();
            }
        }
    private:
        void processEvents() {
            optional<Event> event;
            while ((event = window.pollEvent())) {
                if (event->is<Event::Closed>()) {
                    window.close();
                }
                else if (auto resized = event->getIf<Event::Resized>()) {
                    Vector2u newSize(resized->size.x, resized->size.y);
                    window.setView(View(Vector2f(newSize.x / 2.0f, newSize.y / 2.0f), Vector2f(newSize)));
                    updateButtonsForWindowSize();
                }
                else {
                    switch (currentState) {
                        case GameState::MENU: handleMenuInput(*event); break;
                        case GameState::PLAYING: handleGameInput(*event); break;
                        case GameState::PAUSED: handlePauseInput(*event); break;
                        case GameState::GAME_OVER: handleGameOverInput(*event); break;
                        case GameState::SCORE_SELECTION: handleScoreSelectionInput(*event); break;
                        case GameState::TIME_SELECTION: handleTimeSelectionInput(*event); break;
                    }
                }
            }
        }
        
        void handleMenuInput(const Event &event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
            bool mousePressed = false;
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Left) mousePressed = true;
            }
            
            for (auto &button : menuButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &menuButtons[0];
                    if (index < 4) {
                        selectedMode = static_cast<GameMode>(index);
                        if (selectedMode == GameMode::SCORING || selectedMode == GameMode::RANDOM_EVENTS) {
                            currentState = GameState::SCORE_SELECTION;
                        } else if (selectedMode == GameMode::TIMED) {
                            currentState = GameState::TIME_SELECTION;
                        } else {
                            startGame(selectedMode, selectedLength);
                        }
                    } else if (index == 4) {
                        window.close();
                    }
                }
            }
            
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (keyPress->code == Keyboard::Key::Escape) {
                    window.close();
                }
            }
        }
        
        void handleGameInput(const Event &event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), gameView);
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Left && checkInputCooldown()) {
                    if (game && !game->isGameWon()) {
                        bool gameEnded = game->handleClick(mousePos);
                        if (gameEnded) currentState = GameState::GAME_OVER;
                    }
                }
            }
            
            if (auto mouseWheel = event.getIf<Event::MouseWheelScrolled>()) {
                if (mouseWheel->delta > 0) zoomLevel *= 0.9f;
                else zoomLevel *= 1.1f;
                gameView.setSize(Vector2f(windowSize) * (1.0f / zoomLevel));
            }
            
            static bool isDragging = false;
            static Vector2f lastMousePos;
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Right) {
                    isDragging = true;
                    lastMousePos = mousePos;
                }
            }
            
            if (auto mouseRelease = event.getIf<Event::MouseButtonReleased>()) {
                if (mouseRelease->button == Mouse::Button::Right) {
                    isDragging = false;
                }
            }
            
            if (auto mouseMove = event.getIf<Event::MouseMoved>()) {
                if (isDragging) {
                    Vector2f delta = lastMousePos - mousePos;
                    gameView.move(delta);
                    lastMousePos = window.mapPixelToCoords(Mouse::getPosition(window), gameView);
                }
            }
            
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (!checkInputCooldown()) return;
                
                switch (keyPress->code) {
                    case Keyboard::Key::Escape:
                        currentState = GameState::PAUSED;
                        break;
                    case Keyboard::Key::R:
                        if (game) game->reset(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                        break;
                    case Keyboard::Key::Add:
                        zoomLevel *= 0.9f;
                        gameView.zoom(0.9f);
                        break;
                    case Keyboard::Key::Subtract:
                        zoomLevel *= 1.1f;
                        gameView.zoom(1.1f);
                        break;
                    default: break;
                }
            }
        }
        
        void handlePauseInput(const Event &event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
            bool mousePressed = false;
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Left) mousePressed = true;
            }
            
            for (auto &button : pauseButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &pauseButtons[0];
                    if (index == 0) {
                        currentState = GameState::PLAYING;
                    } else if (index == 1) {
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 2) {
                        currentState = GameState::MENU;
                        game.reset();
                    }
                }
            }
            
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (keyPress->code == Keyboard::Key::Escape) currentState = GameState::PLAYING;
            }
        }
        
        void handleGameOverInput(const Event &event) {
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (!checkInputCooldown()) return;
                
                if (keyPress->code == Keyboard::Key::Enter) {
                    if (game) game->reset(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    currentState = GameState::PLAYING;
                } else if (keyPress->code == Keyboard::Key::Escape) {
                    currentState = GameState::MENU;
                    game.reset();
                }
            }
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (checkInputCooldown()) {
                    if (game) game->reset(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    currentState = GameState::PLAYING;
                }
            }
        }
        
        void handleScoreSelectionInput(const Event &event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
            bool mousePressed = false;
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Left) mousePressed = true;
            }
            
            for (auto &button : scoreButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &scoreButtons[0];
                    if (index == 0) {
                        selectedScoreTarget = 300;
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 1) {
                        selectedScoreTarget = 500;
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 2) {
                        selectedScoreTarget = 1000;
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 3) {
                        currentState = GameState::MENU;
                    }
                }
            }
            
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (keyPress->code == Keyboard::Key::Escape) currentState = GameState::MENU;
            }
        }
        
        void handleTimeSelectionInput(const Event &event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
            bool mousePressed = false;
            
            if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
                if (mousePress->button == Mouse::Button::Left) mousePressed = true;
            }
            
            for (auto &button : timeButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &timeButtons[0];
                    if (index == 0) {
                        selectedTimeLimit = chrono::seconds(5);
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 1) {
                        selectedTimeLimit = chrono::seconds(10);
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 2) {
                        selectedTimeLimit = chrono::seconds(20);
                        startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit);
                    } else if (index == 3) {
                        currentState = GameState::MENU;
                    }
                }
            }
            
            if (auto keyPress = event.getIf<Event::KeyPressed>()) {
                if (keyPress->code == Keyboard::Key::Escape) currentState = GameState::MENU;
            }
        }
        
        void update() {
            float elapsed = frameClock.restart().asSeconds();
            if (elapsed < frameTime) sleep(seconds(frameTime - elapsed));
        }
        
        void render() {
            window.clear(Color(30, 30, 30));
            
            switch (currentState) {
                case GameState::MENU: drawMenu(); break;
                case GameState::PLAYING: drawGame(); break;
                case GameState::PAUSED: drawGame(); drawPauseMenu(); break;
                case GameState::GAME_OVER: drawGame(); break;
                case GameState::SCORE_SELECTION: drawScoreSelection(); break;
                case GameState::TIME_SELECTION: drawTimeSelection(); break;
            }
            
            window.display();
        }
        
        void drawMenu() {
            window.setView(uiView);
            
            RectangleShape background;
            background.setSize(Vector2f(windowSize));
            background.setFillColor(Color(20, 20, 40));
            background.setPosition(Vector2f(0, 0));
            window.draw(background);
            
            Text title(font, L"КРЕСТИКИ-НОЛИКИ", 42);
            title.setFillColor(Color::Cyan);
            title.setStyle(Text::Bold);
            FloatRect titleBounds = title.getLocalBounds();
            title.setPosition(Vector2f(windowSize.x / 2.0f - titleBounds.size.x / 2, 50));
            window.draw(title);
            
            Text subtitle(font, L"Выберите режим игры", 28);
            subtitle.setFillColor(Color::White);
            FloatRect subtitleBounds = subtitle.getLocalBounds();
            subtitle.setPosition(Vector2f(windowSize.x / 2.0f - subtitleBounds.size.x / 2, 110));
            window.draw(subtitle);
            
            for (auto &button : menuButtons) {
                button.draw(window);
            }
            
            Text hints(font, L"Управление в игре:\n"
                             L"ЛКМ - сделать ход\n"
                             L"ПКМ - двигать камеру\n"
                             L"ESC - пауза/меню\n"
                             L"R - перезапуск\n"
                             L"+/- - масштабирование\n", 16);
            hints.setFillColor(Color(150, 150, 150));
            hints.setPosition(Vector2f(windowSize.x - 180, 150));
            window.draw(hints);
        }
        
        void drawGame() {
            window.setView(gameView);
            if (game) game->draw(window);
            
            window.setView(uiView);
            if (game) game->drawUI(window, font);
            
            Text instructions(font, L"Управление в игре:\n"
                             L"ЛКМ - сделать ход\n"
                             L"ПКМ - двигать камеру\n"
                             L"ESC - пауза/меню\n"
                             L"R - перезапуск\n"
                             L"+/- - масштабирование\n", 16);
            instructions.setFillColor(Color(150, 150, 150));
            instructions.setPosition(Vector2f(windowSize.x - 180, 150));
            window.draw(instructions);
        }
        
        void drawPauseMenu() {
            window.setView(uiView);
            
            RectangleShape overlay;
            overlay.setSize(Vector2f(windowSize));
            overlay.setFillColor(Color(0, 0, 0, 150));
            overlay.setPosition(Vector2f(0, 0));
            window.draw(overlay);
            
            Text pauseText(font, L"ПАУЗА", 52);
            pauseText.setFillColor(Color::Yellow);
            pauseText.setStyle(Text::Bold);
            FloatRect pauseBounds = pauseText.getLocalBounds();
            pauseText.setPosition(Vector2f(windowSize.x / 2.0f - pauseBounds.size.x / 2, 150));
            window.draw(pauseText);
            
            for (auto &button : pauseButtons) {
                button.draw(window);
            }
        }
        
        void drawScoreSelection() {
            window.setView(uiView);
            
            RectangleShape background;
            background.setSize(Vector2f(windowSize));
            background.setFillColor(Color(20, 20, 40));
            background.setPosition(Vector2f(0, 0));
            window.draw(background);
            
            Text title(font, L"ВЫБЕРИТЕ ЦЕЛЕВОЙ СЧЕТ", 36);
            title.setFillColor(Color::Cyan);
            title.setStyle(Text::Bold);
            FloatRect titleBounds = title.getLocalBounds();
            title.setPosition(Vector2f(windowSize.x / 2.0f - titleBounds.size.x / 2, 80));
            window.draw(title);
            
            String modeStr;
            switch (selectedMode) {
                case GameMode::SCORING: modeStr = L"Режим на очки"; break;
                case GameMode::RANDOM_EVENTS: modeStr = L"Режим со случайными событиями"; break;
                default: modeStr = L"Режим";
            }
            
            Text subtitle(font, modeStr, 28);
            subtitle.setFillColor(Color::White);
            FloatRect subtitleBounds = subtitle.getLocalBounds();
            subtitle.setPosition(Vector2f(windowSize.x / 2.0f - subtitleBounds.size.x / 2, 130));
            window.draw(subtitle);
            
            for (auto &button : scoreButtons) {
                button.draw(window);
            }
            
            Text hint(font, L"Очки за изолированную линию = (длина линии)²", 16);
            hint.setFillColor(Color::Yellow);
            FloatRect hintBounds = hint.getLocalBounds();
            hint.setPosition(Vector2f(windowSize.x / 2.0f - hintBounds.size.x / 2, 450));
            window.draw(hint);
        }
        
        void drawTimeSelection() {
            window.setView(uiView);
            
            RectangleShape background;
            background.setSize(Vector2f(windowSize));
            background.setFillColor(Color(20, 20, 40));
            background.setPosition(Vector2f(0, 0));
            window.draw(background);
            
            Text title(font, L"ВЫБЕРИТЕ ВРЕМЯ НА ХОД", 36);
            title.setFillColor(Color::Cyan);
            title.setStyle(Text::Bold);
            FloatRect titleBounds = title.getLocalBounds();
            title.setPosition(Vector2f(windowSize.x / 2.0f - titleBounds.size.x / 2, 80));
            window.draw(title);
            
            Text subtitle(font, L"Режим с таймером", 28);
            subtitle.setFillColor(Color::White);
            FloatRect subtitleBounds = subtitle.getLocalBounds();
            subtitle.setPosition(Vector2f(windowSize.x / 2.0f - subtitleBounds.size.x / 2, 130));
            window.draw(subtitle);
            
            for (auto &button : timeButtons) {
                button.draw(window);
            }
            
            Text hint(font, L"Если время истекает, ход переходит другому игроку", 18);
            hint.setFillColor(Color::Yellow);
            FloatRect hintBounds = hint.getLocalBounds();
            hint.setPosition(Vector2f(windowSize.x / 2.0f - hintBounds.size.x / 2, 450));
            window.draw(hint);
        }
};

int main() {
    Game game;
    game.run();
    return 0;
}
