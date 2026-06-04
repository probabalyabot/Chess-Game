#include <SFML/Graphics.hpp>
#include "chessboard.h"
#include <map>
#include <string>
#include <iostream>

sf::Color LIGHT_SQUARE  = sf::Color(240, 217, 181);
sf::Color DARK_SQUARE   = sf::Color(181, 136, 99);
sf::Color HIGHLIGHT     = sf::Color(186, 202, 68, 180);
sf::Color LEGAL_MOVE    = sf::Color(0, 0, 0, 80);
sf::Color CHECK_COLOR   = sf::Color(255, 0, 0, 180);

std::string pieceToFile(char p) {
    std::string prefix = isupper(p) ? "w" : "b";
    char upper = toupper(p);
    std::string name;
    switch(upper) {
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

int main() {
    // Always launch fullscreen — layout computed once, never recomputed
    sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

    sf::RenderWindow window(desktop, "Chess", sf::Style::Fullscreen);
    window.setFramerateLimit(60);

    const int windowW = (int)desktop.width;
    const int windowH = (int)desktop.height;

    // Use 90% of screen height for the board so there's always breathing room
    const int boardPx   = (int)(windowH * 0.90f) / 8 * 8;  // round down to multiple of 8
    const int tileSize  = boardPx / 8;
    const int panelW    = windowW - boardPx - 40;           // remaining width minus right margin
    const int boardOffX = panelW + 20;
    const int boardOffY = (windowH - boardPx) / 2;

    // Load textures
    std::map<char, sf::Texture> textures;
    std::string pieces = "pnbrqkPNBRQK";
    for (char c : pieces) {
        sf::Texture tex;
        std::string path = pieceToFile(c);
        if (!tex.loadFromFile(path)) {
            std::cerr << "Failed to load: " << path << "\n";
            return -1;
        }
        textures[c] = tex;
    }

    chessboard board;

    int selectedRow = -1, selectedCol = -1;
    std::vector<std::pair<int,int>> legalMoves;
    bool pendingPromo = false;
    bool gameOver     = false;
    std::string gameResult;
    bool resignConfirm = false;
    bool drawOffered   = false;

    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/font.ttf");

    // Button layout — computed once
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

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            // Escape exits fullscreen
            if (event.type == sf::Event::KeyPressed &&
                event.key.code == sf::Keyboard::Escape) {
                window.close();
            }

            if (gameOver) continue;

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                // Confirm buttons
                if (resignConfirm || drawOffered) {
                    if (confirmYes.getGlobalBounds().contains(mx, my)) {
                        if (resignConfirm) {
                            gameResult  = (board.getTurn() == 1 ? "White" : "Black");
                            gameResult += " resigns. ";
                            gameResult += (board.getTurn() == 1 ? "Black" : "White");
                            gameResult += " wins!";
                        } else {
                            gameResult = "Draw agreed!";
                        }
                        gameOver = true;
                        resignConfirm = drawOffered = false;
                        continue;
                    }
                    if (confirmNo.getGlobalBounds().contains(mx, my)) {
                        resignConfirm = drawOffered = false;
                        continue;
                    }
                }

                if (resignBtn.getGlobalBounds().contains(mx, my)) {
                    resignConfirm = true; drawOffered = false; continue;
                }
                if (drawBtn.getGlobalBounds().contains(mx, my)) {
                    drawOffered = true; resignConfirm = false; continue;
                }

                // Promotion picker
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
                            board.promotePawn(opts[i]);
                            pendingPromo = false;
                            if (board.checkGameOver(gameResult)) gameOver = true;
                        }
                    }
                    continue;
                }

                // Board click — ignore if outside board area
                if (mx < boardOffX || mx >= boardOffX + boardPx ||
                    my < boardOffY || my >= boardOffY + boardPx) {
                    selectedRow = selectedCol = -1;
                    legalMoves.clear();
                    continue;
                }

                int col = (mx - boardOffX) / tileSize;
                int row = (my - boardOffY) / tileSize;

                if (selectedRow == -1) {
                    // First click: select a piece
                    char piece = board.getBoard(row, col);
                    int  turn  = board.getTurn();
                    bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                    if (piece != '.' && isOwn) {
                        selectedRow = row;
                        selectedCol = col;
                        legalMoves  = board.getLegalMovesFor(row, col);

                        // Inject castling destinations for the king
                        char kingChar = (turn == 1) ? 'K' : 'k';
                        int  kingRow  = (turn == 1) ? 7 : 0;
                        if (piece == kingChar && row == kingRow && col == 4) {
                            legalMoves.push_back({kingRow, 6}); // kingside
                            legalMoves.push_back({kingRow, 2}); // queenside
                        }
                    }
                } else {
                    bool isLegal = false;
                    for (auto& m : legalMoves)
                        if (m.first == row && m.second == col) { isLegal = true; break; }

                    if (isLegal) {
                        char movingPiece = board.getBoard(selectedRow, selectedCol);
                        char kingChar    = (board.getTurn() == 1) ? 'K' : 'k';
                        int  kingRow     = (board.getTurn() == 1) ? 7 : 0;

                        bool isCastle = false;
                        std::string castleStr;
                        if (movingPiece == kingChar &&
                            selectedRow == kingRow && selectedCol == 4) {
                            if (row == kingRow && col == 6) {
                                isCastle  = true;
                                castleStr = "O-O";
                            } else if (row == kingRow && col == 2) {
                                isCastle  = true;
                                castleStr = "O-O-O";
                            }
                        }

                        try {
                            if (isCastle) {
                                board.makeMove(castleStr, "");
                            } else {
                                std::string from, to;
                                from += (char)('a' + selectedCol);
                                from += (char)('8' - selectedRow);
                                to   += (char)('a' + col);
                                to   += (char)('8' - row);
                                board.makeMove(from, to);
                            }

                            if (board.needsPromotion()) {
                                pendingPromo = true;
                            } else {
                                if (board.checkGameOver(gameResult)) gameOver = true;
                            }
                        } catch (const ChessException& e) {
                            std::cerr << "Move error: " << e.what() << "\n";
                        }

                        selectedRow = selectedCol = -1;
                        legalMoves.clear();
                    } else {
                        // Re-select another own piece
                        char piece = board.getBoard(row, col);
                        int  turn  = board.getTurn();
                        bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                        if (piece != '.' && isOwn) {
                            selectedRow = row;
                            selectedCol = col;
                            legalMoves  = board.getLegalMovesFor(row, col);

                            char kingChar = (turn == 1) ? 'K' : 'k';
                            int  kingRow  = (turn == 1) ? 7 : 0;
                            if (piece == kingChar && row == kingRow && col == 4) {
                                legalMoves.push_back({kingRow, 6});
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

        window.clear(sf::Color(30, 30, 30));

        // Draw board squares + pieces
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                sf::RectangleShape tile(sf::Vector2f(tileSize, tileSize));
                tile.setPosition(boardOffX + c * tileSize, boardOffY + r * tileSize);

                bool isSelected = (r == selectedRow && c == selectedCol);
                bool isLegalSq  = false;
                for (auto& m : legalMoves)
                    if (m.first == r && m.second == c) { isLegalSq = true; break; }

                bool isKingInCheck = false;
                if (board.isInCheck(board.getTurn())) {
                    char kingChar = (board.getTurn() == 1) ? 'K' : 'k';
                    if (board.getBoard(r, c) == kingChar) isKingInCheck = true;
                }

                if (isKingInCheck)
                    tile.setFillColor(CHECK_COLOR);
                else if (isSelected)
                    tile.setFillColor(HIGHLIGHT);
                else if ((r + c) % 2 == 0)
                    tile.setFillColor(LIGHT_SQUARE);
                else
                    tile.setFillColor(DARK_SQUARE);

                window.draw(tile);

                // Legal move dot
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

                char piece = board.getBoard(r, c);
                if (piece != '.') {
                    sf::Sprite sprite(textures[piece]);
                    float scale = (float)tileSize / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);
                    sprite.setPosition(boardOffX + c * tileSize, boardOffY + r * tileSize);
                    window.draw(sprite);
                }
            }
        }

        // Rank / file labels
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

        // Side panel
        if (fontLoaded) {
            sf::RectangleShape panel(sf::Vector2f(panelW, windowH));
            panel.setFillColor(sf::Color(20, 20, 20));
            panel.setPosition(0, 0);
            window.draw(panel);

            int titleSz = std::max(14, tileSize / 4);
            int moveSz  = std::max(12, tileSize / 5);

            sf::Text title("Move History", font, titleSz);
            title.setFillColor(sf::Color(220, 220, 220));
            title.setPosition(10, 10);
            window.draw(title);

            auto history = board.getMoveHistory();
            int yOff = titleSz + 20;
            int maxY  = windowH - btnH * 4 - 20;
            for (int i = 0; i < (int)history.size() && yOff < maxY; i += 2) {
                std::string line = std::to_string(i / 2 + 1) + ". " + history[i];
                if (i + 1 < (int)history.size()) line += "  " + history[i + 1];
                sf::Text mv(line, font, moveSz);
                mv.setFillColor(sf::Color(180, 180, 180));
                mv.setPosition(10, yOff);
                window.draw(mv);
                yOff += moveSz + 6;
            }

            // Turn indicator
            sf::Text turnTxt(board.getTurn() == 1 ? "White's turn" : "Black's turn",
                             font, moveSz + 2);
            turnTxt.setFillColor(board.getTurn() == 1
                ? sf::Color::White : sf::Color(150, 150, 150));
            turnTxt.setPosition(10, windowH - btnH * 4 - 10);
            window.draw(turnTxt);

            // Resign / Draw buttons
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

            // Confirm overlay
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

            // Escape hint
            sf::Text hint("ESC - exit", font, std::max(10, tileSize / 6));
            hint.setFillColor(sf::Color(80, 80, 80));
            hint.setPosition(10, windowH - 20);
            window.draw(hint);
        }

        // Promotion overlay
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

                // Promoting side is the previous turn (turn has already advanced)
                int promoTurn = (board.getTurn() == 1) ? 2 : 1;
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

        // Game over overlay
        if (gameOver && fontLoaded) {
            sf::RectangleShape overlay(sf::Vector2f(windowW, windowH));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            int goSz = std::max(24, tileSize / 2);
            sf::Text txt(gameResult, font, goSz);
            txt.setFillColor(sf::Color::White);
            sf::FloatRect b = txt.getLocalBounds();
            txt.setPosition((windowW - b.width) / 2, windowH / 2 - goSz);
            window.draw(txt);
        }

        window.display();
    }

    return 0;
}
