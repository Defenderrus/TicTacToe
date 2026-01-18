#include "TicTacToeBot.hpp"


int TicTacToeBot::evaluateLine(const GameBoard &board, const Position &start, int dx, int dy, Cell player, int lineLength) const {
    int bestScore = 0;
    
    for (int offset = -lineLength + 1; offset <= 0; offset++) {
        int playerCount = 0;
        int emptyCount = 0;
        bool blocked = false;
        
        for (int i = 0; i < lineLength; i++) {
            Position pos(start.x + dx * (offset + i), start.y + dy * (offset + i));
            Cell cell = board.get(pos);
            
            if (cell == player) playerCount++;
            else if (cell == Cell::EMPTY) emptyCount++;
            else {
                blocked = true;
                break;
            }
        }
        
        if (!blocked) {
            if (playerCount == lineLength) return 10000;
            if (playerCount == lineLength - 1 && emptyCount == 1) return 1000;
            if (playerCount == lineLength - 2 && emptyCount == 2) return 100;
            if (playerCount >= 2) bestScore = max(bestScore, playerCount * 10);
        }
    }
    
    return bestScore;
}

int TicTacToeBot::evaluatePosition(const GameBoard &board, int lineLength) const {
    int score = 0;

    score += evaluateAllLines(board, botSymbol, lineLength);
    score -= evaluateAllLines(board, opponentSymbol, lineLength) * 1.2f;
    score += evaluateCenterControl(board);
    
    return score;
}

int TicTacToeBot::evaluateAllLines(const GameBoard &board, Cell player, int lineLength) const {
    int totalScore = 0;
    auto positions = board.getOccupiedPositions(player);
    
    if (positions.empty()) return 0;
    
    constexpr array<pair<int, int>, 4> directions = {{ {1, 0}, {0, 1}, {1, 1}, {1, -1} }};
    
    unordered_map<pair<int, int>, bool, PositionHash> processed;
    
    for (const auto &pos : positions) {
        for (const auto &dir : directions) {
            int dx = dir.first, dy = dir.second;
            
            pair<int, int> lineKey = {pos.x * 1000 + dx, pos.y * 1000 + dy};
            if (processed[lineKey]) continue;
            processed[lineKey] = true;
            
            totalScore += evaluateLine(board, pos, dx, dy, player, lineLength);
        }
    }
    
    return totalScore;
}

int TicTacToeBot::evaluateCenterControl(const GameBoard &board) const {
    int score = 0;
    
    Position center(0, 0);
    if (board.get(center) == botSymbol) score += 50;
    else if (board.get(center) == opponentSymbol) score -= 60;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            Position pos(dx, dy);
            Cell cell = board.get(pos);
            
            if (cell == botSymbol) score += 20;
            else if (cell == opponentSymbol) score -= 25;
        }
    }
    
    return score;
}

optional<Position> TicTacToeBot::checkImmediateWinOrBlock(const GameBoard &board, int lineLength) {
    auto winMove = findWinningMove(board, botSymbol, lineLength);
    if (winMove.has_value()) return winMove;
    
    auto blockMove = findWinningMove(board, opponentSymbol, lineLength);
    if (blockMove.has_value()) return blockMove;
    
    return nullopt;
}

optional<Position> TicTacToeBot::findWinningMove(const GameBoard &board, Cell player, int lineLength) const {
    auto emptyPositions = getPotentialMoves(board);
    
    for (const auto &pos : emptyPositions) {
        GameBoard tempBoard = board;
        tempBoard.set(pos, player);
        if (checkWinForPlayer(tempBoard, player, lineLength)) return pos;
    }
    
    return nullopt;
}

bool TicTacToeBot::checkWinForPlayer(const GameBoard &board, Cell player, int lineLength) const {
    auto positions = board.getOccupiedPositions(player);
    
    for (const auto &pos : positions) {
        constexpr array<pair<int, int>, 4> directions = {{ {1, 0}, {0, 1}, {1, 1}, {1, -1} }};
        
        for (const auto &dir : directions) {
            int dx = dir.first, dy = dir.second;
            
            for (int offset = -lineLength + 1; offset <= 0; offset++) {
                bool win = true;
                for (int i = 0; i < lineLength; i++) {
                    Position checkPos(pos.x + dx * (offset + i), pos.y + dy * (offset + i));
                    if (board.get(checkPos) != player) {
                        win = false;
                        break;
                    }
                }
                if (win) return true;
            }
        }
    }
    
    return false;
}

pair<int, Position> TicTacToeBot::minimax(const GameBoard &board, int depth, int alpha, int beta, bool maximizingPlayer, int lineLength) {
    if (depth == 0) return {evaluatePosition(board, lineLength), Position(0, 0)};
    
    vector<Position> possibleMoves = getPotentialMoves(board);
    if (possibleMoves.empty()) return {0, Position(0, 0)};
    
    sort(possibleMoves.begin(), possibleMoves.end(),
        [&](const Position &a, const Position &b) {
            return evaluateMove(board, a) > evaluateMove(board, b);
        });
    
    int movesToConsider = min(maxMovesToConsider, (int)possibleMoves.size());
    
    if (maximizingPlayer) {
        int maxEval = INT_MIN;
        Position bestMove = possibleMoves[0];
        
        for (int i = 0; i < movesToConsider; i++) {
            const Position &move = possibleMoves[i];
            
            GameBoard newBoard = board;
            newBoard.set(move, botSymbol);
            
            if (checkWinForPlayer(newBoard, botSymbol, lineLength)) {
                return {10000 + depth * 10, move};
            }
            
            auto [eval, _] = minimax(newBoard, depth - 1, alpha, beta, false, lineLength);
            
            if (eval > maxEval) {
                maxEval = eval;
                bestMove = move;
            }
            
            alpha = max(alpha, eval);
            if (beta <= alpha) {
                break;
            }
        }
        
        return {maxEval, bestMove};
    } else {
        int minEval = INT_MAX;
        Position bestMove = possibleMoves[0];
        
        for (int i = 0; i < movesToConsider; i++) {
            const Position &move = possibleMoves[i];
            
            GameBoard newBoard = board;
            newBoard.set(move, opponentSymbol);
            
            if (checkWinForPlayer(newBoard, opponentSymbol, lineLength)) {
                return {-10000 - depth * 10, move};
            }
            
            auto [eval, _] = minimax(newBoard, depth - 1, alpha, beta, true, lineLength);
            
            if (eval < minEval) {
                minEval = eval;
                bestMove = move;
            }
            
            beta = min(beta, eval);
            if (beta <= alpha) {
                break;
            }
        }
        
        return {minEval, bestMove};
    }
}

int TicTacToeBot::evaluateMove(const GameBoard &board, const Position &move) const {
    int score = 0;
    
    int distance = abs(move.x) + abs(move.y);
    score += (5 - distance) * 10;
    
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            if (dx == 0 && dy == 0) continue;
            Position neighbor(move.x + dx, move.y + dy);
            Cell cell = board.get(neighbor);
            
            if (cell == botSymbol) score += 15;
            else if (cell == opponentSymbol) score += 10;
        }
    }
    
    return score;
}

vector<Position> TicTacToeBot::getPotentialMoves(const GameBoard &board) const {
    vector<Position> moves;
    unordered_set<pair<int, int>, PositionHash> moveSet;
    
    int minX, maxX, minY, maxY;
    board.getBounds(minX, maxX, minY, maxY);
    
    int expansion = (difficulty == BotDifficulty::HARD) ? 3 : 2;
    int searchRadius;
    switch (difficulty) {
        case BotDifficulty::EASY: searchRadius = 1; break;
        case BotDifficulty::MEDIUM: searchRadius = 2; break;
        case BotDifficulty::HARD: searchRadius = 2; break;
    }
    
    auto occupied = board.getOccupiedPositions();
    
    for (const auto &pos : occupied) {
        for (int dx = -searchRadius; dx <= searchRadius; dx++) {
            for (int dy = -searchRadius; dy <= searchRadius; dy++) {
                Position candidate(pos.x + dx, pos.y + dy);
                
                if (board.get(candidate) == Cell::EMPTY) {
                    pair<int, int> key = candidate.toPair();
                    if (moveSet.find(key) == moveSet.end()) {
                        moves.push_back(candidate);
                        moveSet.insert(key);
                    }
                }
            }
        }
    }
    
    if (moves.empty()) {
        array<Position, 9> startPositions = {{
            Position(0, 0), Position(-1, -1), Position(-1, 0), Position(-1, 1),
            Position(0, -1), Position(0, 1), Position(1, -1), Position(1, 0), Position(1, 1)
        }};
        
        for (const auto &pos : startPositions) {
            if (board.get(pos) == Cell::EMPTY) moves.push_back(pos);
        }
    }
    
    return moves;
}

TicTacToeBot::TicTacToeBot(BotDifficulty diff, Cell symbol): difficulty(diff), botSymbol(symbol) {
    opponentSymbol = (botSymbol == Cell::X) ? Cell::O : Cell::X;
    
    switch (difficulty) {
        case BotDifficulty::EASY:
            searchDepth = 2;
            maxMovesToConsider = 6;
            break;
        case BotDifficulty::MEDIUM:
            searchDepth = 3;
            maxMovesToConsider = 10;
            break;
        case BotDifficulty::HARD:
            searchDepth = 4;
            maxMovesToConsider = 15;
            break;
    }
}

Position TicTacToeBot::getBestMove(const GameBoard &board, int lineLength) {
    auto immediate = checkImmediateWinOrBlock(board, lineLength);
    if (immediate.has_value()) {
        return immediate.value();
    }
    
    if (difficulty == BotDifficulty::EASY) {
        static mt19937 rng(static_cast<unsigned>(chrono::system_clock::now().time_since_epoch().count()));
        uniform_int_distribution<int> dist(0, 100);
        if (dist(rng) < 40) {
            auto moves = getPotentialMoves(board);
            if (!moves.empty()) {
                uniform_int_distribution<int> moveDist(0, moves.size() - 1);
                return moves[moveDist(rng)];
            }
        }
    }
    
    return minimax(board, searchDepth, INT_MIN, INT_MAX, true, lineLength).second;
}

void TicTacToeBot::setDifficulty(BotDifficulty diff) {
    difficulty = diff;
    switch (difficulty) {
        case BotDifficulty::EASY:
            searchDepth = 2;
            maxMovesToConsider = 6;
            break;
        case BotDifficulty::MEDIUM:
            searchDepth = 3;
            maxMovesToConsider = 10;
            break;
        case BotDifficulty::HARD:
            searchDepth = 4;
            maxMovesToConsider = 15;
            break;
    }
}

void TicTacToeBot::setSymbol(Cell symbol) {
    botSymbol = symbol;
    opponentSymbol = (botSymbol == Cell::X) ? Cell::O : Cell::X;
}

BotDifficulty TicTacToeBot::getDifficulty() const { return difficulty; }
