#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <map>
#include <stdexcept>

extern std::map<char, int>   fileMap;
extern std::map<char, float> pieceValues;

// Exceptions
struct ChessException : public std::runtime_error {
    explicit ChessException(const std::string& msg) : std::runtime_error(msg) {}
};
struct InvalidFormatException    : public ChessException { using ChessException::ChessException; };
struct SameSquareException       : public ChessException { using ChessException::ChessException; };
struct EmptySquareException      : public ChessException { using ChessException::ChessException; };
struct WrongTurnException        : public ChessException { using ChessException::ChessException; };
struct FriendlyFireException     : public ChessException { using ChessException::ChessException; };
struct InvalidPieceMoveException : public ChessException { using ChessException::ChessException; };
struct MovesIntoCheckException   : public ChessException { using ChessException::ChessException; };

class chessboard {
private:
    char board[8][8];
    int  moveCounter  = 0;
    int  enPassantRow = -1;
    int  enPassantCol = -1;
    bool whiteKingMoved  = false;
    bool blackKingMoved  = false;
    bool whiteRookAMoved = false;
    bool whiteRookHMoved = false;
    bool blackRookAMoved = false;
    bool blackRookHMoved = false;
    int  halfMoveClock   = 0;
    std::vector<std::string> positionHistory;
    std::vector<std::string> moveHistory;

    static const int WHITE_KING_ROW     = 7;
    static const int BLACK_KING_ROW     = 0;
    static const int KING_COL           = 4;
    static const int KINGSIDE_ROOK_COL  = 7;
    static const int QUEENSIDE_ROOK_COL = 0;
    static const int KINGSIDE_KING_DST  = 6;
    static const int QUEENSIDE_KING_DST = 2;
    static const int KINGSIDE_ROOK_DST  = 5;
    static const int QUEENSIDE_ROOK_DST = 3;

   
    bool isPathClearRook(int fr, int fc, int tr, int tc);
    bool isPathClearBishop(int fr, int fc, int tr, int tc);
    bool isValidKnightMove(int fr, int fc, int tr, int tc);
    bool isValidBishopMove(int fr, int fc, int tr, int tc);
    bool isValidRookMove(int fr, int fc, int tr, int tc);
    bool isValidQueenMove(int fr, int fc, int tr, int tc);
    bool isValidKingMove(int fr, int fc, int tr, int tc);
    bool isValidPawnMove(char piece, int fr, int fc, int tr, int tc);
    bool isPawnAttacking(char piece, int fr, int fc, int tr, int tc);
    bool isValidMove(char piece, int fr, int fc, int tr, int tc);
    bool isAttacking(char piece, int fr, int fc, int tr, int tc);
    bool canCastle(int turn, bool kingSide);
    bool isSquareAttackedBy(int row, int col, int attackerTurn);
    std::string getBoardHash();
    bool hasLegalMove(int turn);
    bool isInsufficientMaterial();

public:
    chessboard();
    void        printBoard();
    int         getTurn();
    bool        isInCheck(int turn);
    bool        checkGameOver(std::string& outResult);
    void        makeMove(std::string from, std::string to);
    void        performCastle(int turn, bool kingSide);
    void        printMoveHistory();
    std::string toFEN();
};