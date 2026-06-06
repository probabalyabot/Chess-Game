#include <SFML/Graphics.hpp>
#include "chessboard.h"
#include <map>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>

// ─── Colours ────────────────────────────────────────────────────────────────
sf::Color LIGHT_SQUARE = sf::Color(240, 217, 181);
sf::Color DARK_SQUARE  = sf::Color(181, 136,  99);
sf::Color HIGHLIGHT    = sf::Color(186, 202,  68, 180);
sf::Color LEGAL_MOVE   = sf::Color(  0,   0,   0,  80);
sf::Color CHECK_COLOR  = sf::Color(255,   0,   0, 180);

// ─── Helper ─────────────────────────────────────────────────────────────────
std::string pieceToFile(char p) {
    std::string prefix = isupper(p) ? "w" : "b";
    char upper = toupper(p);
    std::string name;
    switch (upper) {
        case 'P': name = "P"; break;
        case 'N': name = "N"; break;
        case 'B': name = "B"; break;
        case 'R': name = "R"; break;
        case 'Q': name = "Q"; break;
        case 'K': name = "K"; break;
        default: return "";
    }
    return "assets/" + prefix + name + ".png";
}

std::string formatTime(int seconds) {
    if (seconds < 0) seconds = 0;
    int m = seconds / 60;
    int s = seconds % 60;
    char buf[8];
    snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
    return std::string(buf);
}

// ─── Simple button helper ────────────────────────────────────────────────────
struct Button {
    sf::RectangleShape rect;
    sf::Text           label;

    void setSelected(bool sel) {
        rect.setFillColor(sel ? sf::Color(200, 170, 60) : sf::Color(60, 60, 60));
    }

    bool contains(int mx, int my) const {
        return rect.getGlobalBounds().contains((float)mx, (float)my);
    }

    void draw(sf::RenderWindow& w) {
        w.draw(rect);
        w.draw(label);
    }
};

Button makeButton(float x, float y, float w, float h,
                  const std::string& text, const sf::Font& font, int fontSize) {
    Button b;
    b.rect.setSize({w, h});
    b.rect.setPosition(x, y);
    b.rect.setFillColor(sf::Color(60, 60, 60));
    b.rect.setOutlineColor(sf::Color(120, 120, 120));
    b.rect.setOutlineThickness(1.f);

    b.label.setFont(font);
    b.label.setString(text);
    b.label.setCharacterSize(fontSize);
    b.label.setFillColor(sf::Color::White);

    sf::FloatRect lb = b.label.getLocalBounds();
    b.label.setPosition(
        x + (w - lb.width)  / 2.f - lb.left,
        y + (h - lb.height) / 2.f - lb.top
    );
    return b;
}

// ─── State ───────────────────────────────────────────────────────────────────
enum State { LANDING, PLAYING };

int main() {
    srand((unsigned)time(nullptr));

    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();
    sf::RenderWindow window(desktop, "Chess", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    const int windowW = (int)desktop.width;
    const int windowH = (int)desktop.height;

    const int boardPx   = (int)(windowH * 0.90f) / 8 * 8;
    const int tileSize  = boardPx / 8;
    const int panelW    = windowW - boardPx - 40;
    const int boardOffX = panelW + 20;
    const int boardOffY = (windowH - boardPx) / 2;

    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/font.ttf");

    std::map<char, sf::Texture> textures;
    for (char c : std::string("pnbrqkPNBRQK")) {
        sf::Texture tex;
        std::string path = pieceToFile(c);
        if (!tex.loadFromFile(path)) {
            std::cerr << "Failed to load: " << path << "\n";
            return -1;
        }
        textures[c] = tex;
    }

    // ── Landing screen state ────────────────────────────────────────────────
    State appState = LANDING;

    const int timePresets[3]          = { 180, 600, 1800 };
    const std::string presetLabels[3] = { "3+0\nBlitz", "10+0\nRapid", "30+0\nClassical" };
    int selectedPreset = 1;

    const std::string sideLabels[3] = { "White", "Black", "Random" };
    int selectedSide = 0;

    const int landBtnW   = std::min(220, (windowW / 5));
    const int landBtnH   = std::max(50,  windowH / 12);
    const int landFontSz = std::max(16,  landBtnH / 3);

    const int timeTotalW = landBtnW * 3 + 40;
    const int timeStartX = (windowW - timeTotalW) / 2;
    const int timeY      = (int)(windowH * 0.45f);

    Button timeBtn[3];
    for (int i = 0; i < 3; i++) {
        timeBtn[i] = makeButton(timeStartX + i * (landBtnW + 20), timeY,
                                landBtnW, landBtnH,
                                presetLabels[i], font, landFontSz);
    }

    const int sideTotalW = landBtnW * 3 + 40;
    const int sideStartX = (windowW - sideTotalW) / 2;
    const int sideY      = (int)(windowH * 0.62f);

    Button sideBtn[3];
    for (int i = 0; i < 3; i++) {
        sideBtn[i] = makeButton(sideStartX + i * (landBtnW + 20), sideY,
                                landBtnW, landBtnH,
                                sideLabels[i], font, landFontSz);
    }

    const int startBtnW = landBtnW + 40;
    const int startBtnH = landBtnH;
    Button startBtn = makeButton((windowW - startBtnW) / 2, (int)(windowH * 0.78f),
                                 startBtnW, startBtnH,
                                 "Start Game", font, landFontSz);
    startBtn.rect.setFillColor(sf::Color(50, 140, 50));
    startBtn.rect.setOutlineColor(sf::Color(100, 200, 100));

    // ── Game state ──────────────────────────────────────────────────────────
    chessboard* board = nullptr;

    int selectedRow = -1, selectedCol = -1;
    std::vector<std::pair<int,int>> legalMoves;
    bool pendingPromo  = false;
    bool gameOver      = false;
    std::string gameResult;
    bool resignConfirm = false;
    bool drawOffered   = false;

    float clockSecondsF[2] = {0.f, 0.f};
    bool  clockRunning     = false;
    sf::Clock tickClock;

    // humanSide: 0=white, 1=black, -1=both (local 2P)
    // Always -1 until AI is added; selectedSide stored for future use.
    int humanSide = -1;

    const int btnW = panelW - 20;
    const int btnH = std::max(30, tileSize / 2);
    const int btnX = 10;

    sf::RectangleShape resignBtn(sf::Vector2f(btnW, btnH));
    resignBtn.setFillColor(sf::Color(180, 50, 50));
    resignBtn.setPosition(btnX, windowH - btnH * 2 - 20);

    sf::RectangleShape drawBtn(sf::Vector2f(btnW, btnH));
    drawBtn.setFillColor(sf::Color(80, 80, 180));
    drawBtn.setPosition(btnX, windowH - btnH - 10);

    const int cfW = (btnW - 10) / 2;
    sf::RectangleShape confirmYes(sf::Vector2f(cfW, btnH));
    confirmYes.setFillColor(sf::Color(50, 150, 50));
    confirmYes.setPosition(btnX, windowH - btnH * 3 - 30);

    sf::RectangleShape confirmNo(sf::Vector2f(cfW, btnH));
    confirmNo.setFillColor(sf::Color(180, 50, 50));
    confirmNo.setPosition(btnX + cfW + 10, windowH - btnH * 3 - 30);

    // ── Main loop ────────────────────────────────────────────────────────────
    while (window.isOpen()) {
        float frameDelta = tickClock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                if (event.key.code == sf::Keyboard::Escape)
                    window.close();

                if (event.key.code == sf::Keyboard::M &&
                    appState == PLAYING && gameOver) {
                    appState = LANDING;
                    delete board;
                    board = nullptr;
                }
            }

            // ── Landing input ──────────────────────────────────────────────
            if (appState == LANDING &&
                event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                for (int i = 0; i < 3; i++) {
                    if (timeBtn[i].contains(mx, my)) selectedPreset = i;
                    if (sideBtn[i].contains(mx, my)) selectedSide   = i;
                }

                if (startBtn.contains(mx, my)) {
                    // selectedSide stored for future AI use; always local 2P for now
                    humanSide = -1;

                    float t = (float)timePresets[selectedPreset];
                    clockSecondsF[0] = t;
                    clockSecondsF[1] = t;
                    clockRunning = false;

                    delete board;
                    board = new chessboard();

                    selectedRow = selectedCol = -1;
                    legalMoves.clear();
                    pendingPromo  = false;
                    gameOver      = false;
                    gameResult    = "";
                    resignConfirm = false;
                    drawOffered   = false;

                    tickClock.restart();
                    appState = PLAYING;
                }
            }

            // ── Game input ─────────────────────────────────────────────────
            if (appState == PLAYING) {
                if (gameOver) continue;

                if (event.type == sf::Event::MouseButtonPressed &&
                    event.mouseButton.button == sf::Mouse::Left) {

                    int mx = event.mouseButton.x;
                    int my = event.mouseButton.y;

                    // ── Resign / draw confirmation dialog ──────────────────
                    if (resignConfirm || drawOffered) {
                        if (confirmYes.getGlobalBounds().contains(mx, my)) {
                            if (resignConfirm) {
                                gameResult  = (board->getTurn() == 1 ? "White" : "Black");
                                gameResult += " resigns. ";
                                gameResult += (board->getTurn() == 1 ? "Black" : "White");
                                gameResult += " wins!";
                            } else {
                                gameResult = "Draw agreed!";
                            }
                            gameOver      = true;
                            clockRunning  = false;
                            resignConfirm = drawOffered = false;
                            continue;
                        }
                        if (confirmNo.getGlobalBounds().contains(mx, my)) {
                            resignConfirm = drawOffered = false;
                            continue;
                        }
                        // Click outside dialog dismisses it
                        resignConfirm = drawOffered = false;
                        // fall through so board click is still processed
                    }

                    if (resignBtn.getGlobalBounds().contains(mx, my)) {
                        resignConfirm = true; drawOffered = false; continue;
                    }
                    if (drawBtn.getGlobalBounds().contains(mx, my)) {
                        drawOffered = true; resignConfirm = false; continue;
                    }

                    // ── Promotion overlay ──────────────────────────────────
                    if (pendingPromo) {
                        std::string opts = "QRBN";
                        int promoTileW = tileSize * 2;
                        int totalW     = promoTileW * 4;
                        int startX     = boardOffX + (boardPx - totalW) / 2;
                        int startY     = windowH / 2 - tileSize;
                        for (int i = 0; i < 4; i++) {
                            sf::FloatRect btn(startX + i * promoTileW, startY,
                                              promoTileW, promoTileW);
                            if (btn.contains(mx, my)) {
                                board->promotePawn(opts[i]);
                                pendingPromo = false;
                                if (board->checkGameOver(gameResult)) {
                                    gameOver     = true;
                                    clockRunning = false;
                                }
                            }
                        }
                        continue;
                    }

                    // ── Board click: reject clicks outside the board ────────
                    if (mx < boardOffX || mx >= boardOffX + boardPx ||
                        my < boardOffY || my >= boardOffY + boardPx) {
                        selectedRow = selectedCol = -1;
                        legalMoves.clear();
                        continue;
                    }

                    int col = (mx - boardOffX) / tileSize;
                    int row = (my - boardOffY) / tileSize;

                    // Enforce humanSide — -1 means both sides playable (local 2P)
                    {
                        int currentTurn  = board->getTurn();
                        bool isHumanTurn = (humanSide == -1) ||
                                           (humanSide == 0 && currentTurn == 1) ||
                                           (humanSide == 1 && currentTurn == 2);
                        if (!isHumanTurn) continue;
                    }

                    if (selectedRow == -1) {
                        char piece = board->getBoard(row, col);
                        int  turn  = board->getTurn();
                        bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                        if (piece != '.' && isOwn) {
                            selectedRow = row;
                            selectedCol = col;
                            legalMoves  = board->getLegalMovesFor(row, col);

                            char kingChar = (turn == 1) ? 'K' : 'k';
                            int  kingRow  = (turn == 1) ? 7 : 0;
                            if (piece == kingChar && row == kingRow && col == 4) {
                                if (board->canCastleKingside(turn))
                                    legalMoves.push_back({kingRow, 6});
                                if (board->canCastleQueenside(turn))
                                    legalMoves.push_back({kingRow, 2});
                            }
                        }
                    } else {
                        bool isLegal = false;
                        for (auto& m : legalMoves)
                            if (m.first == row && m.second == col) { isLegal = true; break; }

                        if (isLegal) {
                            char movingPiece = board->getBoard(selectedRow, selectedCol);
                            char kingChar    = (board->getTurn() == 1) ? 'K' : 'k';
                            int  kingRow     = (board->getTurn() == 1) ? 7 : 0;

                            bool isCastle = false;
                            std::string castleStr;
                            if (movingPiece == kingChar &&
                                selectedRow == kingRow && selectedCol == 4) {
                                if (row == kingRow && col == 6) {
                                    isCastle = true; castleStr = "O-O";
                                } else if (row == kingRow && col == 2) {
                                    isCastle = true; castleStr = "O-O-O";
                                }
                            }

                            if (!clockRunning) {
                                clockRunning = true;
                                tickClock.restart();
                            }

                            try {
                                if (isCastle) {
                                    board->makeMove(castleStr, "");
                                } else {
                                    std::string from, to;
                                    from += (char)('a' + selectedCol);
                                    from += (char)('8' - selectedRow);
                                    to   += (char)('a' + col);
                                    to   += (char)('8' - row);
                                    board->makeMove(from, to);
                                }

                                if (board->needsPromotion()) {
                                    pendingPromo = true;
                                } else {
                                    if (board->checkGameOver(gameResult)) {
                                        gameOver     = true;
                                        clockRunning = false;
                                    }
                                }
                            } catch (const ChessException& e) {
                                std::cerr << "Move error: " << e.what() << "\n";
                            }

                            selectedRow = selectedCol = -1;
                            legalMoves.clear();
                        } else {
                            // Clicked a different own piece — re-select
                            char piece = board->getBoard(row, col);
                            int  turn  = board->getTurn();
                            bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                            if (piece != '.' && isOwn) {
                                selectedRow = row;
                                selectedCol = col;
                                legalMoves  = board->getLegalMovesFor(row, col);

                                char kingChar = (turn == 1) ? 'K' : 'k';
                                int  kingRow  = (turn == 1) ? 7 : 0;
                                if (piece == kingChar && row == kingRow && col == 4) {
                                    if (board->canCastleKingside(turn))
                                        legalMoves.push_back({kingRow, 6});
                                    if (board->canCastleQueenside(turn))
                                        legalMoves.push_back({kingRow, 2});
                                }
                            } else {
                                selectedRow = selectedCol = -1;
                                legalMoves.clear();
                            }
                        }
                    }
                }
            }
        } // end event loop

        // ── Clock tick ───────────────────────────────────────────────────────
        if (appState == PLAYING && clockRunning && !gameOver && !pendingPromo && board) {
            int activePlayer = (board->getTurn() == 1) ? 0 : 1;
            clockSecondsF[activePlayer] -= frameDelta;
            if (clockSecondsF[activePlayer] < 0.f)
                clockSecondsF[activePlayer] = 0.f;

            if (clockSecondsF[activePlayer] <= 0.f) {
                std::string loser  = (activePlayer == 0) ? "White" : "Black";
                std::string winner = (activePlayer == 0) ? "Black" : "White";
                gameResult   = loser + " ran out of time. " + winner + " wins!";
                gameOver     = true;
                clockRunning = false;
            }
        }

        // ── Draw ─────────────────────────────────────────────────────────────
        window.clear(sf::Color(30, 30, 30));

        // ── Landing screen ───────────────────────────────────────────────────
        if (appState == LANDING && fontLoaded) {
            int titleSz = std::max(48, windowH / 12);
            sf::Text title("CHESS", font, titleSz);
            title.setFillColor(sf::Color(220, 200, 140));
            sf::FloatRect tb = title.getLocalBounds();
            title.setPosition((windowW - tb.width) / 2.f - tb.left,
                               windowH * 0.12f - tb.top);
            window.draw(title);

            int secSz = std::max(18, windowH / 30);
            auto drawLabel = [&](const std::string& s, float y) {
                sf::Text lbl(s, font, secSz);
                lbl.setFillColor(sf::Color(160, 160, 160));
                sf::FloatRect lb = lbl.getLocalBounds();
                lbl.setPosition((windowW - lb.width) / 2.f - lb.left, y - lb.top);
                window.draw(lbl);
            };

            drawLabel("Time Control", timeY - secSz * 2.0f);
            drawLabel("Play As",      sideY - secSz * 2.0f);

            for (int i = 0; i < 3; i++) {
                timeBtn[i].setSelected(i == selectedPreset);
                timeBtn[i].draw(window);
            }
            for (int i = 0; i < 3; i++) {
                sideBtn[i].setSelected(i == selectedSide);
                sideBtn[i].draw(window);
            }

            startBtn.draw(window);

            sf::Text hint("ESC - exit", font, std::max(12, windowH / 50));
            hint.setFillColor(sf::Color(70, 70, 70));
            hint.setPosition(10, windowH - 24);
            window.draw(hint);
        }

        // ── Game screen ──────────────────────────────────────────────────────
        if (appState == PLAYING && board != nullptr) {

            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
                    tile.setPosition(boardOffX + c * tileSize, boardOffY + r * tileSize);

                    bool isSelected = (r == selectedRow && c == selectedCol);
                    bool isLegalSq  = false;
                    for (auto& m : legalMoves)
                        if (m.first == r && m.second == c) { isLegalSq = true; break; }

                    bool isKingInCheck = false;
                    if (board->isInCheck(board->getTurn())) {
                        char kingChar = (board->getTurn() == 1) ? 'K' : 'k';
                        if (board->getBoard(r, c) == kingChar) isKingInCheck = true;
                    }

                    if (isKingInCheck)          tile.setFillColor(CHECK_COLOR);
                    else if (isSelected)        tile.setFillColor(HIGHLIGHT);
                    else if ((r + c) % 2 == 0) tile.setFillColor(LIGHT_SQUARE);
                    else                        tile.setFillColor(DARK_SQUARE);

                    window.draw(tile);

                    if (isLegalSq) {
                        sf::CircleShape dot(tileSize * 0.15f);
                        dot.setFillColor(LEGAL_MOVE);
                        dot.setOrigin(tileSize * 0.15f, tileSize * 0.15f);
                        dot.setPosition(
                            boardOffX + c * tileSize + tileSize / 2.0f,
                            boardOffY + r * tileSize + tileSize / 2.0f
                        );
                        window.draw(dot);
                    }

                    char piece = board->getBoard(r, c);
                    if (piece != '.') {
                        sf::Sprite sprite(textures[piece]);
                        float scale = (float)tileSize / sprite.getLocalBounds().width;
                        sprite.setScale(scale, scale);
                        sprite.setPosition(boardOffX + c * tileSize, boardOffY + r * tileSize);
                        window.draw(sprite);
                    }
                }
            }

            if (fontLoaded) {
                int labelSize = std::max(12, tileSize / 5);
                for (int c = 0; c < 8; c++) {
                    sf::Text lbl(std::string(1, 'a' + c), font, labelSize);
                    lbl.setFillColor(sf::Color(200, 200, 200));
                    lbl.setPosition(boardOffX + c * tileSize + tileSize / 2 - labelSize / 3,
                                    boardOffY + boardPx + 4);
                    window.draw(lbl);
                }
                for (int r = 0; r < 8; r++) {
                    sf::Text lbl(std::string(1, '8' - r), font, labelSize);
                    lbl.setFillColor(sf::Color(200, 200, 200));
                    lbl.setPosition(boardOffX - labelSize - 4,
                                    boardOffY + r * tileSize + tileSize / 2 - labelSize / 2);
                    window.draw(lbl);
                }
            }

            if (fontLoaded) {
                sf::RectangleShape panel(sf::Vector2f(panelW, windowH));
                panel.setFillColor(sf::Color(20, 20, 20));
                panel.setPosition(0, 0);
                window.draw(panel);

                int titleSz = std::max(14, tileSize / 4);
                int moveSz  = std::max(12, tileSize / 5);
                int clockSz = std::max(28, tileSize / 2);

                int activeSide = (board->getTurn() == 1) ? 0 : 1;

                auto drawClock = [&](int player, float y) {
                    bool isActive = (!gameOver && clockRunning && activeSide == player);
                    std::string label   = (player == 0) ? "White" : "Black";
                    int dispSecs        = (int)clockSecondsF[player];
                    std::string timeStr = formatTime(dispSecs);

                    sf::RectangleShape bg(sf::Vector2f(btnW, clockSz + 16));
                    bg.setPosition(btnX, y);
                    bg.setFillColor(isActive
                        ? sf::Color(60, 90, 60)
                        : sf::Color(35, 35, 35));
                    bg.setOutlineColor(isActive
                        ? sf::Color(100, 180, 100)
                        : sf::Color(70, 70, 70));
                    bg.setOutlineThickness(1.f);
                    window.draw(bg);

                    sf::Text lbl(label, font, moveSz);
                    lbl.setFillColor(sf::Color(160, 160, 160));
                    lbl.setPosition(btnX + 8, y + 2);
                    window.draw(lbl);

                    sf::Text timeTxt(timeStr, font, clockSz);
                    timeTxt.setFillColor(dispSecs < 30
                        ? sf::Color(220, 80, 80)
                        : sf::Color::White);
                    sf::FloatRect tb2 = timeTxt.getLocalBounds();
                    timeTxt.setPosition(
                        btnX + (btnW - tb2.width) / 2.f - tb2.left,
                        y + (clockSz + 16 - tb2.height) / 2.f - tb2.top + 4
                    );
                    window.draw(timeTxt);
                };

                float blackClockY = (float)(windowH / 2 - (clockSz + 16) - 20);
                float whiteClockY = (float)(windowH / 2 + 20);
                drawClock(1, blackClockY);
                drawClock(0, whiteClockY);

                sf::Text histTitle("Move History", font, titleSz);
                histTitle.setFillColor(sf::Color(220, 220, 220));
                histTitle.setPosition(10, 10);
                window.draw(histTitle);

                auto history = board->getMoveHistory();
                int maxVisible = ((int)blackClockY - (titleSz + 20)) / (moveSz + 6);
                int startIdx   = std::max(0, (int)history.size() - maxVisible * 2);
                if (startIdx % 2 != 0) startIdx--;
                int yOff = titleSz + 20;
                for (int i = startIdx; i < (int)history.size(); i += 2) {
                    std::string line = std::to_string(i / 2 + 1) + ". " + history[i];
                    if (i + 1 < (int)history.size()) line += "  " + history[i + 1];
                    sf::Text mv(line, font, moveSz);
                    mv.setFillColor(sf::Color(180, 180, 180));
                    mv.setPosition(10, yOff);
                    window.draw(mv);
                    yOff += moveSz + 6;
                }

                sf::Text turnTxt(board->getTurn() == 1 ? "White's turn" : "Black's turn",
                                 font, moveSz);
                turnTxt.setFillColor(board->getTurn() == 1
                    ? sf::Color::White : sf::Color(150, 150, 150));
                sf::FloatRect ttb = turnTxt.getLocalBounds();
                turnTxt.setPosition(
                    btnX + (btnW - ttb.width) / 2.f - ttb.left,
                    windowH / 2.f - ttb.height / 2.f - ttb.top
                );
                window.draw(turnTxt);

                window.draw(resignBtn);
                sf::Text resignTxt("Resign", font, moveSz);
                resignTxt.setFillColor(sf::Color::White);
                auto rb = resignBtn.getPosition();
                resignTxt.setPosition(rb.x + 8, rb.y + (btnH - moveSz) / 2);
                window.draw(resignTxt);

                window.draw(drawBtn);
                sf::Text drawTxt("Offer Draw", font, moveSz);
                drawTxt.setFillColor(sf::Color::White);
                auto db = drawBtn.getPosition();
                drawTxt.setPosition(db.x + 8, db.y + (btnH - moveSz) / 2);
                window.draw(drawTxt);

                if (resignConfirm || drawOffered) {
                    sf::Text prompt(resignConfirm ? "Confirm resign?" : "Accept draw?",
                                    font, moveSz);
                    prompt.setFillColor(sf::Color::Yellow);
                    prompt.setPosition(10, confirmYes.getPosition().y - moveSz - 6);
                    window.draw(prompt);

                    window.draw(confirmYes);
                    sf::Text yesTxt("Yes", font, moveSz);
                    yesTxt.setFillColor(sf::Color::White);
                    auto cy = confirmYes.getPosition();
                    yesTxt.setPosition(cy.x + 6, cy.y + (btnH - moveSz) / 2);
                    window.draw(yesTxt);

                    window.draw(confirmNo);
                    sf::Text noTxt("No", font, moveSz);
                    noTxt.setFillColor(sf::Color::White);
                    auto cn = confirmNo.getPosition();
                    noTxt.setPosition(cn.x + 6, cn.y + (btnH - moveSz) / 2);
                    window.draw(noTxt);
                }

                sf::Text hint("ESC - exit  |  M - menu (after game)", font, std::max(10, tileSize / 6));
                hint.setFillColor(sf::Color(80, 80, 80));
                hint.setPosition(10, windowH - 20);
                window.draw(hint);
            }

            // ── Promotion overlay ────────────────────────────────────────────
            if (pendingPromo) {
                sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
                overlay.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(overlay);

                std::string opts = "QRBN";
                int promoTileW = tileSize * 2;
                int totalW     = promoTileW * 4;
                int startX     = boardOffX + (boardPx - totalW) / 2;
                int startY     = windowH / 2 - tileSize;

                for (int i = 0; i < 4; i++) {
                    sf::RectangleShape btn(sf::Vector2f(promoTileW, promoTileW));
                    btn.setPosition(startX + i * promoTileW, startY);
                    btn.setFillColor(sf::Color::White);
                    window.draw(btn);

                    int promoTurn = (board->getTurn() == 1) ? 2 : 1;
                    char pc = (promoTurn == 1) ? toupper(opts[i]) : tolower(opts[i]);
                    if (textures.count(pc)) {
                        sf::Sprite sprite(textures[pc]);
                        float scale = (float)promoTileW / sprite.getLocalBounds().width;
                        sprite.setScale(scale, scale);
                        sprite.setPosition(startX + i * promoTileW, startY);
                        window.draw(sprite);
                    }
                }
            }

            // ── Game over overlay ────────────────────────────────────────────
            if (gameOver && fontLoaded) {
                sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
                overlay.setFillColor(sf::Color(0, 0, 0, 150));
                window.draw(overlay);

                int goSz = std::max(24, tileSize / 2);
                sf::Text txt(gameResult, font, goSz);
                txt.setFillColor(sf::Color::White);
                sf::FloatRect b = txt.getLocalBounds();
                txt.setPosition((windowW - b.width) / 2.f - b.left,
                                windowH / 2.f - goSz);
                window.draw(txt);

                int hintSz = std::max(16, goSz / 2);
                sf::Text menuHint("Press M to return to menu", font, hintSz);
                menuHint.setFillColor(sf::Color(160, 160, 160));
                sf::FloatRect mb = menuHint.getLocalBounds();
                menuHint.setPosition((windowW - mb.width) / 2.f - mb.left,
                                     windowH / 2.f + goSz);
                window.draw(menuHint);
            }
        }

        window.display();
    }

    delete board;
    return 0;
}