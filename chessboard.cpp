#include "chessboard.h"
#include <iostream>
#include <regex>


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

// ___Constructor___

chessboard::chessboard() {
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

    positionHistory.push_back(getBoardHash());
}

// ___Private helpers___

bool chessboard::isPathClearRook(int fromRow, int fromCol, int toRow, int toCol) {
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

bool chessboard::isPathClearBishop(int fromRow, int fromCol, int toRow, int toCol) {
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

bool chessboard::isValidKnightMove(int fromRow, int fromCol, int toRow, int toCol) {
    int rowDiff = abs(toRow - fromRow);
    int colDiff = abs(toCol - fromCol);
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}

bool chessboard::isValidBishopMove(int fromRow, int fromCol, int toRow, int toCol) {
    if (abs(toRow - fromRow) != abs(toCol - fromCol)) return false;
    return isPathClearBishop(fromRow, fromCol, toRow, toCol);
}

bool chessboard::isValidRookMove(int fromRow, int fromCol, int toRow, int toCol) {
    if (fromRow != toRow && fromCol != toCol) return false;
    return isPathClearRook(fromRow, fromCol, toRow, toCol);
}

bool chessboard::isValidQueenMove(int fromRow, int fromCol, int toRow, int toCol) {
    return isValidRookMove(fromRow, fromCol, toRow, toCol) ||
           isValidBishopMove(fromRow, fromCol, toRow, toCol);
}

bool chessboard::isValidKingMove(int fromRow, int fromCol, int toRow, int toCol) {
    return abs(toRow - fromRow) <= 1 && abs(toCol - fromCol) <= 1;
}

bool chessboard::isValidPawnMove(char piece, int fromRow, int fromCol, int toRow, int toCol) {
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

bool chessboard::isPawnAttacking(char piece, int fromRow, int fromCol, int toRow, int toCol) {
    int direction = (piece == 'P') ? -1 : 1;
    return (toRow - fromRow == direction) && (abs(toCol - fromCol) == 1);
}

bool chessboard::isValidMove(char piece, int fromRow, int fromCol, int toRow, int toCol) {
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

bool chessboard::isAttacking(char piece, int fromRow, int fromCol, int toRow, int toCol) {
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

bool chessboard::canCastle(int turn, bool kingSide) {
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

bool chessboard::isSquareAttackedBy(int row, int col, int attackerTurn) {
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

std::string chessboard::getBoardHash() {
    std::ostringstream ss;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            ss << board[r][c];
    ss << enPassantRow << enPassantCol;
    ss << whiteKingMoved << whiteRookAMoved << whiteRookHMoved
       << blackKingMoved << blackRookAMoved << blackRookHMoved;
    return ss.str();
}

bool chessboard::hasLegalMove(int turn) {
    for (int fr = 0; fr < 8; fr++) {
        for (int fc = 0; fc < 8; fc++) {
            char piece = board[fr][fc];
            if (piece == '.') continue;
            bool isOwnPiece = (turn == 1) ? isupper(piece) : islower(piece);
            if (!isOwnPiece) continue;

            for (int tr = 0; tr < 8; tr++) {
                for (int tc = 0; tc < 8; tc++) {
                    if (fr == tr && fc == tc) continue;

                    char target = board[tr][tc];
                    if (turn == 1 && isupper(target)) continue;
                    if (turn == 2 && islower(target)) continue;

                    if (!isValidMove(piece, fr, fc, tr, tc)) continue;

                    char savedFrom = board[fr][fc];
                    char savedTo   = board[tr][tc];
                    int savedEPRow = enPassantRow;
                    int savedEPCol = enPassantCol;
                    int epCapRow   = -1;

                    if ((piece == 'P' || piece == 'p') &&
                        abs(tc - fc) == 1 && board[tr][tc] == '.') {
                        epCapRow = fr;
                        board[fr][tc] = '.';
                    }

                    board[tr][tc] = savedFrom;
                    board[fr][fc] = '.';

                    bool inCheck = isInCheck(turn);

                    board[fr][fc] = savedFrom;
                    board[tr][tc] = savedTo;
                    enPassantRow  = savedEPRow;
                    enPassantCol  = savedEPCol;
                    if (epCapRow != -1)
                        board[epCapRow][tc] = (piece == 'P') ? 'p' : 'P';

                    if (!inCheck) return true;
                }
            }

            if ((turn == 1 && piece == 'K') || (turn == 2 && piece == 'k')) {
                if (canCastle(turn, true))  return true;
                if (canCastle(turn, false)) return true;
            }
        }
    }
    return false;
}

bool chessboard::isInsufficientMaterial() {
    std::vector<char> whitePieces, blackPieces;
    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            char p = board[r][c];
            if (p == '.') continue;
            if (isupper(p) && p != 'K') whitePieces.push_back(p);
            if (islower(p) && p != 'k') blackPieces.push_back(p);
        }
    }

    auto hasMatingPiece = [](const std::vector<char>& pieces) {
        for (char p : pieces)
            if (toupper(p) == 'Q' || toupper(p) == 'R' || toupper(p) == 'P')
                return true;
        return false;
    };

    if (hasMatingPiece(whitePieces) || hasMatingPiece(blackPieces)) return false;
    if (whitePieces.empty() && blackPieces.empty()) return true;
    if ((whitePieces.size() == 1 && blackPieces.empty()) ||
        (whitePieces.empty() && blackPieces.size() == 1)) return true;
    if (whitePieces.size() == 1 && blackPieces.size() == 1 &&
        toupper(whitePieces[0]) == 'B' && toupper(blackPieces[0]) == 'B') return true;

    return false;
}

// ___Public methods___
bool chessboard::needsPromotion() {
    return pendingPromotion;
}

void chessboard::promotePawn(char piece) {
    char current = board[promotionRow][promotionCol];

    board[promotionRow][promotionCol] =
        isupper(current)
            ? toupper(piece)
            : tolower(piece);

    pendingPromotion = false;
    promotionRow = -1;
    promotionCol = -1;
}
char chessboard::getBoard(int row, int col) {
    return board[row][col];
}
//for gui
std::vector<std::pair<int,int>> chessboard::getLegalMovesFor(int row, int col) {
    std::vector<std::pair<int,int>> moves;
    char piece = board[row][col];
    if (piece == '.') return moves;

    int turn = getTurn();
    bool isOwnPiece = (turn == 1) ? isupper(piece) : islower(piece);
    if (!isOwnPiece) return moves;

    for (int tr = 0; tr < 8; tr++) {
        for (int tc = 0; tc < 8; tc++) {
            if (tr == row && tc == col) continue;
            char target = board[tr][tc];
            if (turn == 1 && isupper(target)) continue;
            if (turn == 2 && islower(target)) continue;
            if (!isValidMove(piece, row, col, tr, tc)) continue;

            // need to test it, so that it doesn't leave king in check
            char savedFrom = board[row][col];
            char savedTo   = board[tr][tc];
            int  savedEPRow = enPassantRow;
            int  savedEPCol = enPassantCol;
            int  epCapRow   = -1;

            if ((piece == 'P' || piece == 'p') &&
                abs(tc - col) == 1 && board[tr][tc] == '.') {
                epCapRow = row;
                board[row][tc] = '.';
            }

            board[tr][tc]  = savedFrom;
            board[row][col] = '.';

            bool inCheck = isInCheck(turn);

            board[row][col] = savedFrom;
            board[tr][tc]   = savedTo;
            enPassantRow    = savedEPRow;
            enPassantCol    = savedEPCol;
            if (epCapRow != -1)
                board[epCapRow][tc] = (piece == 'P') ? 'p' : 'P';

            if (!inCheck) moves.push_back({tr, tc});
        }
    }
    return moves;
}

std::vector<std::string> chessboard::getMoveHistory() {
    return moveHistory;
}

void chessboard::printBoard() {
    for (int i = 0; i < 8; i++) {
        std::cout << (8 - i) << "  ";
        for (int j = 0; j < 8; j++)
            std::cout << board[i][j] << " ";
        std::cout << "\n";
    }
    std::cout << "\n   a b c d e f g h\n\n";
}

int chessboard::getTurn() {
    return (moveCounter % 2 == 0) ? 1 : 2;
}

bool chessboard::isInCheck(int turn) {
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

bool chessboard::checkGameOver(std::string& outResult) {
    int currentTurn = getTurn();

    if (!hasLegalMove(currentTurn)) {
        if (isInCheck(currentTurn)) {
            outResult = (currentTurn == 1)
                ? "Checkmate! Black wins!"
                : "Checkmate! White wins!";
        } else {
            outResult = "Stalemate! It's a draw.";
        }
        return true;
    }

    if (halfMoveClock >= 100) {
        outResult = "Draw by the fifty-move rule!";
        return true;
    }

    std::string currentHash = getBoardHash();
    int repetitions = 0;
    for (const std::string& h : positionHistory)
        if (h == currentHash) repetitions++;
    if (repetitions >= 3) {
        outResult = "Draw by threefold repetition!";
        return true;
    }

    if (isInsufficientMaterial()) {
        outResult = "Draw by insufficient material!";
        return true;
    }

    return false;
}

void chessboard::makeMove(std::string from, std::string to) {
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

    if (savedFrom == 'K') whiteKingMoved  = true;
    if (savedFrom == 'k') blackKingMoved  = true;
    if (fromnum == 7 && fromletter == 0) whiteRookAMoved = true;
    if (fromnum == 7 && fromletter == 7) whiteRookHMoved = true;
    if (fromnum == 0 && fromletter == 0) blackRookAMoved = true;
    if (fromnum == 0 && fromletter == 7) blackRookHMoved = true;
    if (tonum == 7 && toletter == 0) whiteRookAMoved = true;
    if (tonum == 7 && toletter == 7) whiteRookHMoved = true;
    if (tonum == 0 && toletter == 0) blackRookAMoved = true;
    if (tonum == 0 && toletter == 7) blackRookHMoved = true;

    bool isCapture  = (savedTo != '.') || (epCaptureRow != -1);
    bool isPawnMove = (savedFrom == 'P' || savedFrom == 'p');
    if (isCapture || isPawnMove)
        halfMoveClock = 0;
    else
        halfMoveClock++;

    moveCounter++;
    moveHistory.push_back(from + "-" + to);
    positionHistory.push_back(getBoardHash());

    // Pawn promotion now handeled in main loop
    if ((board[tonum][toletter] == 'P' && tonum == 0) ||
    (board[tonum][toletter] == 'p' && tonum == 7))
    {
    pendingPromotion = true;
    promotionRow = tonum;
    promotionCol = toletter;
    }
}

void chessboard::performCastle(int turn, bool kingSide) {
    if (!canCastle(turn, kingSide))
        throw ChessException("Castling is not available.");

    int row = (turn == 1) ? WHITE_KING_ROW : BLACK_KING_ROW;

    if (kingSide) {
        board[row][KINGSIDE_KING_DST]  = board[row][KING_COL];
        board[row][KING_COL]           = '.';
        board[row][KINGSIDE_ROOK_DST]  = board[row][KINGSIDE_ROOK_COL];
        board[row][KINGSIDE_ROOK_COL]  = '.';
        moveHistory.push_back("O-O");
    } else {
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

    
}

void chessboard::printMoveHistory() {
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

std::string chessboard::toFEN() {
    std::ostringstream fen;

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

    fen << ' ' << (getTurn() == 1 ? 'w' : 'b');

    std::string castling = "";
    if (!whiteKingMoved && !whiteRookHMoved) castling += 'K';
    if (!whiteKingMoved && !whiteRookAMoved) castling += 'Q';
    if (!blackKingMoved && !blackRookHMoved) castling += 'k';
    if (!blackKingMoved && !blackRookAMoved) castling += 'q';
    fen << ' ' << (castling.empty() ? "-" : castling);

    if (enPassantRow == -1) {
        fen << " -";
    } else {
        char file = 'a' + enPassantCol;
        int  rank = 8 - enPassantRow;
        fen << ' ' << file << rank;
    }

    fen << ' ' << halfMoveClock;
    fen << ' ' << (moveCounter / 2 + 1);

    return fen.str();
}