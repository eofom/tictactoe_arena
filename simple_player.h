#ifndef TICTACTOE_SIMPLE_PLAYER_H
#define TICTACTOE_SIMPLE_PLAYER_H

class SimplePlayer : public BasePlayer {
public:
    Index getNextMove(TictactoeBoard board, const BoardStateChecker &stateChecker) final {
        std::optional<Index> counterMove;
        for (Index move = 0; move < kBoardSize; ++move) {
            if (board.cellIsOccupied(move)) {
                continue;
            }

            TictactoeBoard boardCopy(board);
            boardCopy.playOneMove(myNumber, move);
            auto winningPlayerNumber = stateChecker.getWinnerNumber(boardCopy);

            if (winningPlayerNumber == kNoPlayerNumber) {
                continue;
            } else if (winningPlayerNumber == myNumber) {
                return move;
            } else {
                counterMove = move;
            }
        }
        if (counterMove) {
            return *counterMove;
        }
        return getRandomMove(board);
    }

    std::string getName() const final {
        return "Simple";
    }
};

#endif //TICTACTOE_SIMPLE_PLAYER_H
