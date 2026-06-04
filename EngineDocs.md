# Engine Documentation

This document explains how the chess engine (`chessboard.cpp` / `chessboard.h`) works in full detail — covering every system, every design decision, and how to build one yourself from scratch.

---

## Table of Contents

1. [Overview](#overview)
2. [Build Roadmap](#build-roadmap)
3. [Board Representation](#board-representation)
4. [State Variables](#state-variables)
5. [Turn System](#turn-system)
6. [Move Validation](#move-validation)
7. [Attack Detection](#attack-detection)
8. [Check Detection](#check-detection)
9. [Legal Move Generation](#legal-move-generation)
10. [Making a Move](#making-a-move)
11. [Special Moves](#special-moves)
    - [En Passant](#en-passant)
    - [Castling](#castling)
    - [Pawn Promotion](#pawn-promotion)
12. [Game Over Conditions](#game-over-conditions)
    - [Checkmate & Stalemate](#checkmate--stalemate)
    - [Fifty-Move Rule](#fifty-move-rule)
    - [Threefold Repetition](#threefold-repetition)
    - [Insufficient Material](#insufficient-material)
13. [FEN Export](#fen-export)
14. [Exception Hierarchy](#exception-hierarchy)

---

## Overview

The engine is a self-contained class (`chessboard`) that owns all game state and enforces all rules. It has no knowledge of rendering or input, those are the GUI's responsibility. Every interaction goes through the public API defined in `chessboard.h`.

The engine is **not** an AI. It generates and validates legal moves but does not choose them. It is a pure rules engine its more of referee rather than a player.

The clean split between engine and GUI means you could swap the SFML frontend for a terminal UI, a network interface, or an AI driver without touching a single line of chess logic.

---

## Build Roadmap

If you want to build this engine yourself, here is the order that makes sense and the reasoning behind each step. Each stage builds on the previous one
The idea is simple:
1) Make it work
2) Make it forward compatable
3) Make it pretty
---

### Stage 1: Board Representation

**Build:** A `char board[8][8]` with a hardcoded starting position. Write a `printBoard()` that dumps it to the terminal, ypu could consider Bitboards or even a 64 index array,
           bitboards especially are significantly faster as it allows you to use bitwise operators to make moves, optimising speed, howver the complexity of implemetation
           increases with it, so depending on your scale you could go with that, however for me, i went with the simpler methord as its a simpler project.

**Why this first:** Everything else — movement rules, check detection, legal moves — operates on this array. Get it right and visible before writing any logic.

**Decision — why `char`:** You could use an enum, a struct, or integers. `char` is the simplest option that lets you distinguish colour (upper/lowercase), identify piece type with a `switch`, and print the board with no conversion. It is also directly compatible with FEN notation. The tradeoff is that it is less type-safe than an enum, but for a project this size that is not a problem.

**Decision — why row 0 = rank 8:** Arrays index from 0, and it is natural to initialise the array top-to-bottom with black at the top. The alternative (row 0 = rank 1, white at top) would require flipping every visual representation. Accepting the coordinate inversion here keeps the array initialisation readable.

---

### Stage 2: Piece Movement Rules

**Build:** One validation function per piece type. A dispatcher `isValidMove()` that routes by piece character.

**Why this order:** These functions are pure geometry — they don't need board state beyond the current position of pieces along a path. Write and test them in isolation before connecting them to game logic.

**Decision — separate functions per piece:** A single large `switch` or a monolithic `isValidMove` would work, but separate functions (`isValidKnightMove`, `isValidBishopMove`, etc.) are easier to test, debug, and extend. The queen being defined as "rook + bishop" is also much cleaner this way:

```cpp
bool chessboard::isValidQueenMove(int fr, int fc, int tr, int tc) {
    return isValidRookMove(fr, fc, tr, tc) ||
           isValidBishopMove(fr, fc, tr, tc);
}
```

**Decision  path-clearing as separate helpers:** Rooks and bishops share the same stepping logic. Extracting `isPathClearRook` and `isPathClearBishop` means the path check is written once and reused by both the movement validators and (indirectly) by the queen.

```cpp
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
```

The knight is the only piece that does not need a path check — it jumps. Its validator is just an L-shape check:

```cpp
bool chessboard::isValidKnightMove(int fr, int fc, int tr, int tc) {
    int rowDiff = abs(tr - fr);
    int colDiff = abs(tc - fc);
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}
```

**The pawn is the hardest piece.** It moves differently depending on direction, starting rank, whether it is capturing, and whether en passant is available. Write it last among the pieces and handle all three cases explicitly:

```cpp
bool chessboard::isValidPawnMove(char piece, int fromRow, int fromCol, int toRow, int toCol) {
    int direction = (piece == 'P') ? -1 : 1;  // white moves up (negative row), black moves down

    // Case 1: single step forward onto empty square
    if (fromCol == toCol) {
        if (toRow - fromRow == direction && board[toRow][toCol] == '.') return true;

        // Case 2: double push from starting rank
        if ((fromRow == 6 && piece == 'P') || (fromRow == 1 && piece == 'p')) {
            if (toRow - fromRow == 2 * direction &&
                board[fromRow + direction][toCol] == '.' &&
                board[toRow][toCol] == '.') return true;
        }
    }
    // Case 3: diagonal capture (normal or en passant)
    else if (abs(toCol - fromCol) == 1 && toRow - fromRow == direction) {
        if (board[toRow][toCol] != '.' &&
            ((piece == 'P' && islower(board[toRow][toCol])) ||
             (piece == 'p' && isupper(board[toRow][toCol])))) return true;
        if (toRow == enPassantRow && toCol == enPassantCol) return true;
    }
    return false;
}
```

---

### Stage 3: Attack Detection

**Build:** `isAttacking()` and `isPawnAttacking()`. Keep them separate from `isValidMove()`.

**Why:** A pawn moves forward but **attacks** diagonally. If you use `isValidMove` for check detection, a pawn standing in front of your king would incorrectly "check" it by moving forward. Attack detection must use diagonal-only logic for pawns:

```cpp
bool chessboard::isPawnAttacking(char piece, int fr, int fc, int tr, int tc) {
    int direction = (piece == 'P') ? -1 : 1;
    return (tr - fr == direction) && (abs(tc - fc) == 1);
}
```

For every other piece, attacking and moving are the same, so `isAttacking` delegates back to the movement validators.

```cpp
bool chessboard::isAttacking(char piece, int fr, int fc, int tr, int tc) {
    switch (piece) {
        case 'P': case 'p': return isPawnAttacking(piece, fr, fc, tr, tc);
        case 'N': case 'n': return isValidKnightMove(fr, fc, tr, tc);
        case 'B': case 'b': return isValidBishopMove(fr, fc, tr, tc);
        case 'R': case 'r': return isValidRookMove(fr, fc, tr, tc);
        case 'Q': case 'q': return isValidQueenMove(fr, fc, tr, tc);
        case 'K': case 'k': return isValidKingMove(fr, fc, tr, tc);
        default: return false;
    }
}
```

---

### Stage 4: Check Detection

**Build:** `isInCheck(int turn)` and `isSquareAttackedBy(int row, int col, int attackerTurn)`.

**Why now:** Check detection is needed by nearly everything that follows, legal move generation, `makeMove`, castling legality. Build it before any of those.

**How `isInCheck` works:** Scan the board for the king, then scan every enemy piece and ask whether it attacks the king's square:

```cpp
bool chessboard::isInCheck(int turn) {
    char kingChar = (turn == 1) ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;

    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == kingChar) { kingRow = r; kingCol = c; }

    if (kingRow == -1) return false;  // safety guard

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
```

**Why not optimise with ray-casting yet:** A brute-force scan of all 64 squares is fast enough for a two-player game with no AI. Ray-casting (checking lines outward from the king) would be faster but significantly more complex to implement and debug. The simple approach is correct and plenty fast.I mean you can do it but for this scale its implementation is annoying, compared to brute force is minimal

---

### Stage 5: Turn System and Basic `makeMove`

**Build:** `getTurn()`, the `fileMap` for algebraic-to-index conversion, input validation, and a basic `makeMove` that moves pieces without checking legality beyond piece movement rules.

```cpp
int chessboard::getTurn() {
    return (moveCounter % 2 == 0) ? 1 : 2;
}
```

**Why `moveCounter % 2` instead of a `bool currentTurn`:** A counter gives you move count for free (needed for the fifty-move rule and FEN), and turn is a derived value. One fewer variable to keep in sync.

**Input validation order matters.** Validate cheaply first, expensively last. Format and same-square checks cost nothing. `isValidMove` and `isInCheck` are more expensive, only run them if the basic checks pass:

```cpp
void chessboard::makeMove(std::string from, std::string to) {
    // 1. Castling shortcut — detect before regex so "O-O" is never parsed as a square
    if (from == "O-O" || from == "o-o") { performCastle(getTurn(), true); return; }
    if (from == "O-O-O" || from == "o-o-o") { performCastle(getTurn(), false); return; }

    // 2. Format
    const std::regex movePattern("[a-h][1-8]");
    if (!std::regex_match(from, movePattern) || !std::regex_match(to, movePattern))
        throw InvalidFormatException("Invalid format: use a-h for file and 1-8 for rank.");

    // 3. Same square
    if (from == to) throw SameSquareException("Source and destination are the same.");

    // 4. Convert algebraic to array indices
    int fromletter = fileMap[from[0]];
    int toletter   = fileMap[to[0]];
    int fromnum    = 8 - (from[1] - '0');
    int tonum      = 8 - (to[1] - '0');

    // 5. Piece existence and ownership
    if (board[fromnum][fromletter] == '.') throw EmptySquareException(...);
    // ... wrong turn, friendly fire checks ...

    // 6. Piece movement geometry
    if (!isValidMove(board[fromnum][fromletter], fromnum, fromletter, tonum, toletter))
        throw InvalidPieceMoveException(...);

    // 7. Simulate, check for self-check, commit or rollback
    // ...
}
```

At this stage, get basic pawn/knight/bishop/rook/queen/king moves working and printing to the terminal. Don't worry about check, en passant, or castling yet.

---

### Stage 6: Self-Check Filtering in `makeMove`

**Build:** The simulate-and-rollback block inside `makeMove`.

**Why this is its own stage:** This is the most error-prone part of the engine. The idea is: apply the move to the board temporarily, call `isInCheck`, then undo it if it leaves the king in check. Every piece of state you modify during the simulation must be saved and restored:

```cpp
char savedFrom  = board[fromnum][fromletter];
char savedTo    = board[tonum][toletter];
int  savedEPRow = enPassantRow;
int  savedEPCol = enPassantCol;

board[tonum][toletter]     = savedFrom;
board[fromnum][fromletter] = '.';

if (isInCheck(getTurn())) {
    // Rollback
    board[fromnum][fromletter] = savedFrom;
    board[tonum][toletter]     = savedTo;
    enPassantRow = savedEPRow;
    enPassantCol = savedEPCol;
    throw MovesIntoCheckException("Illegal move: your king would be in check.");
}

// Commit — update flags, increment counter, record history
```

 Dont forget to restore en passant state on rollback. If a move is rejected and `enPassantRow/Col` were reset to -1 at the start of `makeMove`, a valid en passant on the next move will be incorrectly blocked.

---

### Stage 7: En Passant

**Build:** Set `enPassantRow/Col` after double pawn pushes. Handle the en passant capture in both `makeMove` and `getLegalMovesFor`.

 The captured pawn is not on the destination square, it is on the same row as the capturing pawn, one column over. You must remove it separately during the move, and restore it during any rollback:

```cpp
// After a double push, record the skipped square
if (savedFrom == 'P' || savedFrom == 'p') {
    int direction = (savedFrom == 'P') ? -1 : 1;
    if (tonum - fromnum == 2 * direction) {
        enPassantRow = fromnum + direction;
        enPassantCol = fromletter;
    }
    // En passant capture: destination is empty, but a pawn sits beside us
    if (abs(toletter - fromletter) == 1 && board[tonum][toletter] == '.') {
        epCaptureRow = fromnum;
        board[fromnum][toletter] = '.';  // remove the captured pawn
    }
}
```

The en passant window is exactly one move. At the start of every `makeMove` call, `enPassantRow` and `enPassantCol` are reset to `-1` before being potentially re-set by a new double push.

---

### Stage 8: Legal Move Generation (`getLegalMovesFor`)



**This is separate from `makeMove`:** The GUI needs to show legal move dots before the player clicks a destination. `makeMove` validates and executes; `getLegalMovesFor` just queries. They share the same simulate-and-check logic:

```cpp
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
            if (turn == 1 && isupper(target)) continue;  // can't capture own piece
            if (turn == 2 && islower(target)) continue;
            if (!isValidMove(piece, row, col, tr, tc)) continue;

            // Simulate
            char savedFrom = board[row][col];
            char savedTo   = board[tr][tc];
            int  savedEPRow = enPassantRow;
            int  savedEPCol = enPassantCol;
            int  epCapRow   = -1;

            if ((piece == 'P' || piece == 'p') && abs(tc - col) == 1 && board[tr][tc] == '.') {
                epCapRow = row;
                board[row][tc] = '.';  // temporarily remove en passant victim
            }

            board[tr][tc]  = savedFrom;
            board[row][col] = '.';

            bool inCheck = isInCheck(turn);

            // Undo
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
```

**Note: castling is not returned here.** The GUI injects castling destinations `{kingRow, 6}` and `{kingRow, 2}` after calling this function when the selected piece is a king on its home square. The engine validates castling when `makeMove("O-O", "")` is called.

---

### Stage 9: Castling

**Build:** `canCastle()` and `performCastle()`. Add moved-flag tracking to `makeMove`.

Castling has the most conditions of any chess rule. `canCastle` checks all of them:

```cpp
bool chessboard::canCastle(int turn, bool kingSide) {
    if (turn == 1) {
        if (whiteKingMoved) return false;              // king has moved
        if (kingSide) {
            if (whiteRookHMoved) return false;         // h1 rook has moved
            if (board[WHITE_KING_ROW][KINGSIDE_ROOK_DST] != '.' ||
                board[WHITE_KING_ROW][KINGSIDE_KING_DST] != '.') return false;  // path blocked
            if (isInCheck(1)) return false;            // currently in check
            if (isSquareAttackedBy(WHITE_KING_ROW, KINGSIDE_ROOK_DST, 2)) return false;  // f1 attacked
            if (isSquareAttackedBy(WHITE_KING_ROW, KINGSIDE_KING_DST, 2)) return false;  // g1 attacked
        }
        // queenside mirrors kingside...
    }
    return true;
}
```

**Why check transit squares, not the rook's path:** FIDE rules only prohibit the king from moving through or into check. The rook may pass through attacked squares. So only the king's path (`e→f→g` kingside, `e→d→c` queenside) is checked for attacks.

**Why the rook-moved flags are also set on capture:** If an opponent captures your rook on a1 or h1, you lose castling rights on that side even though your king and rook never moved. `makeMove` handles this:

```cpp
// Set flags for the destination square too — covers the case where the rook is captured
if (tonum == 7 && toletter == 0) whiteRookAMoved = true;
if (tonum == 7 && toletter == 7) whiteRookHMoved = true;
if (tonum == 0 && toletter == 0) blackRookAMoved = true;
if (tonum == 0 && toletter == 7) blackRookHMoved = true;
```

`performCastle` moves both pieces atomically using the named constants:

```cpp
void chessboard::performCastle(int turn, bool kingSide) {
    if (!canCastle(turn, kingSide)) throw ChessException("Castling is not available.");

    int row = (turn == 1) ? WHITE_KING_ROW : BLACK_KING_ROW;

    if (kingSide) {
        board[row][KINGSIDE_KING_DST]  = board[row][KING_COL];   // king e→g
        board[row][KING_COL]           = '.';
        board[row][KINGSIDE_ROOK_DST]  = board[row][KINGSIDE_ROOK_COL]; // rook h→f
        board[row][KINGSIDE_ROOK_COL]  = '.';
        moveHistory.push_back("O-O");
    } else {
        board[row][QUEENSIDE_KING_DST] = board[row][KING_COL];   // king e→c
        board[row][KING_COL]           = '.';
        board[row][QUEENSIDE_ROOK_DST] = board[row][QUEENSIDE_ROOK_COL]; // rook a→d
        board[row][QUEENSIDE_ROOK_COL] = '.';
        moveHistory.push_back("O-O-O");
    }

    // Mark both pieces as moved
    if (turn == 1) { whiteKingMoved = true; /* + rook flag */ }
    else           { blackKingMoved = true; /* + rook flag */ }

    halfMoveClock++;
    moveCounter++;
    positionHistory.push_back(getBoardHash());
}
```

---

### Stage 10: Pawn Promotion

**Build:** Promotion detection in `makeMove`, the `pendingPromotion` flag, and `promotePawn()`.

**Why a pending flag instead of auto-queening:** Auto-queening is simpler but wrong — players should be able to underpromote (promote to rook, bishop, or knight), which is a legal and sometimes strategically important move. A flag pauses the game loop until the GUI supplies a choice.

Detection at the end of `makeMove`:

```cpp
if ((board[tonum][toletter] == 'P' && tonum == 0) ||
    (board[tonum][toletter] == 'p' && tonum == 7)) {
    pendingPromotion = true;
    promotionRow = tonum;
    promotionCol = toletter;
}
```

Committing the promotion when the GUI calls `promotePawn('Q')` etc.:

```cpp
void chessboard::promotePawn(char piece) {
    char current = board[promotionRow][promotionCol];
    // Preserve colour: if the pawn was white (uppercase), promote to uppercase
    board[promotionRow][promotionCol] = isupper(current) ? toupper(piece) : tolower(piece);
    pendingPromotion = false;
    promotionRow = promotionCol = -1;
}
```

---

### Stage 11: Game Over Detection

**Build:** `hasLegalMove()`, `checkGameOver()`, `isInsufficientMaterial()`, and `getBoardHash()`.

Build these last because they depend on everything above.

**`hasLegalMove`** is essentially `getLegalMovesFor` run over all pieces of a given side. If any piece has any legal move, it returns `true` immediately (short-circuits). Castling is also tested:

```cpp
if ((turn == 1 && piece == 'K') || (turn == 2 && piece == 'k')) {
    if (canCastle(turn, true))  return true;
    if (canCastle(turn, false)) return true;
}
```

**`checkGameOver`** orchestrates all four terminal conditions in order:

```cpp
bool chessboard::checkGameOver(std::string& outResult) {
    int currentTurn = getTurn();

    // 1. No legal moves — checkmate or stalemate
    if (!hasLegalMove(currentTurn)) {
        outResult = isInCheck(currentTurn)
            ? (currentTurn == 1 ? "Checkmate! Black wins!" : "Checkmate! White wins!")
            : "Stalemate! It's a draw.";
        return true;
    }

    // 2. Fifty-move rule
    if (halfMoveClock >= 100) { outResult = "Draw by the fifty-move rule!"; return true; }

    // 3. Threefold repetition
    std::string currentHash = getBoardHash();
    int repetitions = 0;
    for (const std::string& h : positionHistory)
        if (h == currentHash) repetitions++;
    if (repetitions >= 3) { outResult = "Draw by threefold repetition!"; return true; }

    // 4. Insufficient material
    if (isInsufficientMaterial()) { outResult = "Draw by insufficient material!"; return true; }

    return false;
}
```

**`getBoardHash`** encodes everything that defines a chess position — piece placement, en passant availability, and castling rights. Without the last two fields, positions that look identical on the board but differ in available options would incorrectly count as repetitions:

```cpp
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
```

**`isInsufficientMaterial`** collects all non-king pieces for each side, then tests a set of known drawing configurations:

```cpp
bool chessboard::isInsufficientMaterial() {
    std::vector<char> whitePieces, blackPieces;
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            char p = board[r][c];
            if (isupper(p) && p != 'K') whitePieces.push_back(p);
            if (islower(p) && p != 'k') blackPieces.push_back(p);
        }

    // Any queen, rook, or pawn = mating potential
    auto hasMatingPiece = [](const std::vector<char>& pieces) {
        for (char p : pieces)
            if (toupper(p) == 'Q' || toupper(p) == 'R' || toupper(p) == 'P') return true;
        return false;
    };

    if (hasMatingPiece(whitePieces) || hasMatingPiece(blackPieces)) return false;

    if (whitePieces.empty() && blackPieces.empty()) return true;             // K vs K
    if (whitePieces.size() == 1 && blackPieces.empty()) return true;        // K+minor vs K
    if (whitePieces.empty() && blackPieces.size() == 1) return true;        // K vs K+minor
    if (whitePieces.size() == 1 && blackPieces.size() == 1 &&
        toupper(whitePieces[0]) == 'B' && toupper(blackPieces[0]) == 'B')   // KBvKB
        return true;

    return false;
}
```

Note: KNNvK is not included — two knights can technically force checkmate with incorrect play from the defending side, so it is not a drawn position by rule.

---

### Stage 12 — FEN Export (optional but useful)

**Build:** `toFEN()`. This is a nice-to-have that lets you export positions to tools like Lichess's board editor for debugging.

```cpp
std::string chessboard::toFEN() {
    std::ostringstream fen;

    // Piece placement — run-length encode empty squares
    for (int r = 0; r < 8; r++) {
        int empty = 0;
        for (int c = 0; c < 8; c++) {
            if (board[r][c] == '.') { empty++; }
            else {
                if (empty > 0) { fen << empty; empty = 0; }
                fen << board[r][c];
            }
        }
        if (empty > 0) fen << empty;
        if (r < 7) fen << '/';
    }

    fen << ' ' << (getTurn() == 1 ? 'w' : 'b');

    // Castling availability
    std::string castling = "";
    if (!whiteKingMoved && !whiteRookHMoved) castling += 'K';
    if (!whiteKingMoved && !whiteRookAMoved) castling += 'Q';
    if (!blackKingMoved && !blackRookHMoved) castling += 'k';
    if (!blackKingMoved && !blackRookAMoved) castling += 'q';
    fen << ' ' << (castling.empty() ? "-" : castling);

    // En passant target square
    if (enPassantRow == -1) { fen << " -"; }
    else { fen << ' ' << (char)('a' + enPassantCol) << (8 - enPassantRow); }

    fen << ' ' << halfMoveClock;
    fen << ' ' << (moveCounter / 2 + 1);

    return fen.str();
}
```

---

## Board Representation

The board is a plain `char[8][8]`:

```cpp
char board[8][8];
```

| Character | Meaning |
|-----------|---------|
| `'.'` | Empty square |
| `'P'` | White pawn |
| `'N'` | White knight |
| `'B'` | White bishop |
| `'R'` | White rook |
| `'Q'` | White queen |
| `'K'` | White king |
| `'p'` `'n'` `'b'` `'r'` `'q'` `'k'` | Same pieces, black |

**Uppercase = white, lowercase = black.** `isupper(piece)` is white, `islower(piece)` is black. This is used everywhere in the engine for ownership checks.

### Coordinate System

```
row 0 = rank 8  (black's back rank)
row 7 = rank 1  (white's back rank)
col 0 = a-file
col 7 = h-file
```

So `board[0][4]` is `e8` and `board[7][4]` is `e1`.

The GUI converts between screen coordinates and this system:

```cpp
from += (char)('a' + selectedCol);   // col 0→'a', col 7→'h'
from += (char)('8' - selectedRow);   // row 0→'8', row 7→'1'
```

### Initial Position

```cpp
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
```

The starting position hash is pushed immediately so threefold repetition can detect if the game returns to the starting position.

---

## State Variables

These private members track everything that cannot be inferred from the board array alone:

| Variable | Type | Purpose |
|----------|------|---------|
| `moveCounter` | `int` | Total half-moves played. Drives turn calculation and FEN full-move number. |
| `enPassantRow` / `enPassantCol` | `int` | The square an en passant capture may land on. Reset to `-1` every move. |
| `whiteKingMoved` | `bool` | White's king has moved — castling permanently forbidden. |
| `blackKingMoved` | `bool` | Same for black. |
| `whiteRookAMoved` | `bool` | White's a-file rook has moved or been captured. |
| `whiteRookHMoved` | `bool` | White's h-file rook has moved or been captured. |
| `blackRookAMoved` | `bool` | Black's a-file rook has moved or been captured. |
| `blackRookHMoved` | `bool` | Black's h-file rook has moved or been captured. |
| `halfMoveClock` | `int` | Half-moves since last pawn move or capture. Fifty-move rule triggers at 100. |
| `pendingPromotion` | `bool` | A pawn reached the back rank. Game is paused until `promotePawn()` is called. |
| `promotionRow` / `promotionCol` | `int` | Location of the pawn awaiting promotion. |
| `positionHistory` | `vector<string>` | All board hashes since game start. Used for threefold repetition. |
| `moveHistory` | `vector<string>` | Human-readable move list displayed in the UI panel. |

---

## Turn System

```cpp
int chessboard::getTurn() {
    return (moveCounter % 2 == 0) ? 1 : 2;  // 1 = white, 2 = black
}
```

Turn is derived from `moveCounter`, not stored separately. This avoids any possibility of the two going out of sync.

---

## Move Validation

`isValidMove` dispatches to a per-piece function:

```cpp
bool chessboard::isValidMove(char piece, int fr, int fc, int tr, int tc) {
    switch (piece) {
        case 'P': case 'p': return isValidPawnMove(piece, fr, fc, tr, tc);
        case 'N': case 'n': return isValidKnightMove(fr, fc, tr, tc);
        case 'B': case 'b': return isValidBishopMove(fr, fc, tr, tc);
        case 'R': case 'r': return isValidRookMove(fr, fc, tr, tc);
        case 'Q': case 'q': return isValidQueenMove(fr, fc, tr, tc);
        case 'K': case 'k': return isValidKingMove(fr, fc, tr, tc);
        default: return false;
    }
}
```

These functions only check geometry, they do not check whether the move leaves the king in check. That is handled separately.

### Knight

```cpp
bool chessboard::isValidKnightMove(int fr, int fc, int tr, int tc) {
    int rowDiff = abs(tr - fr);
    int colDiff = abs(tc - fc);
    return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);
}
```



### Bishop

```cpp
bool chessboard::isValidBishopMove(int fr, int fc, int tr, int tc) {
    if (abs(tr - fr) != abs(tc - fc)) return false;  // must be diagonal
    return isPathClearBishop(fr, fc, tr, tc);
}
```

### Rook

```cpp
bool chessboard::isValidRookMove(int fr, int fc, int tr, int tc) {
    if (fr != tr && fc != tc) return false;  // must be same rank or file
    return isPathClearRook(fr, fc, tr, tc);
}
```

### Queen

```cpp
bool chessboard::isValidQueenMove(int fr, int fc, int tr, int tc) {
    return isValidRookMove(fr, fc, tr, tc) || isValidBishopMove(fr, fc, tr, tc);
}
```

### King

```cpp
bool chessboard::isValidKingMove(int fr, int fc, int tr, int tc) {
    return abs(tr - fr) <= 1 && abs(tc - fc) <= 1;
}
```

This does not validate castling — that is handled by `canCastle`/`performCastle`.

### Pawn

```cpp
bool chessboard::isValidPawnMove(char piece, int fr, int fc, int tr, int tc) {
    int direction = (piece == 'P') ? -1 : 1;  // white moves up (negative), black down

    if (fc == tc) {
        // Single step
        if (tr - fr == direction && board[tr][tc] == '.') return true;
        // Double push from starting rank
        if ((fr == 6 && piece == 'P') || (fr == 1 && piece == 'p'))
            if (tr - fr == 2 * direction &&
                board[fr + direction][tc] == '.' &&
                board[tr][tc] == '.') return true;
    } else if (abs(tc - fc) == 1 && tr - fr == direction) {
        // Diagonal capture
        if (board[tr][tc] != '.' &&
            ((piece == 'P' && islower(board[tr][tc])) ||
             (piece == 'p' && isupper(board[tr][tc])))) return true;
        // En passant
        if (tr == enPassantRow && tc == enPassantCol) return true;
    }
    return false;
}
```

---

## Attack Detection

Pawns attack diagonally but move forward. `isAttacking` uses `isPawnAttacking` for pawns and the normal movement validators for everything else:

```cpp
bool chessboard::isPawnAttacking(char piece, int fr, int fc, int tr, int tc) {
    int direction = (piece == 'P') ? -1 : 1;
    return (tr - fr == direction) && (abs(tc - fc) == 1);
}
```

`isSquareAttackedBy` scans all pieces of a given side and asks whether any of them attack a specific square:

```cpp
bool chessboard::isSquareAttackedBy(int row, int col, int attackerTurn) {
    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++) {
            char piece = board[r][c];
            if (piece == '.') continue;
            bool isPieceOfAttacker = (attackerTurn == 1) ? isupper(piece) : islower(piece);
            if (!isPieceOfAttacker) continue;
            if (isAttacking(piece, r, c, row, col)) return true;
        }
    return false;
}
```

This is used exclusively by `canCastle` to verify that the king does not pass through attacked squares.

---

## Check Detection

```cpp
bool chessboard::isInCheck(int turn) {
    char kingChar = (turn == 1) ? 'K' : 'k';
    int kingRow = -1, kingCol = -1;

    for (int r = 0; r < 8; r++)
        for (int c = 0; c < 8; c++)
            if (board[r][c] == kingChar) { kingRow = r; kingCol = c; }

    if (kingRow == -1) return false;  // safety guard — king should always exist

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
```

Called inside `makeMove` after every simulated move, inside `getLegalMovesFor` for every candidate destination, and inside `canCastle` to verify the king is not currently in check before castling.

---

## Legal Move Generation

```cpp
std::vector<std::pair<int,int>> chessboard::getLegalMovesFor(int row, int col)
```

For each of the 63 other squares, the function tests whether the piece can move there (`isValidMove`), simulates the move, checks `isInCheck`, and undoes it. Only destinations that do not leave the king in check are included.

En passant captures require removing the captured pawn at `(row, toCol)` — not `(toRow, toCol)` — during the simulation:

```cpp
if ((piece == 'P' || piece == 'p') && abs(tc - col) == 1 && board[tr][tc] == '.') {
    epCapRow = row;
    board[row][tc] = '.';  // remove the en passant victim before checking
}
```

Castling destinations are not returned by this function. The GUI injects `{kingRow, 6}` and `{kingRow, 2}` for kings on their home square.

---

## Making a Move

Full sequence inside `makeMove`:

```
1. Castling shortcut — detect "O-O" / "O-O-O" before any other processing
2. Regex format check — [a-h][1-8]
3. Same-square check
4. Convert algebraic to row/col indices
5. Empty square check
6. Wrong turn check (pieceValues sign: positive = white, negative = black)
7. Friendly fire check
8. Piece movement geometry (isValidMove)
9. Save state (board squares, en passant coords)
10. Handle en passant capture (remove victim pawn)
11. Apply move to board
12. isInCheck — rollback and throw MovesIntoCheckException if true
13. Update king/rook moved flags
14. Update halfMoveClock (reset on pawn move or capture)
15. Increment moveCounter
16. Record in moveHistory and positionHistory
17. Set pendingPromotion if pawn reached back rank
```

---

## Special Moves

### En Passant

After a double pawn push, `enPassantRow/Col` is set to the skipped square:

```cpp
if (tonum - fromnum == 2 * direction) {
    enPassantRow = fromnum + direction;
    enPassantCol = fromletter;
}
```

Reset to `-1` at the start of every `makeMove` call — the window is exactly one move.

The captured pawn is at `(fromRow, toCol)`, not the destination square:

```cpp
if (abs(toletter - fromletter) == 1 && board[tonum][toletter] == '.') {
    epCaptureRow = fromnum;
    board[fromnum][toletter] = '.';  // remove the captured pawn
}
```

### Castling

Seven conditions checked in `canCastle`:

1. King has not moved
2. Relevant rook has not moved
3. Squares between king and rook are empty
4. King is not currently in check
5. King's transit square is not attacked
6. King's destination is not attacked
7. *(Queenside only)* b-file square must also be empty (even though the king doesn't pass through it, the rook does)

`performCastle` moves both pieces in a single operation using named constants:

```
Kingside:   King e1→g1, Rook h1→f1
Queenside:  King e1→c1, Rook a1→d1
(Mirror for black on rank 8)
```

### Pawn Promotion

When a pawn lands on the back rank, `makeMove` sets `pendingPromotion = true` and records the square. The GUI polls `needsPromotion()` and presents a picker. The player's choice is submitted via `promotePawn('Q')` etc:

```cpp
void chessboard::promotePawn(char piece) {
    char current = board[promotionRow][promotionCol];
    board[promotionRow][promotionCol] = isupper(current) ? toupper(piece) : tolower(piece);
    pendingPromotion = false;
    promotionRow = promotionCol = -1;
}
```

`checkGameOver()` must not be called while `pendingPromotion` is true.

---

## Game Over Conditions

### Checkmate & Stalemate

`hasLegalMove(turn)` iterates all pieces and all destinations, simulating each move to find at least one that does not leave the king in check. Returns `true` as soon as one is found.

If no legal move exists:
- King is in check → **Checkmate**
- King is not in check → **Stalemate**

### Fifty-Move Rule

```cpp
if (halfMoveClock >= 100) {
    outResult = "Draw by the fifty-move rule!";
    return true;
}
```

`halfMoveClock` is reset on any pawn move or capture, incremented otherwise. 100 half-moves = 50 full moves.

### Threefold Repetition

`getBoardHash()` encodes 64 piece characters + en passant coords + 6 castling flags. This is appended to `positionHistory` after every move. `checkGameOver` counts occurrences of the current hash in the full history — three or more is a draw. Non-consecutive repetitions are correctly detected.

### Insufficient Material

Draw when neither side can force checkmate:

| Position | Result |
|----------|--------|
| K vs K | Draw |
| K+B vs K | Draw |
| K+N vs K | Draw |
| K+B vs K+B | Draw |
| Anything with Q, R, or P | Not a draw |

KNN vs K is not included — by strict FIDE rules it is not a forced draw.

---

## FEN Export

```cpp
std::string chessboard::toFEN()
```

| Field | Example | Source |
|-------|---------|--------|
| Piece placement | `rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR` | `board[8][8]` |
| Active colour | `w` or `b` | `getTurn()` |
| Castling availability | `KQkq` or `-` | moved flags |
| En passant target | `e3` or `-` | `enPassantRow/Col` |
| Half-move clock | `0` | `halfMoveClock` |
| Full-move number | `1` | `moveCounter / 2 + 1` |

Compatible with Lichess's board editor and any standard FEN parser.

---

## Exception Hierarchy

All exceptions inherit from `ChessException : std::runtime_error`. The GUI catches `ChessException` generically at the call site.

```
std::runtime_error
└── ChessException
    ├── InvalidFormatException     move string doesn't match [a-h][1-8]
    ├── SameSquareException        from == to
    ├── EmptySquareException       no piece on source square
    ├── WrongTurnException         moving a piece belonging to the other side
    ├── FriendlyFireException      destination occupied by own piece
    ├── InvalidPieceMoveException  piece cannot geometrically reach destination
    └── MovesIntoCheckException    move would leave own king in check
```

`ChessException` itself (base, not a subclass) is thrown directly only by `performCastle` when `canCastle` returns false.
