# ♟ Chess

A fully-featured two-player chess game with a native GUI built in C++ and SFML. Supports the complete FIDE ruleset including castling, en passant, pawn promotion, and all draw conditions.

![C++](https://img.shields.io/badge/C++-20-00599C?style=flat&logo=c%2B%2B)
![SFML](https://img.shields.io/badge/SFML-2.x-8CC445?style=flat)
![Platform](https://img.shields.io/badge/platform-Windows-0078D6?style=flat)

---

## Features

- Full legal move generation with check validation
- Castling (kingside and queenside, both colours)
- En passant
- Pawn promotion with in-game piece picker
- Check, checkmate, and stalemate detection
- Draw conditions: fifty-move rule, threefold repetition, insufficient material
- Resign and draw-by-agreement
- Move history panel (algebraic notation)
- Resolution-aware fullscreen rendering — scales correctly on any monitor
- FEN export

---

## Getting Started

### 1. Clone the Repository

If you have Git installed, open a terminal and run:

```bash
git clone https://github.com/probabalyabot/Chess-Game.git
```

If you don't have Git, click the green **Code** button on the GitHub repo page and choose **Download ZIP**, then extract it.

---

### 2. Install SFML

SFML is the graphics/window/audio library this project uses for rendering. You need version **2.x** (not SFML 3).

#### Windows (recommended — MinGW/GCC)

1. Go to [sfml-dev.org/download.php](https://www.sfml-dev.org/download.php) and download the version that matches your compiler. If you're using GCC/MinGW, pick the **GCC MinGW** build. If you're not sure which GCC version you have, run `g++ --version` in a terminal.
2. Extract the downloaded archive somewhere permanent, e.g. `C:\SFML\`.
3. Inside you'll find three folders you care about:
   - `include/` — header files
   - `lib/` — `.a` static libraries used at compile time
   - `bin/` — `.dll` files needed at runtime
4. Copy the `.dll` files from `bin/` into the same folder as your `main.exe` (or add `C:\SFML\bin` to your system PATH).

Else Install SMFL though MYSYS2 terminal + pacman for easier and more direct installation

Your compile command then needs to point at the SFML headers and libs:

```bash
g++ -std=c++20 chessboard.cpp main.cpp -o main.exe \
    -I"C:/SFML/include" \
    -L"C:/SFML/lib" \
    -lsfml-graphics -lsfml-window -lsfml-system
```

#### Windows ("vcpkg" easiest if you already use it)

```bash
vcpkg install sfml
```

Then compile normally,vcpkg handles the include/lib paths automatically if integrated with your build system.

#### Linux (Debian/Ubuntu)

```bash
sudo apt install libsfml-dev
```

Then compile:

```bash
g++ -std=c++20 chessboard.cpp main.cpp -o chess \
    -lsfml-graphics -lsfml-window -lsfml-system
```

#### macOS (Homebrew)

```bash
brew install sfml
```

Then compile:

```bash
g++ -std=c++20 chessboard.cpp main.cpp -o chess \
    -lsfml-graphics -lsfml-window -lsfml-system
```

### 3. Build

```bash
g++ -std=c++20 chessboard.cpp main.cpp -o main.exe \
    -I"C:/SFML/include" \
    -L"C:/SFML/lib" \
    -lsfml-graphics -lsfml-window -lsfml-system
```

Adjust the `-I` and `-L` paths to wherever you extracted SFML. On Linux/macOS with a package manager install, you can omit those flags entirely.

---

### 4. Run

```bash
./main.exe
```

The game launches in fullscreen at your native resolution. Press **ESC** to exit.

---

## Project Structure

```
chess-game/
├── main.cpp          # Rendering, input handling, UI
├── chessboard.cpp    # Game engine — all chess logic
├── chessboard.h      # Engine interface
└── assets/           # Sprites and font
```

### Architecture

The project is split into two clear layers:

**Engine (`chessboard.cpp` / `chessboard.h`)**
Owns all game state and rules. It is entirely independent of SFML, no rendering code lives here. The engine validates every move, tracks piece positions, handles special moves, and detects game-ending conditions.

**GUI (`main.cpp`)**
Reads from the engine via its public API and handles all rendering and user input. It translates mouse clicks into board coordinates, constructs algebraic move strings, and delegates everything else to the engine.

This separation means the engine can be reused or tested independently of the GUI.

---

## Engine API

Defined in `chessboard.h`. All interaction between the GUI and the engine goes through these methods:

```cpp
// Board state
char getBoard(int row, int col);
int  getTurn();                          // 1 = white, 2 = black

// Move generation
std::vector<std::pair<int,int>> getLegalMovesFor(int row, int col);

// Move execution
// Normal move:    makeMove("e2", "e4")
// Castling:       makeMove("O-O", "")  or  makeMove("O-O-O", "")
void makeMove(std::string from, std::string to);
void performCastle(int turn, bool kingSide);

// Promotion
bool needsPromotion();
void promotePawn(char piece);            // 'Q', 'R', 'B', or 'N'

// Game status
bool isInCheck(int turn);
bool checkGameOver(std::string& outResult);

// History & export
std::vector<std::string> getMoveHistory();
std::string toFEN();
```

### Coordinate System

The engine uses **row 0 = rank 8** (black's back rank), **row 7 = rank 1** (white's back rank). When constructing algebraic strings from screen coordinates, the GUI applies this conversion:

```cpp
from += (char)('a' + selectedCol);
from += (char)('8' - selectedRow);
```

### Castling

`getLegalMovesFor()` does not return castling squares directly. The GUI injects `{kingRow, 6}` (kingside) and `{kingRow, 2}` (queenside) into the legal moves list when the selected piece is a king on its home square. When the player clicks one of those squares, the GUI calls `makeMove("O-O", "")` or `makeMove("O-O-O", "")` rather than a normal algebraic move. The engine's `canCastle()` logic handles all legality (path clear, squares not attacked, king/rook not previously moved) and throws `ChessException` if castling is unavailable.

---

## How It Works

### Layout

The window always opens fullscreen at the native desktop resolution. Layout is computed once at startup:

```cpp
const int boardPx   = (int)(windowH * 0.90f) / 8 * 8;  // 90% of screen height
const int tileSize  = boardPx / 8;
const int panelW    = windowW - boardPx - 40;
const int boardOffX = panelW + 20;
const int boardOffY = (windowH - boardPx) / 2;
```

All hit-testing uses `(mx - boardOffX) / tileSize` and `(my - boardOffY) / tileSize`, so coordinates are always correct regardless of resolution.

### Input Flow

1. Player clicks a square inside the board area.
2. If no piece is selected: check ownership, select the piece, fetch legal moves from the engine (plus castling injection for kings).
3. If a piece is already selected:
   - If the click is a legal destination → execute the move via the engine.
   - If the click is another own piece → re-select.
   - Otherwise → deselect.
4. After each move, check `needsPromotion()` and `checkGameOver()`.

### Promotion

When `needsPromotion()` returns true, a full-screen overlay renders four piece options (Q, R, B, N) scaled to `tileSize * 2`. The correct colour is inferred from the previous turn (since `getTurn()` has already advanced).

