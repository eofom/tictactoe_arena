#ifndef TICTACTOE_PLAYER_H
#define TICTACTOE_PLAYER_H

#include <iostream>
#include <array>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <memory>
#include <chrono>

using BoardState = uint32_t;
using CellsMask = BoardState;
using Index = uint32_t;

constexpr Index kNoPlayerNumber = 0u;
constexpr Index kFirstPlayerNumber = 1u;
constexpr Index kMaxPlayers = 3u;
constexpr Index kBoardSize = 16;

void printBoard(BoardState board, bool digitsOnEmptySpaces = true) {
    constexpr std::array<char, 4> playerSymbols{'.', 'X', 'O', '#'};
    std::cout << std::hex;
    for (Index position = 0; position < kBoardSize; ++position) {
        auto shift = (position << 1u);
        auto mask = 0b11u << shift;
        Index playerNumber = (board & mask) >> shift;
        if (playerNumber == kNoPlayerNumber && digitsOnEmptySpaces) {
            std::cout << position;
        } else {
            std::cout << playerSymbols.at(playerNumber);
        }

        auto modulo4 = position - ((position >> 2u) << 2u);
        if (modulo4 < 3u) {
            std::cout << " ";
        } else {
            std::cout << std::endl;
        }
    }
    std::cout << std::dec;
}

class TictactoeBoard {
public:
    static inline Index getCellShift(Index cell) {
        return cell << 1u;
    }

    bool cellIsOccupied(Index cell) const {
        CellsMask cellMask = 0b11u << getCellShift(cell);
        return static_cast<bool>(board_ & cellMask);
    }

    int countEmptyCells() const {
        int count = 0;
        for (Index cell = 0; cell < kBoardSize; ++cell) {
            if (!cellIsOccupied(cell)) {
                ++count;
            }
        }
        return count;
    }

    void playOneMove(Index playerNumber, Index cell) {
        if (playerNumber < kFirstPlayerNumber || playerNumber > kMaxPlayers) {
            throw std::invalid_argument("invalid player index in playOneMove");
        }
        if (cellIsOccupied(cell)) {
            throw std::invalid_argument("trying to play on occupied cell");
        }
        auto playerMask = playerNumber << getCellShift(cell);
        board_ |= playerMask;
    }

    BoardState internalRepresentation() const {
        return board_;
    }

    void print() const {
        printBoard(board_, false);
    }

private:
    BoardState board_ = 0u;
};

class BoardStateChecker {
public:
    BoardStateChecker(int seed, int count) {
        std::mt19937 generator(seed);
        std::uniform_int_distribution<uint32_t> positionDistribution(0, kBoardSize - 1);
        winningSubsets.reserve(count);
        for (Index winningSubsetIndex = 0; winningSubsetIndex < count; ++winningSubsetIndex) {
            auto subsetSize = std::uniform_int_distribution<int>(3, 5)(generator);
            std::vector<Index> positions;
            for (Index positionIndex = 0; positionIndex < subsetSize; ++positionIndex) {
                Index nextPosition;
                do {
                    nextPosition = positionDistribution(generator);
                } while (std::find(positions.begin(), positions.end(), nextPosition) != positions.end());
                positions.emplace_back(nextPosition);
            }
            winningSubsets.emplace_back(positions);
        }
    }

    Index getWinnerNumber(const TictactoeBoard &board) const {
        for (const auto &winningSubset : winningSubsets) {
            auto winner = winningSubset.getWinner(board.internalRepresentation());
            if (winner != kNoPlayerNumber) {
                return winner;
            }
        }
        return kNoPlayerNumber;
    }

    void printWinningSubsets() {
        for (const auto &subset : winningSubsets) {
            printBoard(subset.mask(), false);
            std::cout << std::endl;
        }
    }

    std::vector<CellsMask> getWinningSubsets() const {
        std::vector<CellsMask> result;
        for (const auto &subset : winningSubsets) {
            result.emplace_back(subset.mask());
        }
        return result;
    }

private:
    class WinningSubset {
    public:
        WinningSubset(WinningSubset &&other) = default;
        explicit WinningSubset(const std::vector<uint32_t> &positions) : playerWinMask{0u, 0u,0u} {
            for (const auto &pos : positions) {
                for (auto playerNumber = kFirstPlayerNumber; playerNumber <= kMaxPlayers; ++playerNumber) {
                    playerWinMask.at(playerNumber - 1) |= (playerNumber << (pos << 1u));
                }
            }
        }

        Index getWinner(BoardState board) const {
            board &= mask();
            for (auto playerIndex = kFirstPlayerNumber; playerIndex <= kMaxPlayers; ++playerIndex) {
                if (board == playerWinMask.at(playerIndex - 1)) {
                    return playerIndex;
                }
            }
            return 0u;
        }

        CellsMask mask() const {
            return playerWinMask.back();
        }
    private:
        std::array<BoardState, kMaxPlayers> playerWinMask;
    };

    std::vector<WinningSubset> winningSubsets;
};

class BasePlayer {
public:
    virtual Index getNextMove(TictactoeBoard board, const BoardStateChecker &stateChecker) {
        return getRandomMove(board);
    }

    virtual void reset() {}

    void setMyNumber(Index number) {
        myNumber = number;
    }

    virtual void registerOtherPlayerMove(Index playerNumber, Index movePosition) {}

    virtual std::string getName() const {
        return "Base";
    }

    static Index getRandomMove(TictactoeBoard board) {
        static std::mt19937 generator(std::random_device{}());
        auto targetMoveNumber = std::uniform_int_distribution(1, board.countEmptyCells())(generator);
        Index moveNumber = 0;
        for (Index move = 0; move < kBoardSize; ++move) {
            if (!board.cellIsOccupied(move)) {
                ++moveNumber;
                if (moveNumber == targetMoveNumber) {
                    return move;
                }
            }
        }
        throw std::logic_error("end of getRandomMove");
    }

    virtual ~BasePlayer() = default;

    Index myNumber = kNoPlayerNumber;
};

double getCurrentSeconds() {
    auto timePointNow = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto durationNow = std::chrono::duration_cast<std::chrono::microseconds>(timePointNow);
    constexpr double kPrecision = 1.e-6;
    return static_cast<double>(durationNow.count()) * kPrecision;
}

void playTictactoe(std::vector<std::unique_ptr<BasePlayer>> &players, const int iterations, bool verbose = false) {
    auto startTime = getCurrentSeconds();
    const auto kReseedFrequency = 1000000;
    std::random_device rd;
    std::mt19937 generator(rd());
    std::vector<uint32_t> winsByPlayer(players.size() + 1);
    std::vector<double> timeSpentByPlayer(players.size() + 1);

    for (auto playerIndex = 0; playerIndex < players.size(); ++playerIndex) {
        players.at(playerIndex)->setMyNumber(playerIndex + 1);
    }

    auto iterationsBeforeReseed = kReseedFrequency;
    for (auto i = 0; i < iterations; ++i) {
        --iterationsBeforeReseed;
        if (iterationsBeforeReseed == 0) {
            iterationsBeforeReseed = kReseedFrequency;
            generator.seed(rd());
        }

        for (auto &player : players) {
            player->reset();
        }

        TictactoeBoard board{};
        BoardStateChecker checker(generator(), 15);

        auto turnDecider = std::uniform_int_distribution<Index>(0, players.size() - 1);
        for (auto turn = 0; turn < kBoardSize; ++turn) {
            auto playerIndexToMove = turnDecider(generator);
            Index playerNumberToMove = playerIndexToMove + 1;
            auto timeBeforeMove = getCurrentSeconds();
            Index nextMove = players.at(playerIndexToMove)->getNextMove(board, checker);
            timeSpentByPlayer.at(playerNumberToMove) += (getCurrentSeconds() - timeBeforeMove);
            board.playOneMove(playerNumberToMove, nextMove);
            if (verbose) {
                board.print();
                std::cout << std::endl;
            }

            auto winnerNumber = checker.getWinnerNumber(board);
            if (winnerNumber != kNoPlayerNumber || turn == kBoardSize - 1) {
                ++winsByPlayer.at(winnerNumber);
                break;
            }

            for (auto playerIndex = 0; playerIndex < players.size(); ++playerIndex) {
                auto playerNumber = playerIndex + 1;
                if (playerIndex != playerIndexToMove) {
                    players.at(playerIndex)->registerOtherPlayerMove(playerNumber, nextMove);
                }
            }
        }
    }
    timeSpentByPlayer.at(kNoPlayerNumber) = getCurrentSeconds() - startTime;
    for (int i = 0; i < players.size(); ++i) {
        timeSpentByPlayer.at(kNoPlayerNumber) -= timeSpentByPlayer.at(i + 1);
    }
    for (Index playerNumber = 0; playerNumber < winsByPlayer.size(); ++playerNumber) {
        if (playerNumber == kNoPlayerNumber) {
            std::cout << "Draws:\t";
        } else {
            std::cout << players.at(playerNumber - 1)->getName() << " wins: ";
        }
        auto wins = winsByPlayer.at(playerNumber);
        std::cout << wins << " = " << static_cast<double>(wins) / iterations;
        std::cout << " time spent: " << timeSpentByPlayer.at(playerNumber) << "s\n";
    }
}

#endif // TICTACTOE_PLAYER_H
