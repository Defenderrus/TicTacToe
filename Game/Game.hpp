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
        bool playerJustMoved;

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
