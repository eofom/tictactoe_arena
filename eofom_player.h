#ifndef TICTACTOE_EOFOM_PLAYER_H
#define TICTACTOE_EOFOM_PLAYER_H

#include <array>
#include <cmath>
#include <bitset>
#include "lib.h"

class EofomPlayer : public BasePlayer {
public:
    struct PositionScore {
        int firstLevelAdvantage = std::numeric_limits<int>::min();
        int secondLevelAdvantage = std::numeric_limits<int>::min();
        bool immediateWin = false;

        bool operator<(const PositionScore &other) const {
            return firstLevelAdvantage < other.firstLevelAdvantage
                || (firstLevelAdvantage == other.firstLevelAdvantage && secondLevelAdvantage < other.secondLevelAdvantage);
        }
    };

    struct WinningSubsetState {
        CellsMask mask;
        int empty = 0;
        Index playerNumber = kNoPlayerNumber;
        bool noOnes = false;
        void addPlayer(Index addedPlayerNumber)
        {
            if (addedPlayerNumber == kNoPlayerNumber) { ++empty; }
            else {
                if (playerNumber == kNoPlayerNumber) {
                    playerNumber = addedPlayerNumber;
                } else if (playerNumber != addedPlayerNumber) {
                    noOnes = true;
                    return;
                }
            }
        }
    };

    Index getNextMove(TictactoeBoard board, const BoardStateChecker& stateChecker) final {
        auto winningSubsets = stateChecker.getWinningSubsets();
        std::vector<WinningSubsetState> subsetsStates;
        subsetsStates.reserve(winningSubsets.size());
        for (auto subset : winningSubsets) {
            subsetsStates.emplace_back(WinningSubsetState{subset});
        }

        for (Index pos = 0; pos < kBoardSize; ++pos) {
            auto playerNumberOnCell = board.getPlayerOnCell(pos);
            for (auto &state : subsetsStates) {
                if (state.noOnes) {
                    continue;
                }
                if (cellIsInSubset(state.mask, pos)) {
                    state.addPlayer(playerNumberOnCell);
                }
            }
        }

        for (auto &state : subsetsStates) {
            if (!state.noOnes && state.empty == 0) {
                printBoard(state.mask, false);
                throw std::logic_error("empty subset state");
            }
        }

        Index nextMove = -1;
        PositionScore score;
        for (Index move = 0; move < kBoardSize; ++move) {
            if (board.cellIsOccupied(move)) {
                continue;
            }
            auto moveScore = getPosititonScore(move, subsetsStates);
            if (moveScore.immediateWin) {
                return move;
            }
            if (score < moveScore) {
                score = moveScore;
                nextMove = move;
            }
        }
        if (nextMove == -1) {
            throw std::logic_error("undecided move");
        }
        return nextMove;
    }

    PositionScore getPosititonScore(Index move, const std::vector<WinningSubsetState> &states) {
        int myClosestWin = kBoardSize;
        int theirClosestWin = kBoardSize;
        int myVariability = 0;
        int theirVariability = 0;
        for (const auto &state : states) {
            if (state.noOnes) {continue;}
            Index subsetPlayer = state.playerNumber;
            int subsetLeft = state.empty;
            if (subsetLeft == 0) {
                throw std::logic_error("filled subset in get position score");
            }
            if (cellIsInSubset(state.mask, move)) {
                if (subsetPlayer != myNumber && subsetPlayer != kNoPlayerNumber) {
                    continue;
                }
                subsetPlayer = myNumber;
                --subsetLeft;
                if (subsetLeft == 0) {
                    return {0, 0, true};
                }
            }

            if (subsetPlayer == myNumber || subsetPlayer == kNoPlayerNumber) {
                if (myClosestWin == subsetLeft) {
                    ++myVariability;
                } else if (subsetLeft < myClosestWin) {
                    myClosestWin = subsetLeft;
                    myVariability = 1;
                }
            }

            if (subsetPlayer != myNumber) {
                if (theirClosestWin == subsetLeft) {
                    ++theirVariability;
                } else if (subsetLeft < theirClosestWin) {
                    theirClosestWin = subsetLeft;
                    theirVariability = 1;
                }
            }
        }
        if (theirVariability == 0) {
            return {100, -myClosestWin * 10 + myVariability};
        }
        if (theirClosestWin < 1) {
            throw std::logic_error("they won");
        }
        if (theirClosestWin == 1) {
            return {-100, -theirVariability};
        }

        return {theirClosestWin - myClosestWin, myVariability - theirVariability};
    }

    std::string getName() const final {
        return "Eofom";
    }
};

#endif //TICTACTOE_EOFOM_PLAYER_H
