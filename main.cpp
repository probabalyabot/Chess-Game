#include <iostream>
#include <map>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>    
#include <sstream>   

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

//Exceptions
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

    // ___Castling rights___

    bool whiteKingMoved  = false;
    bool blackKingMoved  = false;
    bool whiteRookAMoved = false; // a1 rook (queenside)
    bool whiteRookHMoved = false; // h1 rook (kingside)
    bool blackRookAMoved = false; // a8 rook (queenside)
    bool blackRookHMoved = false; // h8 rook (kingside)

    // ___Draw tracking___
    // Counts half-moves since the last pawn move or capture.
    // Under the fifty-move rule, if this hits 100 (50 full moves) it's a draw.
    int halfMoveClock = 0;

    // store a snapshot of the board after every move as a string.

    std::vector<std::string> positionHistory;

    // ___Move history for display___
    // save each move algebraic notation
    std::vector<std::string> moveHistory;
    // ___Castling square constants___
    static const int WHITE_KING_ROW      = 7;
    static const int BLACK_KING_ROW      = 0;
    static const int KING_COL            = 4;
    static const int KINGSIDE_ROOK_COL   = 7;
    static const int QUEENSIDE_ROOK_COL  = 0;
    static const int KINGSIDE_KING_DST   = 6;
    static const int QUEENSIDE_KING_DST  = 2;
    static const int KINGSIDE_ROOK_DST   = 5;
    static const int QUEENSIDE_ROOK_DST  = 3;


    bool isPathClearRook(int fromRow, int fromCol, int toRow, int toCol) {
        int rowStep = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
        int colStep = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;//check which direction the rook is moving
        int currentRow = fromRow + rowStep;
        int currentCol = fromCol + colStep;
        while (currentRow != toRow || currentCol != toCol) {//iterates thru each square from start to finish and check if null is present
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

    // Pure attack geometry for pawns = no board state, no en passant.
    // Used by isInCheck so we don't confuse "legal move" with "attacks square".
    // this is to check if a pawn is attacking, so we route it thru isAttacking
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

    // check if attack = valid
    // Same as isValidMove but takes pawns thru isPawnAttacking, as diaonal movement when capturing need to be accounted
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

    // ___ Castling validation___
    bool canCastle(int turn, bool kingSide) {
    if (turn == 1) {
        if (whiteKingMoved) return false;
        if (kingSide) {
            if (whiteRookHMoved) return false;
            if (board[WHITE_KING_ROW][KINGSIDE_ROOK_DST] != '.' ||
                board[WHITE_KING_ROW][KINGSIDE_KING_DST] != '.') return false;
            if (isInCheck(1)) return false;
            if (isSquareAttackedBy(WHITE_KING_ROW, KINGSIDE_ROOK_DST, 2)) return false;
            if (isSquareAttackedBy(WHITE_KING_ROW, KINGSIDE_KING_DST, 2)) return false;
        } else {
            if (whiteRookAMoved) return false;
            if (board[WHITE_KING_ROW][1] != '.' ||
                board[WHITE_KING_ROW][QUEENSIDE_ROOK_DST] != '.' ||
                board[WHITE_KING_ROW][QUEENSIDE_KING_DST] != '.') return false;
            if (isInCheck(1)) return false;
            if (isSquareAttackedBy(WHITE_KING_ROW, QUEENSIDE_ROOK_DST, 2)) return false;
            if (isSquareAttackedBy(WHITE_KING_ROW, QUEENSIDE_KING_DST, 2)) return false;
        }
    } else {
        if (blackKingMoved) return false;
        if (kingSide) {
            if (blackRookHMoved) return false;
            if (board[BLACK_KING_ROW][KINGSIDE_ROOK_DST] != '.' ||
                board[BLACK_KING_ROW][KINGSIDE_KING_DST] != '.') return false;
            if (isInCheck(2)) return false;
            if (isSquareAttackedBy(BLACK_KING_ROW, KINGSIDE_ROOK_DST, 1)) return false;
            if (isSquareAttackedBy(BLACK_KING_ROW, KINGSIDE_KING_DST, 1)) return false;
        } else {
            if (blackRookAMoved) return false;
            if (board[BLACK_KING_ROW][1] != '.' ||
                board[BLACK_KING_ROW][QUEENSIDE_ROOK_DST] != '.' ||
                board[BLACK_KING_ROW][QUEENSIDE_KING_DST] != '.') return false;
            if (isInCheck(2)) return false;
            if (isSquareAttackedBy(BLACK_KING_ROW, QUEENSIDE_ROOK_DST, 1)) return false;
            if (isSquareAttackedBy(BLACK_KING_ROW, QUEENSIDE_KING_DST, 1)) return false;
        }
    }
    return true;
}

    // Checks whether a given square is attacked by any piece belonging to "attackerTurn".
    // Used for castling checks and general king safety validation.
    bool isSquareAttackedBy(int row, int col, int attackerTurn) {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                char piece = board[r][c];
                if (piece == '.') continue;
                bool isPieceOfAttacker = (attackerTurn == 1) ? isupper(piece) : islower(piece);
                if (!isPieceOfAttacker) continue;
                if (isAttacking(piece, r, c, row, col)) return true;
            }
        }
        return false;
    }

    // Generates a unique string snapshot of the current board position
    // This encodes board state + en passant target + castling rights.
    // Two identical strings = identical game positions (for threefold repetition).
    std::string getBoardHash() {
        std::ostringstream ss;
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                ss << board[r][c];
        // Append en passant target square so the same board with different
        // en passant availability doesn't count as the same position.
        ss << enPassantRow << enPassantCol;
        // Append castling rights — losing castling rights changes the position.
        ss << whiteKingMoved << whiteRookAMoved << whiteRookHMoved
           << blackKingMoved << blackRookAMoved << blackRookHMoved;
        return ss.str();
    }

    // Checks if a given player has ANY legal move available

    bool hasLegalMove(int turn) {
        for (int fr = 0; fr < 8; fr++) {
            for (int fc = 0; fc < 8; fc++) {
                char piece = board[fr][fc];
                if (piece == '.') continue;
                bool isOwnPiece = (turn == 1) ? isupper(piece) : islower(piece);
                if (!isOwnPiece) continue;

                // Try every square on the board as a destination
                for (int tr = 0; tr < 8; tr++) {
                    for (int tc = 0; tc < 8; tc++) {
                        if (fr == tr && fc == tc) continue;

                        // Can't capture your own piece
                        char target = board[tr][tc];
                        if (turn == 1 && isupper(target)) continue;
                        if (turn == 2 && islower(target)) continue;

                        // Is the piece move geometrically valid?
                        if (!isValidMove(piece, fr, fc, tr, tc)) continue;

                        // Simulate the move and check if it leaves the king in check.
                        // We have to manually replicate what makeMove does here (without
                        // all the side effects) because makeMove throws exceptions.
                        char savedFrom  = board[fr][fc];
                        char savedTo    = board[tr][tc];
                        int savedEPRow  = enPassantRow;
                        int savedEPCol  = enPassantCol;
                        int epCapRow    = -1;

                        // Handle en passant capture in simulation
                        if ((piece == 'P' || piece == 'p') &&
                            abs(tc - fc) == 1 && board[tr][tc] == '.') {
                            epCapRow = fr;
                            board[fr][tc] = '.';
                        }

                        board[tr][tc] = savedFrom;
                        board[fr][fc] = '.';

                        bool inCheck = isInCheck(turn);

                        // Undo the simulation
                        board[fr][fc] = savedFrom;
                        board[tr][tc] = savedTo;
                        enPassantRow  = savedEPRow;
                        enPassantCol  = savedEPCol;
                        if (epCapRow != -1)
                            board[epCapRow][tc] = (piece == 'P') ? 'p' : 'P';

                        if (!inCheck) return true; // Found at least one legal move = not stuck
                    }
                }

                // Also check if castling is available = that counts as a legal move too
                if ((turn == 1 && piece == 'K') || (turn == 2 && piece == 'k')) {
                    if (canCastle(turn, true))  return true;
                    if (canCastle(turn, false)) return true;
                }
            }
        }
        return false; // No legal move found anywhere
    }

    // Insufficient material check 
    // Returns true if neither side has enough pieces to force checkmate.
    // Drawn positions:
    //   K vs K
    //   K+B vs K
    //   K+N vs K
    //   K+B vs K+B (both bishops on same color — but we simplify to K+B vs K+B)
    // We count each side's pieces and flag if no mating material exists.
    bool isInsufficientMaterial() {
        std::vector<char> whitePieces, blackPieces;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                char p = board[r][c];
                if (p == '.') continue;
                if (isupper(p) && p != 'K') whitePieces.push_back(p);
                if (islower(p) && p != 'k') blackPieces.push_back(p);
            }
        }

        // If either side still has a queen, rook, or pawn, checkmate is possible
        auto hasMatingPiece = [](const std::vector<char>& pieces) {
            for (char p : pieces)
                if (toupper(p) == 'Q' || toupper(p) == 'R' || toupper(p) == 'P')
                    return true;
            return false;
        };

        if (hasMatingPiece(whitePieces) || hasMatingPiece(blackPieces)) return false;

        // only have kings, knights, and bishops left
        // K vs K = draw
        if (whitePieces.empty() && blackPieces.empty()) return true;

        // K+minor vs K = not enough to mate
        if ((whitePieces.size() == 1 && blackPieces.empty()) ||
            (whitePieces.empty() && blackPieces.size() == 1)) return true;

        // K+B vs K+B technically only a draw if same color squares,
        // I cba so its just draw if both sides have 1 bishop left
        if (whitePieces.size() == 1 && blackPieces.size() == 1 &&
            toupper(whitePieces[0]) == 'B' && toupper(blackPieces[0]) == 'B') return true;

        return false;
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

        // Record the starting position so 3 repetition can count
        positionHistory.push_back(getBoardHash());
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
        return (moveCounter % 2 == 0) ? 1 : 2;// 1 = white, 2 = black
    }

    // 1 = white king in check, 2 = black king in check
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

   
    // Call after every successful move to check if game end

    bool checkGameOver(std::string& outResult) {
        int currentTurn = getTurn(); // whose turn it is NOW 

        // No legal moves = either checkmate or stalemate
        if (!hasLegalMove(currentTurn)) {
            if (isInCheck(currentTurn)) {
                // The player who just moved (the other side) wins.
                outResult = (currentTurn == 1)
                    ? "Checkmate! Black wins!"
                    : "Checkmate! White wins!";
            } else {
                // Not in check but no moves = stalemate
                outResult = "Stalemate! It's a draw.";
            }
            return true;
        }

        // Fifty-move rule: 100 half-moves (50 full moves) without a pawn move or capture
        if (halfMoveClock >= 100) {
            outResult = "Draw by the fifty-move rule!";
            return true;
        }

        // Threefold repetition: count how many times this exact position has appeared
        std::string currentHash = getBoardHash();
        int repetitions = 0;
        for (const std::string& h : positionHistory)
            if (h == currentHash) repetitions++;
        if (repetitions >= 3) {
            outResult = "Draw by threefold repetition!";
            return true;
        }

        // Insufficient material: neither side can force checkmate
        if (isInsufficientMaterial()) {
            outResult = "Draw by insufficient material!";
            return true;
        }

        return false; // Game is still going
    }

    void makeMove(std::string from, std::string to) {
        // ___Special command: castling___
        if (from == "O-O" || from == "o-o") {
            performCastle(getTurn(), true);
            return;
        }
        if (from == "O-O-O" || from == "o-o-o") {
            performCastle(getTurn(), false);
            return;
        }

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

        // Speculative execution => apply move, test for self-check, go back if needed
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

        // ___Update castling rights___
        // If king/rook moves or gets captured, it forefits casteling rights 
        if (savedFrom == 'K') whiteKingMoved  = true;
        if (savedFrom == 'k') blackKingMoved  = true;
        if (fromnum == 7 && fromletter == 0) whiteRookAMoved = true; // a1
        if (fromnum == 7 && fromletter == 7) whiteRookHMoved = true; // h1
        if (fromnum == 0 && fromletter == 0) blackRookAMoved = true; // a8
        if (fromnum == 0 && fromletter == 7) blackRookHMoved = true; // h8
        //  revoke castling rights if a rook gets captured on its home square
        if (tonum == 7 && toletter == 0) whiteRookAMoved = true;
        if (tonum == 7 && toletter == 7) whiteRookHMoved = true;
        if (tonum == 0 && toletter == 0) blackRookAMoved = true;
        if (tonum == 0 && toletter == 7) blackRookHMoved = true;

        // Update the half-move clock (for 50 move rule)
        // Reset to 0 on pawn moves and captures; increment otherwise.
        bool isCapture = (savedTo != '.') || (epCaptureRow != -1);
        bool isPawnMove = (savedFrom == 'P' || savedFrom == 'p');
        if (isCapture || isPawnMove)
            halfMoveClock = 0;
        else
            halfMoveClock++;

        // now move = legal
        moveCounter++;

        //Record the move in history
        moveHistory.push_back(from + "-" + to);

        // Record board position for 3 repetition
        positionHistory.push_back(getBoardHash());

        // Pawn promotion - techinically shouldnt have input here but again too much work to restructure
        if((board[tonum][toletter] == 'P' && tonum == 0) || (board[tonum][toletter] == 'p' && tonum == 7)){
            char choice = 'Q';
            std::cout << "Pawn promotion! Choose piece (Q/R/B/N): ";
            while (true) {
                std::cin >> choice;
                choice = toupper(choice);
                if (choice == 'Q' || choice == 'R' || choice == 'B' || choice == 'N') break;
                std::cout << "Invalid choice. Please enter Q, R, B, or N: ";
        }
        board[tonum][toletter] = (savedFrom == 'P') ? toupper(choice) : tolower(choice);
    }


        int opponent = getTurn(); //points to whoever moves next
        if (isInCheck(opponent))
            std::cout << ((opponent == 1) ? "White" : "Black") << " is in check!\n";
    }

    // Performs a castling move
    // Moves the king two squares toward the rook, then hops the rook over.
    void performCastle(int turn, bool kingSide) {
    if (!canCastle(turn, kingSide))
        throw ChessException("Castling is not available.");

    int row = (turn == 1) ? WHITE_KING_ROW : BLACK_KING_ROW;

    if (kingSide){
        // King e→g, rook h→f
        board[row][KINGSIDE_KING_DST]  = board[row][KING_COL];
        board[row][KING_COL]           = '.';
        board[row][KINGSIDE_ROOK_DST]  = board[row][KINGSIDE_ROOK_COL];
        board[row][KINGSIDE_ROOK_COL]  = '.';
        moveHistory.push_back("O-O");
    } else {
        // King e→c, rook a→d
        board[row][QUEENSIDE_KING_DST] = board[row][KING_COL];
        board[row][KING_COL]           = '.';
        board[row][QUEENSIDE_ROOK_DST] = board[row][QUEENSIDE_ROOK_COL];
        board[row][QUEENSIDE_ROOK_COL] = '.';
        moveHistory.push_back("O-O-O");
    }

    if (turn == 1) {
        whiteKingMoved = true;
        if (kingSide) whiteRookHMoved = true;
        else          whiteRookAMoved = true;
    } else {
        blackKingMoved = true;
        if (kingSide) blackRookHMoved = true;
        else          blackRookAMoved = true;
    }

    halfMoveClock++;
    moveCounter++;

    positionHistory.push_back(getBoardHash());

    int opponent = getTurn();
    if (isInCheck(opponent))
        std::cout << ((opponent == 1) ? "White" : "Black") << " is in check!\n";
}

    // Prints a numbered list of all moves played so far.
    // Useful to review the game after it ends.
    void printMoveHistory() {
        std::cout << "\n Move History\n";
        for (int i = 0; i < (int)moveHistory.size(); i++) {
            if (i % 2 == 0)
                std::cout << (i / 2 + 1) << ". ";
            std::cout << moveHistory[i] << " ";
            if (i % 2 == 1)
                std::cout << "\n";
        }
        if (moveHistory.size() % 2 != 0) std::cout << "\n";
        std::cout << "\n";
    }

    // Generates a FEN string
    std::string toFEN() {
    std::ostringstream fen; 

    // 1. Piece placement
    for (int r = 0; r < 8; r++) {
        int empty = 0;
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == '.') {
                empty++;
            } else {
                if (empty > 0) { fen << empty; empty = 0; }
                fen << board[r][c];
            }
        }
        if (empty > 0) fen << empty;
        if (r < 7) fen << '/';
    }

    // 2. Active color
    fen << ' ' << (getTurn() == 1 ? 'w' : 'b');

    // 3. Castling rights
    std::string castling = "";
    if (!whiteKingMoved && !whiteRookHMoved) castling += 'K';
    if (!whiteKingMoved && !whiteRookAMoved) castling += 'Q';
    if (!blackKingMoved && !blackRookHMoved) castling += 'k';
    if (!blackKingMoved && !blackRookAMoved) castling += 'q';
    fen << ' ' << (castling.empty() ? "-" : castling);

    // 4. En passant target square
    if (enPassantRow == -1) {
        fen << " -";
    } else {
        char file = 'a' + enPassantCol;
        int  rank = 8 - enPassantRow;
        fen << ' ' << file << rank;
    }

    // 5. Halfmove clock
    fen << ' ' << halfMoveClock;

    // 6. Fullmove number
    fen << ' ' << (moveCounter / 2 + 1);

    return fen.str();
}
};

int main() {
    std::cout << "Welcome to the chess game!\n";
    std::cout << "Commands: enter moves as 'e2 e4', castle with 'O-O' or 'O-O-O',\n";
    std::cout << "          type 'resign' to concede, 'draw' to offer a draw.\n\n";

    chessboard board;

    // ____Main game loop____
    // check for EOF on every read so Ctrl+D doesn't spin forever.
    while (true) {
        board.printBoard();
        std::string from, to;
        std::cout << (board.getTurn() == 1 ? "White" : "Black")
                  << "'s turn. Enter move (e.g., e2 e4): ";

        // Safely read input — if stdin closes (Ctrl+D / EOF), exit
        if (!(std::cin >> from)) {
            std::cout << "\nInput ended. Goodbye!\n";
            break;
        }

        // ____Resign command_____
        // Either player can type "resign" to immediately concede the game.
        if (from == "resign") {
            std::cout << (board.getTurn() == 1 ? "White" : "Black")
                      << " resigns. "
                      << (board.getTurn() == 1 ? "Black" : "White")
                      << " wins!\n";
            board.printMoveHistory();
            break;

        }

        // ____Castling shorthand_______
        // The user types just "O-O" or "O-O-O"
        if (from == "O-O" || from == "o-o" ||
            from == "O-O-O" || from == "o-o-o") {
            try {
                board.makeMove(from, "");
            } catch (const ChessException& e) {
                std::cout << "Error: " << e.what() << "\n";
                continue;
            }
            std::string result;
            if (board.checkGameOver(result)) {
                board.printBoard();
                std::cout << result << "\n";
                board.printMoveHistory();
                break;
            }
            continue;
        }

        // ____Draw offer command____
        // Checked BEFORE reading 'to' so typing "draw" doesn't block
        // waiting for a second token that will never come.
        if (from == "draw") {
            std::cout << (board.getTurn() == 1 ? "Black" : "White")
                      << ", do you accept the draw? (y/n): ";
            char response;
            if (std::cin >> response && (response == 'y' || response == 'Y')) {
                std::cout << "Draw agreed!\n";
                board.printMoveHistory();
                break;
            } else {
                std::cout << "Draw declined. Game continues.\n";
                continue;
            }
        }

        // Read the destination square for normal moves
        if (!(std::cin >> to)) {
            std::cout << "\nInput ended. Goodbye!\n";
            break;
        }

        // ____Attempt the move____
        try {
            board.makeMove(from, to);
        } catch (const ChessException& e) {
            std::cout << "Error: " << e.what() << "\n";
            continue; // Bad move do again
        }

        //  ___Check for game-ending conditions after every successful move___
        std::string result;
        if (board.checkGameOver(result)) {
            board.printBoard();
            std::cout << result << "\n";
            board.printMoveHistory();
            break;
        }
        std::cout << "\nFEN: " << board.toFEN() << "\n"; // Print FEN for debugging
    }

    return 0;
}
