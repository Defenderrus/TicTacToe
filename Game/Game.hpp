#pragma once

#include "GameStates.hpp"
#include "GameUI.hpp"
#include "GameBoard/InfiniteTicTacToe.hpp"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>

using namespace std;
using namespace sf;


class Game {
    private:
        RenderWindow window;
        GameState currentState;
        GameState opponentSelectionState;
        GameState difficultySelectionState;
        unique_ptr<InfiniteTicTacToe> game;
        Font font;
        
        // UI элементы
        vector<Button> menuButtons;
        vector<Button> pauseButtons;
        vector<Button> scoreButtons;
        vector<Button> timeButtons;
        vector<Button> opponentButtons;
        vector<Button> difficultyButtons;
        
        // Настройки игры
        GameMode selectedMode;
        int selectedLength;
        int selectedScoreTarget;
        chrono::seconds selectedTimeLimit;
        OpponentType selectedOpponent;
        BotDifficulty selectedDifficulty;
        
        // Камера
        View gameView;
        View uiView;
        Vector2f viewCenter;
        float zoomLevel;
        
        // Оптимизация
        Clock inputClock;
        float inputCooldown;
        Clock frameClock;
        float targetFPS;
        float frameTime;
        Vector2u windowSize;

    public:
        Game();
        void run();

    private:
        void processEvents();
        void update();
        void render();
        
        // Методы настройки интерфейса
        void setupMenu();
        void setupPauseMenu();
        void setupScoreSelection();
        void setupTimeSelection();
        void setupOpponentSelection();
        void setupDifficultySelection();
        void updateButtonsForWindowSize();
        
        // Запуск игры
        void startGame(GameMode mode, int length, int scoreTarget = 300, chrono::seconds timeLimit = chrono::seconds(10),
                       OpponentType opponent = OpponentType::PLAYER_VS_PLAYER, BotDifficulty difficulty = BotDifficulty::MEDIUM);
        
        bool checkInputCooldown();
        
        // Обработчики ввода для разных состояний
        void handleMenuInput(const Event &event);
        void handleGameInput(const Event &event);
        void handlePauseInput(const Event &event);
        void handleGameOverInput(const Event &event);
        void handleScoreSelectionInput(const Event &event);
        void handleTimeSelectionInput(const Event &event);
        void handleOpponentSelectionInput(const Event &event);
        void handleDifficultySelectionInput(const Event &event);
        
        // Методы отрисовки
        void drawMenu();
        void drawGame();
        void drawPauseMenu();
        void drawScoreSelection();
        void drawTimeSelection();
        void drawOpponentSelection();
        void drawDifficultySelection();
};

Game::Game(): window(VideoMode({800, 600}), L"Крестики-Нолики", Style::Default), currentState(GameState::MENU),
              viewCenter(400, 300), zoomLevel(1.0f), selectedLength(5), selectedScoreTarget(300),
              selectedTimeLimit(chrono::seconds(10)), selectedOpponent(OpponentType::PLAYER_VS_PLAYER),
              selectedDifficulty(BotDifficulty::MEDIUM), windowSize(800, 600),
              inputCooldown(0.1f), targetFPS(60.0f), frameTime(1.0f / targetFPS) {
    
    window.setFramerateLimit(static_cast<unsigned int>(targetFPS));
    
    if (!font.openFromFile("C:/Windows/Fonts/arial.ttf")) {
        if (!font.openFromFile("C:/Windows/Fonts/tahoma.ttf")) {
            cerr << L"Не удалось загрузить шрифт!" << endl;
        }
    }
    
    uiView = View(Vector2f(windowSize.x / 2.0f, windowSize.y / 2.0f), Vector2f(windowSize));
    gameView = View(viewCenter, Vector2f(windowSize));
    
    setupMenu();
    setupPauseMenu();
    setupScoreSelection();
    setupTimeSelection();
    setupOpponentSelection();
    setupDifficultySelection();
}

void Game::run() {
    while (window.isOpen()) {
        processEvents();
        update();
        render();
    }
}

void Game::processEvents() {
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
                case GameState::OPPONENT_SELECTION: handleOpponentSelectionInput(*event); break;
                case GameState::DIFFICULTY_SELECTION: handleDifficultySelectionInput(*event); break;
            }
        }
    }
}

void Game::update() {
    float elapsed = frameClock.restart().asSeconds();
    if (elapsed < frameTime) sleep(seconds(frameTime - elapsed));

    if (currentState == GameState::PLAYING && game && game->isBotCurrentTurn() && !game->isGameWon()) {
        game->makeBotMove();
        if (game->isGameWon()) currentState = GameState::GAME_OVER;
    }
}

void Game::render() {
    window.clear(Color(30, 30, 30));
    
    switch (currentState) {
        case GameState::MENU: drawMenu(); break;
        case GameState::PLAYING: drawGame(); break;
        case GameState::PAUSED: drawGame(); drawPauseMenu(); break;
        case GameState::GAME_OVER: drawGame(); break;
        case GameState::SCORE_SELECTION: drawScoreSelection(); break;
        case GameState::TIME_SELECTION: drawTimeSelection(); break;
        case GameState::OPPONENT_SELECTION: drawOpponentSelection(); break;
        case GameState::DIFFICULTY_SELECTION: drawDifficultySelection(); break;
    }
    
    window.display();
}

void Game::setupMenu() {
    menuButtons.clear();
    
    const float buttonWidth = 320;
    const float buttonHeight = 50;
    const float startY = 180;
    const float spacing = 60;
    
    menuButtons.reserve(5);
    menuButtons.emplace_back(Vector2f(240, startY), Vector2f(buttonWidth, buttonHeight), font, L"Классический режим", 22);
    menuButtons.emplace_back(Vector2f(240, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"Режим с таймером", 22);
    menuButtons.emplace_back(Vector2f(240, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"Режим на очки", 22);
    menuButtons.emplace_back(Vector2f(240, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Режим с событиями", 22);
    menuButtons.emplace_back(Vector2f(240, startY + spacing * 4), Vector2f(buttonWidth, buttonHeight), font, L"Выход", 22);
}

void Game::setupPauseMenu() {
    pauseButtons.clear();
    pauseButtons.reserve(3);
    
    const float buttonWidth = 240;
    const float buttonHeight = 50;
    const float startY = 250;
    const float spacing = 60;
    
    pauseButtons.emplace_back(Vector2f(280, startY), Vector2f(buttonWidth, buttonHeight), font, L"Продолжить", 22);
    pauseButtons.emplace_back(Vector2f(280, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"Новая игра", 22);
    pauseButtons.emplace_back(Vector2f(280, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"В главное меню", 22);
}

void Game::setupScoreSelection() {
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

void Game::setupTimeSelection() {
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

void Game::setupOpponentSelection() {
    opponentButtons.clear();
    opponentButtons.reserve(3);
    
    const float buttonWidth = 300;
    const float buttonHeight = 50;
    const float startY = 200;
    const float spacing = 60;
    
    opponentButtons.emplace_back(Vector2f(250, startY), Vector2f(buttonWidth, buttonHeight), font, L"Игрок vs Игрок", 22);
    opponentButtons.emplace_back(Vector2f(250, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"Игрок vs Бот", 22);
    opponentButtons.emplace_back(Vector2f(250, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"Назад", 22);
}

void Game::setupDifficultySelection() {
    difficultyButtons.clear();
    difficultyButtons.reserve(4);
    
    const float buttonWidth = 240;
    const float buttonHeight = 50;
    const float startY = 200;
    const float spacing = 60;
    
    difficultyButtons.emplace_back(Vector2f(280, startY), Vector2f(buttonWidth, buttonHeight), font, L"Легкий", 22);
    difficultyButtons.emplace_back(Vector2f(280, startY + spacing), Vector2f(buttonWidth, buttonHeight), font, L"Средний", 22);
    difficultyButtons.emplace_back(Vector2f(280, startY + spacing * 2), Vector2f(buttonWidth, buttonHeight), font, L"Сложный", 22);
    difficultyButtons.emplace_back(Vector2f(280, startY + spacing * 3), Vector2f(buttonWidth, buttonHeight), font, L"Назад", 22);
}

void Game::updateButtonsForWindowSize() {
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
        
        menuButtons[0].setPosition(Vector2f(centerX - 160, 180));
        menuButtons[1].setPosition(Vector2f(centerX - 160, 240));
        menuButtons[2].setPosition(Vector2f(centerX - 160, 300));
        menuButtons[3].setPosition(Vector2f(centerX - 160, 360));
        menuButtons[4].setPosition(Vector2f(centerX - 160, 420));
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

    if (!opponentButtons.empty()) {
        float centerX = windowSize.x / 2.0f;
        
        opponentButtons[0].setPosition(Vector2f(centerX - 150, 200));
        opponentButtons[1].setPosition(Vector2f(centerX - 150, 260));
        opponentButtons[2].setPosition(Vector2f(centerX - 150, 320));
    }
    
    if (!difficultyButtons.empty()) {
        float centerX = windowSize.x / 2.0f;
        
        difficultyButtons[0].setPosition(Vector2f(centerX - 120, 200));
        difficultyButtons[1].setPosition(Vector2f(centerX - 120, 260));
        difficultyButtons[2].setPosition(Vector2f(centerX - 120, 320));
        difficultyButtons[3].setPosition(Vector2f(centerX - 120, 380));
    }
    
    if (game) {
        game->setCenter(viewCenter);
    }
}

void Game::startGame(GameMode mode, int length, int scoreTarget, chrono::seconds timeLimit,
                     OpponentType opponent, BotDifficulty difficulty) {
    selectedMode = mode;
    selectedLength = length;
    selectedScoreTarget = scoreTarget;
    selectedTimeLimit = timeLimit;
    selectedOpponent = opponent;
    selectedDifficulty = difficulty;
    
    game = make_unique<InfiniteTicTacToe>(mode, length, scoreTarget, timeLimit, viewCenter, opponent, difficulty);
    currentState = GameState::PLAYING;
    
    gameView.setCenter(viewCenter);
    gameView.setSize(Vector2f(window.getSize()));
    zoomLevel = 1.0f;
}

bool Game::checkInputCooldown() {
    if (inputClock.getElapsedTime().asSeconds() >= inputCooldown) {
        inputClock.restart();
        return true;
    }
    return false;
}

// Обработчики ввода (сокращенные реализации)
void Game::handleMenuInput(const Event& event) {
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
                if (selectedMode == GameMode::CLASSIC) {
                    currentState = GameState::OPPONENT_SELECTION;
                } else if (selectedMode == GameMode::SCORING || selectedMode == GameMode::RANDOM_EVENTS) {
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

void Game::handleGameInput(const Event& event) {
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

void Game::handlePauseInput(const Event& event) {
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

void Game::handleGameOverInput(const Event& event) {
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

void Game::handleScoreSelectionInput(const Event& event) {
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

void Game::handleTimeSelectionInput(const Event& event) {
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

void Game::handleOpponentSelectionInput(const Event& event) {
    Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
    bool mousePressed = false;
    
    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) mousePressed = true;
    }
    
    for (auto &button : opponentButtons) {
        button.update(mousePos);
        if (button.isClicked(mousePos, mousePressed)) {
            size_t index = &button - &opponentButtons[0];
            if (index == 0) {
                selectedOpponent = OpponentType::PLAYER_VS_PLAYER;
                startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit, selectedOpponent, selectedDifficulty);
            } else if (index == 1) {
                currentState = GameState::DIFFICULTY_SELECTION;
            } else if (index == 2) {
                currentState = GameState::MENU;
            }
        }
    }
    
    if (auto keyPress = event.getIf<Event::KeyPressed>()) {
        if (keyPress->code == Keyboard::Key::Escape) currentState = GameState::MENU;
    }
}

void Game::handleDifficultySelectionInput(const Event& event) {
    Vector2f mousePos = window.mapPixelToCoords(Mouse::getPosition(window), uiView);
    bool mousePressed = false;
    
    if (auto mousePress = event.getIf<Event::MouseButtonPressed>()) {
        if (mousePress->button == Mouse::Button::Left) mousePressed = true;
    }
    
    for (auto &button : difficultyButtons) {
        button.update(mousePos);
        if (button.isClicked(mousePos, mousePressed)) {
            size_t index = &button - &difficultyButtons[0];
            if (index < 3) {
                selectedDifficulty = static_cast<BotDifficulty>(index);
                startGame(selectedMode, selectedLength, selectedScoreTarget, selectedTimeLimit, OpponentType::PLAYER_VS_BOT, selectedDifficulty);
            } else if (index == 3) {
                currentState = GameState::OPPONENT_SELECTION;
            }
        }
    }
    
    if (auto keyPress = event.getIf<Event::KeyPressed>()) {
        if (keyPress->code == Keyboard::Key::Escape) currentState = GameState::OPPONENT_SELECTION;
    }
}

void Game::drawMenu() {
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
    
    Text hints(font, L"Управление в игре:\n" \
                     L"ЛКМ - сделать ход\n" \
                     L"ПКМ - двигать камеру\n" \
                     L"ESC - пауза/меню\n" \
                     L"R - перезапуск\n" \
                     L"+/- - масштабирование\n", 16);
    hints.setFillColor(Color(150, 150, 150));
    hints.setPosition(Vector2f(windowSize.x - 180, 180));
    window.draw(hints);
}

void Game::drawGame() {
    window.setView(gameView);
    if (game) game->draw(window);
    
    window.setView(uiView);
    if (game) game->drawUI(window, font);
    
    Text instructions(font, L"Управление в игре:\n" \
                            L"ЛКМ - сделать ход\n" \
                            L"ПКМ - двигать камеру\n" \
                            L"ESC - пауза/меню\n" \
                            L"R - перезапуск\n" \
                            L"+/- - масштабирование\n", 16);
    instructions.setFillColor(Color(150, 150, 150));
    instructions.setPosition(Vector2f(windowSize.x - 180, 180));
    window.draw(instructions);
}

void Game::drawPauseMenu() {
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

void Game::drawScoreSelection() {
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
        case GameMode::RANDOM_EVENTS: modeStr = L"Режим с событиями"; break;
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

void Game::drawTimeSelection() {
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
    
    Text hint(font, L"Если время истекает, ход переходит другому игроку", 16);
    hint.setFillColor(Color::Yellow);
    FloatRect hintBounds = hint.getLocalBounds();
    hint.setPosition(Vector2f(windowSize.x / 2.0f - hintBounds.size.x / 2, 450));
    window.draw(hint);
}

void Game::drawOpponentSelection() {
    window.setView(uiView);
    
    RectangleShape background;
    background.setSize(Vector2f(windowSize));
    background.setFillColor(Color(20, 20, 40));
    background.setPosition(Vector2f(0, 0));
    window.draw(background);
    
    Text title(font, L"ВЫБЕРИТЕ ПРОТИВНИКА", 36);
    title.setFillColor(Color::Cyan);
    title.setStyle(Text::Bold);
    FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(Vector2f(windowSize.x / 2.0f - titleBounds.size.x / 2, 80));
    window.draw(title);
    
    Text subtitle(font, L"Классический режим", 28);
    subtitle.setFillColor(Color::White);
    FloatRect subtitleBounds = subtitle.getLocalBounds();
    subtitle.setPosition(Vector2f(windowSize.x / 2.0f - subtitleBounds.size.x / 2, 130));
    window.draw(subtitle);
    
    for (auto &button : opponentButtons) {
        button.draw(window);
    }
}

void Game::drawDifficultySelection() {
    window.setView(uiView);
    
    RectangleShape background;
    background.setSize(Vector2f(windowSize));
    background.setFillColor(Color(20, 20, 40));
    background.setPosition(Vector2f(0, 0));
    window.draw(background);
    
    Text title(font, L"ВЫБЕРИТЕ СЛОЖНОСТЬ БОТА", 36);
    title.setFillColor(Color::Cyan);
    title.setStyle(Text::Bold);
    FloatRect titleBounds = title.getLocalBounds();
    title.setPosition(Vector2f(windowSize.x / 2.0f - titleBounds.size.x / 2, 80));
    window.draw(title);
    
    Text subtitle(font, L"Игра против бота", 28);
    subtitle.setFillColor(Color::White);
    FloatRect subtitleBounds = subtitle.getLocalBounds();
    subtitle.setPosition(Vector2f(windowSize.x / 2.0f - subtitleBounds.size.x / 2, 130));
    window.draw(subtitle);
    
    for (auto &button : difficultyButtons) {
        button.draw(window);
    }
    
    Text hints(font, L"Легкий: случайные ходы и простые решения\n" \
                     L"Средний: базовая стратегия\n" \
                     L"Сложный: продвинутая стратегия с просчетом ходов", 16);
    hints.setFillColor(Color::Yellow);
    hints.setLineSpacing(1.2f);
    FloatRect hintsBounds = hints.getLocalBounds();
    hints.setPosition(Vector2f(windowSize.x / 2.0f - hintsBounds.size.x / 2, 450));
    window.draw(hints);
}
