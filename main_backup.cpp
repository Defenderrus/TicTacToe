#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/System.hpp>
#include <iostream>
#include <functional>
#include <memory>
#include <vector>
#include <random>
#include <chrono>
#include <unordered_map>
#include <algorithm>
#include <sstream>
#include <cmath>

using namespace std;
using namespace sf;

// Перечисления и структуры
enum class Cell { EMPTY, X, O };
enum class GameMode { CLASSIC, TIMED, SCORING, RANDOM_EVENTS, INFINITE_COMBO };
enum class RandomEvent { NONE, BONUS_MOVE, SWAP_PLAYERS, CLEAR_AREA, TELEPORT, DOUBLE_POINTS };
enum class GameState { MENU, PLAYING, PAUSED, GAME_OVER };

struct Position {
    int x, y;
    Position(int x = 0, int y = 0) : x(x), y(y) {}
    bool operator==(const Position& other) const { return x == other.x && y == other.y; }
    Vector2f toPixel(float cellSize, Vector2f center) const {
        return Vector2f(center.x + x * cellSize, center.y - y * cellSize);
    }
};

struct PositionHash {
    size_t operator()(const Position& p) const {
        return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
    }
};

// Класс генератора последовательности
template <class T>
class Generator {
    private:
        function<T()> next;
        function<bool()> hasNext;
    public:
        Generator(function<T()> func, function<bool()> flag = [](){ return true; }):
            next(func), hasNext(flag) {}

        T GetNext() {
            return next();
        }
        
        bool HasNext() const {
            return hasNext();
        }
        
        bool TryGetNext(T& result) {
            if (HasNext()) {
                result = GetNext();
                return true;
            }
            return false;
        }
};

// Класс кнопки для интерфейса
class Button {
    private:
        RectangleShape shape;
        Text text;
        Color normalColor;
        Color hoverColor;
        bool isHovered;
    public:
        Button() = default;
        Button(const Vector2f& position, const Vector2f& size, const Font& font, const wstring& label, unsigned int charSize = 20):
            text(font, label, charSize) {
            shape.setSize(size);
            shape.setPosition(position);
            shape.setFillColor(Color(70, 70, 70));
            shape.setOutlineThickness(2);
            shape.setOutlineColor(Color(100, 100, 100));

            text.setFillColor(Color::White);

            FloatRect textBounds = text.getLocalBounds();
            text.setPosition(Vector2f(
                position.x + (size.x - textBounds.size.x) / 2,
                position.y + (size.y - textBounds.size.y) / 2 - 5
            ));
            
            normalColor = Color(70, 70, 70);
            hoverColor = Color(100, 100, 100);
            isHovered = false;
        }
        
        void update(const Vector2f& mousePos) {
            isHovered = shape.getGlobalBounds().contains(mousePos);
            shape.setFillColor(isHovered ? hoverColor : normalColor);
            shape.setOutlineColor(isHovered ? Color(150, 150, 150) : Color(100, 100, 100));
        }
        
        void draw(RenderWindow& window) const {
            window.draw(shape);
            window.draw(text);
        }
        
        bool isClicked(const Vector2f& mousePos, bool mousePressed) {
            return isHovered && mousePressed;
        }
};

// Класс игры
class InfiniteTicTacToe {
    private:
        unordered_map<Position, Cell, PositionHash> board;
        Cell currentPlayer;
        GameMode mode;
        int winningLength;
        
        // Графика
        float cellSize;
        Vector2f center;
        vector<RectangleShape> gridLines;
        vector<RectangleShape> xShapes;
        vector<CircleShape> oShapes;
        vector<RectangleShape> highlightSquares;
        
        // Игровая логика
        chrono::seconds moveTimeLimit;
        chrono::steady_clock::time_point lastMoveTime;
        int playerXScore;
        int playerOScore;
        int comboMultiplier;
        mt19937 rng;
        uniform_int_distribution<int> eventChance;
        shared_ptr<Generator<Position>> infiniteMoveGenerator;
        vector<Position> moveHistory;
        vector<Position> winLine;
        RandomEvent lastEvent;
        bool gameWon;
        Cell winner;
        
        // Функции проверки победы
        vector<Position> getLine(const Position& start, int dx, int dy, int length, Cell player) {
            vector<Position> line;
            for (int i = 0; i < length; i++) {
                Position pos(start.x + dx * i, start.y + dy * i);
                auto it = board.find(pos);
                if (it == board.end() || it->second != player) return {};
                line.push_back(pos);
            }
            return line;
        }
        
        bool checkWin(const Position& lastMove) {
            Cell player = board[lastMove];
            if (player == Cell::EMPTY) return false;
            
            // Направления: горизонталь, вертикаль, диагонали
            vector<pair<int, int>> directions = {{1,0}, {0,1}, {1,1}, {1,-1}};
            
            for (auto &dir : directions) {
                int dx = dir.first;
                int dy = dir.second;
                for (int start = -winningLength + 1; start <= 0; start++) {
                    Position startPos(lastMove.x + dx * start, lastMove.y + dy * start);
                    auto line = getLine(startPos, dx, dy, winningLength, player);
                    if (!line.empty()) {
                        winLine = line;
                        return true;
                    }
                }
            }
            return false;
        }
        
        // Случайные события
        RandomEvent generateRandomEvent() {
            if (mode != GameMode::RANDOM_EVENTS) return RandomEvent::NONE;
            
            int roll = eventChance(rng);
            if (roll < 5) return RandomEvent::BONUS_MOVE;
            if (roll < 8) return RandomEvent::SWAP_PLAYERS;
            if (roll < 12) return RandomEvent::CLEAR_AREA;
            if (roll < 15) return RandomEvent::TELEPORT;
            if (roll < 18) return RandomEvent::DOUBLE_POINTS;
            return RandomEvent::NONE;
        }
        
        void handleRandomEvent(RandomEvent event) {
            lastEvent = event;
            
            switch (event) {
                case RandomEvent::BONUS_MOVE:
                    // Бонусный ход - текущий игрок ходит еще раз
                    return;
                case RandomEvent::SWAP_PLAYERS:
                    currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
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
                        updateGraphics();
                    }
                    break;
                case RandomEvent::TELEPORT:
                    if (!board.empty()) {
                        vector<Position> occupied;
                        for (const auto &entry : board) {
                            Position pos = entry.first;
                            Cell cell = entry.second;
                            if (cell != Cell::EMPTY) occupied.push_back(pos);
                        }
                        if (!occupied.empty()) {
                            uniform_int_distribution<int> dist(0, occupied.size() - 1);
                            Position oldPos = occupied[dist(rng)];
                            Cell teleportedCell = board[oldPos];
                            Position newPos;
                            int attempts = 0;
                            do {
                                uniform_int_distribution<int> coordDist(-5, 5);
                                newPos = Position(coordDist(rng), coordDist(rng));
                                attempts++;
                            } while (board.find(newPos) != board.end() && attempts < 100);
                            if (attempts < 100) {
                                board.erase(oldPos);
                                board[newPos] = teleportedCell;
                                updateGraphics();
                            }
                        }
                    }
                    break;
                case RandomEvent::DOUBLE_POINTS:
                    comboMultiplier = 2;
                    break;
                default:
                    break;
            }
        }
        
        // Генератор бесконечных ходов
        shared_ptr<Generator<Position>> createInfiniteMoveGenerator() {
            return make_shared<Generator<Position>>(
                [this]() -> Position {
                    static int radius = 0;
                    static int side = 0;
                    static int x = 0, y = 0;
                    
                    // Спираль вокруг центра
                    if (side == 0) { x++; if (x > radius) { x = radius; side = 1; } }
                    else if (side == 1) { y++; if (y > radius) { y = radius; side = 2; } }
                    else if (side == 2) { x--; if (x < -radius) { x = -radius; side = 3; } }
                    else if (side == 3) { y--; if (y < -radius) { y = -radius; side = 0; radius++; } }
                    
                    return Position(x, y);
                },
                []() -> bool { return true; }
            );
        }
        
        // Обновление графики
        void updateGraphics() {
            xShapes.clear();
            oShapes.clear();
            gridLines.clear();
            highlightSquares.clear();
            
            // Определяем видимую область (от -10 до 10)
            int visibleRadius = 8;
            
            // Сетка
            for (int x = -visibleRadius; x <= visibleRadius; x++) {
                RectangleShape line(Vector2f(2, visibleRadius * 2 * cellSize));
                line.setFillColor(Color(100, 100, 100, 100));
                line.setPosition(Vector2f(center.x + x * cellSize - 1, center.y - visibleRadius * cellSize));
                gridLines.push_back(line);
            }
            
            for (int y = -visibleRadius; y <= visibleRadius; y++) {
                RectangleShape line(Vector2f(visibleRadius * 2 * cellSize, 2));
                line.setFillColor(Color(100, 100, 100, 100));
                line.setPosition(Vector2f(center.x - visibleRadius * cellSize, center.y + y * cellSize - 1));
                gridLines.push_back(line);
            }
            
            // Крестики и нолики
            for (int x = -visibleRadius; x <= visibleRadius; x++) {
                for (int y = -visibleRadius; y <= visibleRadius; y++) {
                    Position pos(x, y);
                    auto it = board.find(pos);
                    if (it != board.end()) {
                        Vector2f pixelPos = pos.toPixel(cellSize, center);
                        if (it->second == Cell::X) {
                            // Крестик (две линии)
                            float offset = cellSize * 0.3f;
                            
                            RectangleShape line1(Vector2f(cellSize * 0.4f, 4));
                            line1.setFillColor(Color::Red);
                            line1.setPosition(Vector2f(pixelPos.x - offset, pixelPos.y - offset));
                            line1.setRotation(degrees(45));
                            xShapes.push_back(line1);
                            
                            RectangleShape line2(Vector2f(cellSize * 0.4f, 4));
                            line2.setFillColor(Color::Red);
                            line2.setPosition(Vector2f(pixelPos.x + offset, pixelPos.y - offset));
                            line2.setRotation(degrees(-45));
                            xShapes.push_back(line2);
                        } else if (it->second == Cell::O) {
                            // Нолик (круг)
                            CircleShape circle(cellSize * 0.3f);
                            circle.setFillColor(Color::Transparent);
                            circle.setOutlineThickness(3);
                            circle.setOutlineColor(Color::Blue);
                            circle.setPosition(Vector2f(pixelPos.x - cellSize * 0.3f, pixelPos.y - cellSize * 0.3f));
                            oShapes.push_back(circle);
                        }
                    }
                }
            }
            
            // Подсветка победной линии
            if (!winLine.empty()) {
                for (const auto& pos : winLine) {
                    Vector2f pixelPos = pos.toPixel(cellSize, center);
                    
                    RectangleShape highlight(Vector2f(cellSize - 4, cellSize - 4));
                    highlight.setFillColor(Color(255, 255, 0, 100));
                    highlight.setPosition(Vector2f(pixelPos.x - cellSize/2 + 2, pixelPos.y - cellSize/2 + 2));
                    highlightSquares.push_back(highlight);
                }
            }
        }
    public:
        InfiniteTicTacToe(GameMode mode = GameMode::CLASSIC, int winningLength = 5, Vector2f windowCenter = Vector2f(400, 300)):
            currentPlayer(Cell::X), mode(mode), winningLength(winningLength), cellSize(40.0f), center(windowCenter),
            moveTimeLimit(chrono::seconds(30)), playerXScore(0), playerOScore(0), comboMultiplier(1), gameWon(false), winner(Cell::EMPTY),
            rng(chrono::steady_clock::now().time_since_epoch().count()), eventChance(0, 99), lastEvent(RandomEvent::NONE) {
            lastMoveTime = chrono::steady_clock::now();
            if (mode == GameMode::INFINITE_COMBO) infiniteMoveGenerator = createInfiniteMoveGenerator();
            updateGraphics();
        }
        
        // Обработка клика мыши
        bool handleClick(const Vector2f& mousePos) {
            if (gameWon) return false;
            
            // Преобразование координат мыши в координаты сетки
            int gridX = round((mousePos.x - center.x) / cellSize);
            int gridY = round((center.y - mousePos.y) / cellSize);
            
            Position pos(gridX, gridY);
            
            // Проверка нахождения в пределах видимой области
            if (abs(gridX) > 8 || abs(gridY) > 8) return false;
            
            // Проверка, свободна ли ячейка
            if (board.find(pos) != board.end() && board[pos] != Cell::EMPTY) {
                return false;
            }
            
            // Проверка времени для режима с таймером
            if (mode == GameMode::TIMED) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastMoveTime);
                if (elapsed > moveTimeLimit) {
                    currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
                    lastMoveTime = now;
                    return false;
                }
            }
            
            // Делаем ход
            board[pos] = currentPlayer;
            moveHistory.push_back(pos);
            lastMoveTime = chrono::steady_clock::now();
            
            // Подсчет очков
            if (mode == GameMode::SCORING) {
                int points = 1 * comboMultiplier;
                if (currentPlayer == Cell::X) playerXScore += points;
                else playerOScore += points;
                if (comboMultiplier > 1) comboMultiplier = 1;
            }
            
            // Проверка победы
            if (checkWin(pos)) {
                gameWon = true;
                winner = currentPlayer;
                updateGraphics();
                return true;
            }
            
            // Случайные события
            RandomEvent event = generateRandomEvent();
            if (event != RandomEvent::NONE) {
                handleRandomEvent(event);
                if (event == RandomEvent::BONUS_MOVE) {
                    // Не меняем игрока при бонусном ходе
                    updateGraphics();
                    return false;
                }
            }
            
            // Смена игрока
            currentPlayer = (currentPlayer == Cell::X) ? Cell::O : Cell::X;
            updateGraphics();
            
            return false;
        }
        
        // Автоматический ход
        bool makeAutoMove() {
            if (mode != GameMode::INFINITE_COMBO || !infiniteMoveGenerator || gameWon) return false;
            
            Position pos;
            if (infiniteMoveGenerator->TryGetNext(pos)) {
                // Проверяем, свободна ли позиция
                if (board.find(pos) == board.end() || board[pos] == Cell::EMPTY) {
                    return handleClick(pos.toPixel(cellSize, center));
                }
            }
            return false;
        }
        
        // Получить подсказку
        Position getHint() {
            if (infiniteMoveGenerator) {
                Position pos;
                if (infiniteMoveGenerator->TryGetNext(pos)) return pos;
            }
            
            // Ищем ближайшую свободную ячейку
            for (int radius = 0; radius < 5; radius++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    for (int dy = -radius; dy <= radius; dy++) {
                        Position pos(dx, dy);
                        if (board.find(pos) == board.end() || board[pos] == Cell::EMPTY) {
                            return pos;
                        }
                    }
                }
            }
            
            return Position(0, 0);
        }
        
        // Отрисовка
        void draw(RenderWindow& window) const {
            // Ось координат
            RectangleShape xAxis(Vector2f(window.getSize().x, 2));
            xAxis.setFillColor(Color(150, 150, 150, 150));
            xAxis.setPosition(Vector2f(0, center.y));
            window.draw(xAxis);
            
            RectangleShape yAxis(Vector2f(2, window.getSize().y));
            yAxis.setFillColor(Color(150, 150, 150, 150));
            yAxis.setPosition(Vector2f(center.x, 0));
            window.draw(yAxis);
            
            // Подписи осей
            Font font;
            if (font.openFromFile("arial.ttf")) {
                Text xLabel(font, "X", 16);
                xLabel.setFillColor(Color::White);
                xLabel.setPosition(Vector2f(window.getSize().x - 30.f, center.y - 20.f));
                window.draw(xLabel);
                
                Text yLabel(font, "Y", 16);
                yLabel.setFillColor(Color::White);
                yLabel.setPosition(Vector2f(center.x + 10.f, 10.f));
                window.draw(yLabel);
            }
            
            // Сетка
            for (const auto& line : gridLines) {
                window.draw(line);
            }
            
            // Подсветка
            for (const auto& highlight : highlightSquares) {
                window.draw(highlight);
            }
            
            // Фигуры
            for (const auto& xShape : xShapes) {
                window.draw(xShape);
            }
            
            for (const auto& oShape : oShapes) {
                window.draw(oShape);
            }
            
            // Центр координат
            CircleShape centerPoint(5);
            centerPoint.setFillColor(Color::Green);
            centerPoint.setPosition(Vector2f(center.x - 5, center.y - 5));
            window.draw(centerPoint);
        }
        
        // Отрисовка интерфейса
        void drawUI(RenderWindow& window, const Font& font) const {
            // Панель информации
            RectangleShape infoPanel(Vector2f(200, 150));
            infoPanel.setFillColor(Color(40, 40, 40, 200));
            infoPanel.setPosition(Vector2f(10, 10));
            window.draw(infoPanel);
            
            // Текущий игрок
            Text currentPlayerText(font, L"Текущий: " + wstring(currentPlayer == Cell::X ? L"X" : L"O"), 20);
            currentPlayerText.setFillColor(currentPlayer == Cell::X ? Color::Red : Color::Blue);
            currentPlayerText.setPosition(Vector2f(20, 20));
            window.draw(currentPlayerText);
            
            // Режим игры
            wstring modeStr;
            switch (mode) {
                case GameMode::CLASSIC: modeStr = L"Классический"; break;
                case GameMode::TIMED: modeStr = L"С таймером"; break;
                case GameMode::SCORING: modeStr = L"Система очков"; break;
                case GameMode::RANDOM_EVENTS: modeStr = L"События"; break;
                case GameMode::INFINITE_COMBO: modeStr = L"Бесконечный"; break;
            }
            
            Text modeText(font, L"Режим: " + modeStr, 16);
            modeText.setFillColor(Color::White);
            modeText.setPosition(Vector2f(20, 50));
            window.draw(modeText);
            
            // Счет
            if (mode == GameMode::SCORING || playerXScore > 0 || playerOScore > 0) {
                Text scoreText(font, L"Счет: X=" + to_string(playerXScore) + L" O=" + to_string(playerOScore), 16);
                scoreText.setFillColor(Color::White);
                scoreText.setPosition(Vector2f(20, 75));
                window.draw(scoreText);
            }
            
            // Таймер
            if (mode == GameMode::TIMED) {
                auto now = chrono::steady_clock::now();
                auto elapsed = chrono::duration_cast<chrono::seconds>(now - lastMoveTime);
                auto remaining = moveTimeLimit > elapsed ? moveTimeLimit - elapsed : chrono::seconds(0);
                
                Text timerText(font, L"Время: " + to_string(remaining.count()) + L"с", 16);
                timerText.setFillColor(remaining.count() < 10 ? Color::Red : Color::Yellow);
                timerText.setPosition(Vector2f(20, 100));
                window.draw(timerText);
            }
            
            // Последнее событие
            if (lastEvent != RandomEvent::NONE) {
                wstring eventStr;
                switch (lastEvent) {
                    case RandomEvent::BONUS_MOVE: eventStr = L"Бонусный ход!"; break;
                    case RandomEvent::SWAP_PLAYERS: eventStr = L"Смена игроков!"; break;
                    case RandomEvent::CLEAR_AREA: eventStr = L"Очистка области!"; break;
                    case RandomEvent::TELEPORT: eventStr = L"Телепорт!"; break;
                    case RandomEvent::DOUBLE_POINTS: eventStr = L"Удвоение очков!"; break;
                    default: break;
                }
                
                Text eventText(font, L"Событие: " + eventStr, 14);
                eventText.setFillColor(Color::Cyan);
                eventText.setPosition(Vector2f(20, 125));
                window.draw(eventText);
            }
            
            // Сообщение о победе
            if (gameWon) {
                RectangleShape winPanel(Vector2f(300, 100));
                winPanel.setFillColor(Color(0, 0, 0, 200));
                winPanel.setPosition(Vector2f(window.getSize().x/2 - 150, window.getSize().y/2 - 50));
                window.draw(winPanel);
                
                Text winText(font, L"Игрок " + wstring(winner == Cell::X ? L"X" : L"O") + L" победил!", 30);
                winText.setFillColor(winner == Cell::X ? Color::Red : Color::Blue);
                winText.setPosition(Vector2f(window.getSize().x/2 - 100, window.getSize().y/2 - 30));
                window.draw(winText);
            }
        }
        
        // Геттеры
        bool isGameWon() const { return gameWon; }
        Cell getCurrentPlayer() const { return currentPlayer; }
        pair<int, int> getScore() const { return {playerXScore, playerOScore}; }
        
        // Сброс игры
        void reset() {
            board.clear();
            moveHistory.clear();
            winLine.clear();
            currentPlayer = Cell::X;
            playerXScore = playerOScore = 0;
            comboMultiplier = 1;
            gameWon = false;
            winner = Cell::EMPTY;
            lastEvent = RandomEvent::NONE;
            lastMoveTime = chrono::steady_clock::now();
            updateGraphics();
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
        
        // Выбранный режим
        GameMode selectedMode;
        int selectedLength;
        
        // Камера
        View gameView;
        Vector2f viewCenter;
        float zoomLevel;
        
        void setupMenu() {
            menuButtons.clear();
            
            float buttonWidth = 200;
            float buttonHeight = 40;
            float startY = 150;
            float spacing = 50;
            
            menuButtons.emplace_back(Vector2f(300, startY), Vector2f(buttonWidth, buttonHeight), font, L"Классический режим");
            menuButtons.emplace_back(Vector2f(300, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"С таймером");
            menuButtons.emplace_back(Vector2f(300, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"Система очков");
            menuButtons.emplace_back(Vector2f(300, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Случайные события");
            menuButtons.emplace_back(Vector2f(300, startY + spacing * 4), Vector2f(buttonWidth, buttonHeight), font, L"Бесконечный режим");
            menuButtons.emplace_back(Vector2f(300, startY + spacing * 5), Vector2f(buttonWidth, buttonHeight), font, L"Выход");
        }
        
        void setupPauseMenu() {
            pauseButtons.clear();
            
            float buttonWidth = 200;
            float buttonHeight = 40;
            float startY = 250;
            
            pauseButtons.emplace_back(Vector2f(300, startY), Vector2f(buttonWidth, buttonHeight), font, L"Продолжить");
            pauseButtons.emplace_back(Vector2f(300, startY + 50), Vector2f(buttonWidth, buttonHeight), font, L"Новая игра");
            pauseButtons.emplace_back(Vector2f(300, startY + 100), Vector2f(buttonWidth, buttonHeight), font, L"В главное меню");
        }
        
        void startGame(GameMode mode, int length) {
            selectedMode = mode;
            selectedLength = length;
            game = make_unique<InfiniteTicTacToe>(mode, length, viewCenter);
            currentState = GameState::PLAYING;
            
            // Сброс камеры
            gameView.setCenter(viewCenter);
            gameView.setSize(Vector2f(window.getSize()));
            zoomLevel = 1.0f;
        }
    public:
        Game(): window(VideoMode({800, 600}), L"Крестики-Нолики"), currentState(GameState::MENU), viewCenter(400, 300), zoomLevel(1.0f) {
            window.setFramerateLimit(60);
            
            bool fontLoaded = font.openFromFile("arial.ttf");
            if (!fontLoaded) fontLoaded = font.openFromFile("C:/Windows/Fonts/arial.ttf");
            
            setupMenu();
            setupPauseMenu();
            
            gameView = View(viewCenter, Vector2f(window.getSize()));
        }
        
        void run() {
            while (window.isOpen()) {
                // Обработка событий через новый API SFML 3
                if (auto event = window.pollEvent()) {
                    if (event->is<Event::Closed>()) {
                        window.close();
                    } else {
                        // Обработка ввода в зависимости от состояния
                        switch (currentState) {
                            case GameState::MENU:
                                handleMenuInput(*event);
                                break;
                            case GameState::PLAYING:
                                handleGameInput(*event);
                                break;
                            case GameState::PAUSED:
                                handlePauseInput(*event);
                                break;
                            case GameState::GAME_OVER:
                                handleGameOverInput(*event);
                                break;
                        }
                    }
                }

                update();
                window.clear(Color(30, 30, 30));
                draw();
                window.display();
            }
        }
    private:
        void handleMenuInput(const Event& event) {
            Vector2f mousePos = Vector2f(Mouse::getPosition(window));
            bool mousePressed = false;
            
            if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
                if (mouseEvent->button == Mouse::Button::Left) mousePressed = true;
            }
            
            // Обновление кнопок
            for (auto& button : menuButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &menuButtons[0];
                    if (index < 5) { // Режимы игры
                        startGame(static_cast<GameMode>(index), 5);
                    } else if (index == 5) { // Выход
                        window.close();
                    }
                }
            }
            
            // Обработка клавиш
            if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
                if (keyEvent->scancode == Keyboard::Scan::Escape) {
                    window.close();
                }
            }
        }
        
        void handleGameInput(const Event& event) {
            Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), gameView);
            bool mousePressed = false;
            
            if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
                if (mouseEvent->button == Mouse::Button::Left) {
                    // Ход игрока
                    if (!game->isGameWon()) {
                        game->handleClick(mousePos);
                    }
                }
            }
            
            // Управление камерой
            if (const auto* wheelEvent = event.getIf<Event::MouseWheelScrolled>()) {
                if (wheelEvent->delta > 0) zoomLevel *= 0.9f;
                else zoomLevel *= 1.1f;
                gameView.setSize(Vector2f(window.getSize()) * (1.0f / zoomLevel));
            }
            
            // Перемещение камеры
            if (Mouse::isButtonPressed(Mouse::Button::Right)) {
                static Vector2f lastMousePos;
                if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
                    if (mouseEvent->button == Mouse::Button::Right) {
                        lastMousePos = mousePos;
                    }
                } else if (event.is<Event::MouseMoved>() && Mouse::isButtonPressed(Mouse::Button::Right)) {
                    Vector2f delta = lastMousePos - mousePos;
                    gameView.move(delta);
                    lastMousePos = mousePos;
                }
            }
            
            // Клавиши управления
            if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
                switch (keyEvent->scancode) {
                    case Keyboard::Scan::Escape:
                        currentState = GameState::PAUSED;
                        break;
                    case Keyboard::Scan::Space:
                        if (!game->isGameWon()) {
                            game->makeAutoMove();
                        }
                        break;
                    case Keyboard::Scan::H:
                        if (!game->isGameWon()) {
                            Position hint = game->getHint();
                            // Можно добавить визуальную подсказку
                        }
                        break;
                    case Keyboard::Scan::R:
                        game->reset();
                        break;
                    case Keyboard::Scan::NumpadPlus:
                        zoomLevel *= 0.9f;
                        gameView.zoom(0.9f);
                        break;
                    case Keyboard::Scan::NumpadMinus:
                        zoomLevel *= 1.1f;
                        gameView.zoom(1.1f);
                        break;
                }
            }
        }
        
        void handlePauseInput(const Event& event) {
            Vector2f mousePos = Vector2f(Mouse::getPosition(window));
            bool mousePressed = false;
            
            if (const auto* mouseEvent = event.getIf<Event::MouseButtonPressed>()) {
                if (mouseEvent->button == Mouse::Button::Left) mousePressed = true;
            }
            
            for (auto& button : pauseButtons) {
                button.update(mousePos);
                if (button.isClicked(mousePos, mousePressed)) {
                    size_t index = &button - &pauseButtons[0];
                    if (index == 0) { // Продолжить
                        currentState = GameState::PLAYING;
                    } else if (index == 1) { // Новая игра
                        startGame(selectedMode, selectedLength);
                    } else if (index == 2) { // В главное меню
                        currentState = GameState::MENU;
                        game.reset();
                    }
                }
            }
            
            if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
                if (keyEvent->scancode == Keyboard::Scan::Escape) {
                    currentState = GameState::PLAYING;
                }
            }
        }
        
        void handleGameOverInput(const Event& event) {
            if (event.is<Event::KeyPressed>()) {
                if (const auto* keyEvent = event.getIf<Event::KeyPressed>()) {
                    if (keyEvent->scancode == Keyboard::Scan::Enter) {
                        game->reset();
                        currentState = GameState::PLAYING;
                    } else if (keyEvent->scancode == Keyboard::Scan::Escape) {
                        currentState = GameState::MENU;
                        game.reset();
                    }
                }
            }
            
            if (event.is<Event::MouseButtonPressed>()) {
                game->reset();
                currentState = GameState::PLAYING;
            }
        }
        
        void update() {
            // Автоматические ходы для бесконечного режима
            if (currentState == GameState::PLAYING && game && 
                selectedMode == GameMode::INFINITE_COMBO && !game->isGameWon()) {
                static Clock autoMoveClock;
                if (autoMoveClock.getElapsedTime().asSeconds() > 1.0f) {
                    game->makeAutoMove();
                    autoMoveClock.restart();
                }
            }
        }
        
        void draw() {
            switch (currentState) {
                case GameState::MENU:
                    drawMenu();
                    break;
                case GameState::PLAYING:
                    drawGame();
                    break;
                case GameState::PAUSED:
                    drawGame();
                    drawPauseMenu();
                    break;
                case GameState::GAME_OVER:
                    drawGame();
                    break;
            }
        }
        
        void drawMenu() {
            // Фон
            RectangleShape background(Vector2f(window.getSize()));
            background.setFillColor(Color(20, 20, 40));
            window.draw(background);
            
            // Заголовок
            Text title(font, L"КРЕСТИКИ-НОЛИКИ", 36);
            title.setFillColor(Color::Cyan);
            title.setPosition(Vector2f(200, 50));
            window.draw(title);
            
            Text subtitle(font, L"Выберите режим игры", 24);
            subtitle.setFillColor(Color::White);
            subtitle.setPosition(Vector2f(300, 100));
            window.draw(subtitle);
            
            // Кнопки
            for (auto& button : menuButtons) {
                button.draw(window);
            }
            
            // Подсказки
            Text hints(font, L"Управление в игре:\n"
                    L"ЛКМ - сделать ход\n"
                    L"ПКМ + перемещение - двигать камеру\n"
                    L"Колесо мыши - зум\n"
                    L"Пробел - автоход (в бесконечном режиме)\n"
                    L"H - подсказка\n"
                    L"R - перезапуск\n"
                    L"ESC - пауза/меню", 14);
            hints.setFillColor(Color(150, 150, 150));
            hints.setPosition(Vector2f(550, 150));
            window.draw(hints);
        }
        
        void drawGame() {
            window.setView(gameView);
            if (game) game->draw(window);
            
            window.setView(window.getDefaultView());
            if (game) game->drawUI(window, font);
            
            // Инструкции
            Text instructions(font, L"ESC - Пауза | R - Перезапуск | ПКМ + мышь - Двигать камеру", 14);
            instructions.setFillColor(Color(150, 150, 150));
            instructions.setPosition(Vector2f(10, window.getSize().y - 30));
            window.draw(instructions);
        }
        
        void drawPauseMenu() {
            // Полупрозрачный фон
            RectangleShape overlay(Vector2f(window.getSize()));
            overlay.setFillColor(Color(0, 0, 0, 150));
            window.draw(overlay);
            
            Text pauseText(font, L"ПАУЗА", 48);
            pauseText.setFillColor(Color::Yellow);
            pauseText.setPosition(Vector2f(350, 150));
            window.draw(pauseText);
            
            for (auto& button : pauseButtons) {
                button.draw(window);
            }
        }
};

int main() {
    try {
        Game game;
        game.run();
    } catch (const exception& e) {
        cerr << L"Ошибка: " << e.what() << endl;
        return 1;
    }
    
    return 0;
}
