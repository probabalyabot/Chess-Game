#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>

std::map<char, int> fileMap = {
    {'a', 0}, {'b', 1}, {'c', 2}, {'d', 3},
    {'e', 4}, {'f', 5}, {'g', 6}, {'h', 7}
};
std::map<char, float> pieceValues = {
    {'P', 1},   {'p', -1},
    {'N', 3},   {'n', -3},
    {'B', 4},   {'b', -4},
    {'R', 5},   {'r', -5},
    {'Q', 9},   {'q', -9},
    {'K', 100}, {'k', -100}
};

// --- Exception hierarchy ---
struct ChessException : public std::runtime_error {
    explicit ChessException(const std::string& msg) : std::runtime_error(msg) {}
};
struct InvalidFormatException : public ChessException {
    explicit InvalidFormatException(const std::string& msg) : ChessException(msg) {}
};
struct SameSquareException : public ChessException {
    explicit SameSquareException(const std::string& msg) : ChessException(msg) {}
};
struct EmptySquareException : public ChessException {
    explicit EmptySquareException(const std::string& msg) : ChessException(msg) {}
};
struct WrongTurnException : public ChessException {
    explicit WrongTurnException(const std::string& msg) : ChessException(msg) {}
};
struct FriendlyFireException : public ChessException {
    explicit FriendlyFireException(const std::string& msg) : ChessException(msg) {}
};
struct InvalidPieceMoveException : public ChessException {
    explicit InvalidPieceMoveException(const std::string& msg) : ChessException(msg) {}
};
struct MovesIntoCheckException : public ChessException {
    explicit MovesIntoCheckException(const std::string& msg) : ChessException(msg) {}
};

class chessboard {
private:
    char board[8][8];
    int moveCounter = 0;
    int enPassantRow = -1;
    int enPassantCol = -1;

    bool isPathClearRook(int fromRow, int fromCol, int toRow, int toCol) {
        int rowStep = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
        int colStep = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
        int currentRow = fromRow + rowStep;
        int currentCol = fromCol + colStep;
        while (currentRow != toRow || currentCol != toCol) {
            if (board[currentRow][currentCol] != '.') return false;
            currentRow += rowStep;
            currentCol += colStep;
        }
        return true;
    }

    bool isPathClearBishop(int fromRow, int fromCol, int toRow, int toCol) {
        int rowStep = (toRow > fromRow) ? 1 : -1;
        int colStep = (toCol > fromCol) ? 1 : -1;
        int currentRow = fromRow + rowStep;
        int currentCol = fromCol + colStep;
        while (currentRow != toRow && currentCol != toCol) {
            if (board[currentRow][currentCol] != '.') return false;
            currentRow += rowStep;
            currentCol += colStep;
        }
        return true;
    }

    bool isValidKnightMove(int fromRow, int fromCol, int toRow, int toCol) {
        int rowDiff = abs(toRow - fromRow);
        int colDiff = abs(toCol - fromCol);
        return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
    }

    bool isValidBishopMove(int fromRow, int fromCol, int toRow, int toCol) {
        if (abs(toRow - fromRow) != abs(toCol - fromCol)) return false;
        return isPathClearBishop(fromRow, fromCol, toRow, toCol);
    }

    bool isValidRookMove(int fromRow, int fromCol, int toRow, int toCol) {
        if (fromRow != toRow && fromCol != toCol) return false;
        return isPathClearRook(fromRow, fromCol, toRow, toCol);
    }

    bool isValidQueenMove(int fromRow, int fromCol, int toRow, int toCol) {
        return isValidRookMove(fromRow, fromCol, toRow, toCol) ||
               isValidBishopMove(fromRow, fromCol, toRow, toCol);
    }

    bool isValidKingMove(int fromRow, int fromCol, int toRow, int toCol) {
        return abs(toRow - fromRow) <= 1 && abs(toCol - fromCol) <= 1;
    }

    bool isValidPawnMove(char piece, int fromRow, int fromCol, int toRow, int toCol) {
        int direction = (piece == 'P') ? -1 : 1;
        if (fromCol == toCol) {
            if (toRow - fromRow == direction && board[toRow][toCol] == '.') return true;
            if ((fromRow == 6 && piece == 'P') || (fromRow == 1 && piece == 'p')) {
                if (toRow - fromRow == 2 * direction &&
                    board[fromRow + direction][toCol] == '.' &&
                    board[toRow][toCol] == '.') return true;
            }
        } else if (abs(toCol - fromCol) == 1 && toRow - fromRow == direction) {
            if (board[toRow][toCol] != '.' &&
                ((piece == 'P' && islower(board[toRow][toCol])) ||
                 (piece == 'p' && isupper(board[toRow][toCol])))) return true;
            if (toRow == enPassantRow && toCol == enPassantCol) return true;
        }
        return false;
    }

    // Pure attack geometry for pawns — no board state, no en passant.
    // Used by isInCheck so we don't conflate "legal move" with "attacks square".
    bool isPawnAttacking(char piece, int fromRow, int fromCol, int toRow, int toCol) {
        int direction = (piece == 'P') ? -1 : 1;
        return (toRow - fromRow == direction) && (abs(toCol - fromCol) == 1);
    }

    bool isValidMove(char piece, int fromRow, int fromCol, int toRow, int toCol) {
        switch (piece) {
            case 'P': case 'p': return isValidPawnMove(piece, fromRow, fromCol, toRow, toCol);
            case 'N': case 'n': return isValidKnightMove(fromRow, fromCol, toRow, toCol);
            case 'B': case 'b': return isValidBishopMove(fromRow, fromCol, toRow, toCol);
            case 'R': case 'r': return isValidRookMove(fromRow, fromCol, toRow, toCol);
            case 'Q': case 'q': return isValidQueenMove(fromRow, fromCol, toRow, toCol);
            case 'K': case 'k': return isValidKingMove(fromRow, fromCol, toRow, toCol);
            default: return false;
        }
    }

    // Does piece at (fromRow, fromCol) attack square (toRow, toCol)?
    // Same as isValidMove but routes pawns through isPawnAttacking.
    bool isAttacking(char piece, int fromRow, int fromCol, int toRow, int toCol) {
        switch (piece) {
            case 'P': case 'p': return isPawnAttacking(piece, fromRow, fromCol, toRow, toCol);
            case 'N': case 'n': return isValidKnightMove(fromRow, fromCol, toRow, toCol);
            case 'B': case 'b': return isValidBishopMove(fromRow, fromCol, toRow, toCol);
            case 'R': case 'r': return isValidRookMove(fromRow, fromCol, toRow, toCol);
            case 'Q': case 'q': return isValidQueenMove(fromRow, fromCol, toRow, toCol);
            case 'K': case 'k': return isValidKingMove(fromRow, fromCol, toRow, toCol);
            default: return false;
        }
    }

public:
    chessboard() {
        char initialBoard[8][8] = {
            {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
            {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'.', '.', '.', '.', '.', '.', '.', '.'},
            {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
            {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
        };
        for (int i = 0; i < 8; i++)
            for (int j = 0; j < 8; j++)
                board[i][j] = initialBoard[i][j];
    }

    void printBoard() {
        for (int i = 0; i < 8; i++) {
            std::cout << (8 - i) << "  ";
            for (int j = 0; j < 8; j++)
                std::cout << board[i][j] << " ";
            std::cout << "\n";
        }
        std::cout << "\n   a b c d e f g h\n\n";
    }

    int getTurn() {
        return (moveCounter % 2 == 0) ? 1 : 2;
    }

    // 1 = is white king in check, 2 = is black king in check
    bool isInCheck(int turn) {
        char kingChar = (turn == 1) ? 'K' : 'k';

        int kingRow = -1, kingCol = -1;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (board[r][c] == kingChar) { kingRow = r; kingCol = c; }

        if (kingRow == -1) return false;

        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                char piece = board[r][c];
                if (piece == '.') continue;
                bool isEnemy = (turn == 1) ? islower(piece) : isupper(piece);
                if (!isEnemy) continue;
                if (isAttacking(piece, r, c, kingRow, kingCol)) return true;
            }
        }
        return false;
    }

    void makeMove(std::string from, std::string to) {
        const std::regex movePattern("[a-h][1-8]");
        if (!std::regex_match(from, movePattern) || !std::regex_match(to, movePattern))
            throw InvalidFormatException("Invalid format: use a-h for file and 1-8 for rank (e.g. e2).");

        if (from == to)
            throw SameSquareException("Source and destination squares are the same.");

        int fromletter = fileMap[from[0]];
        int toletter   = fileMap[to[0]];
        int fromnum    = 8 - (from[1] - '0');
        int tonum      = 8 - (to[1] - '0');

        if (board[fromnum][fromletter] == '.')
            throw EmptySquareException("No piece on square " + from + ".");

        if (getTurn() == 1 && pieceValues[board[fromnum][fromletter]] < 0)
            throw WrongTurnException("It is white's turn. Please move a white piece.");
        if (getTurn() == 2 && pieceValues[board[fromnum][fromletter]] > 0)
            throw WrongTurnException("It is black's turn. Please move a black piece.");

        char targetPiece = board[tonum][toletter];
        if (getTurn() == 1 && isupper(targetPiece))
            throw FriendlyFireException("You cannot capture your own piece on " + to + ".");
        if (getTurn() == 2 && islower(targetPiece))
            throw FriendlyFireException("You cannot capture your own piece on " + to + ".");

        if (!isValidMove(board[fromnum][fromletter], fromnum, fromletter, tonum, toletter))
            throw InvalidPieceMoveException(
                std::string(1, board[fromnum][fromletter]) + " cannot move from " + from + " to " + to + "."
            );

        // Speculative execution — apply move, test for self-check, rollback if needed
        char savedFrom    = board[fromnum][fromletter];
        char savedTo      = board[tonum][toletter];
        int  savedEPRow   = enPassantRow;
        int  savedEPCol   = enPassantCol;
        int  epCaptureRow = -1;

        enPassantRow = enPassantCol = -1;

        if (savedFrom == 'P' || savedFrom == 'p') {
            int direction = (savedFrom == 'P') ? -1 : 1;
            if (tonum - fromnum == 2 * direction) {
                enPassantRow = fromnum + direction;
                enPassantCol = fromletter;
            }
            if (abs(toletter - fromletter) == 1 && board[tonum][toletter] == '.') {
                epCaptureRow = fromnum;
                board[fromnum][toletter] = '.';
            }
        }

        board[tonum][toletter]     = savedFrom;
        board[fromnum][fromletter] = '.';

        if (isInCheck(getTurn())) {
            board[fromnum][fromletter] = savedFrom;
            board[tonum][toletter]     = savedTo;
            enPassantRow = savedEPRow;
            enPassantCol = savedEPCol;
            if (epCaptureRow != -1)
                board[epCaptureRow][toletter] = (savedFrom == 'P') ? 'p' : 'P';
            throw MovesIntoCheckException("Illegal move: your king would be in check.");
        }

        // Move is legal — commit
        moveCounter++;

        // Pawn promotion
        if ((board[tonum][toletter] == 'P' && tonum == 0) ||
            (board[tonum][toletter] == 'p' && tonum == 7)) {
            char choice = 'Q';
            std::cout << "Pawn promotion! Choose piece (Q/R/B/N): ";
            std::cin >> choice;
            choice = toupper(choice);
            switch (choice) {
                case 'Q': case 'R': case 'B': case 'N': break;
                default: choice = 'Q'; break;
            }
            board[tonum][toletter] = (savedFrom == 'P') ? toupper(choice) : tolower(choice);
        }

        // Announce check on opponent
        int opponent = (getTurn() == 1) ? 1 : 2;
        if (isInCheck(opponent))
            std::cout << ((opponent == 1) ? "White" : "Black") << " is in check!\n";
    }
};

int main() {
    std::cout << "Welcome to the chess game!\n\n";
    chessboard board;
    while (true) {
        board.printBoard();
        std::string from, to;
        std::cout << (board.getTurn() == 1 ? "White" : "Black")
                  << "'s turn. Enter move (e.g., e2 e4): ";
        std::cin >> from >> to;
        try {
            board.makeMove(from, to);
        } catch (const ChessException& e) {
            std::cout << "Error: " << e.what() << "\n";
        }
    }
    return 0;
}