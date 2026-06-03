#include <SFML/Graphics.hpp>
#include "chessboard.h"
#include <map>
#include <string>
#include <iostream>

const int TILE_SIZE    = 80;
const int BOARD_OFFSET = 300;
const int WINDOW_W     = 1100;
const int WINDOW_H     = 670;

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
    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Chess");
    window.setFramerateLimit(60);

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

    int selectedRow = -1;
    int selectedCol = -1;
    std::vector<std::pair<int,int>> legalMoves;
    bool pendingPromo = false;
    bool gameOver = false;
    std::string gameResult;
    bool resignConfirm = false;
    bool drawOffered   = false;

    sf::Font font;
    bool fontLoaded = font.loadFromFile("assets/font.ttf");

    // Buttons
    sf::RectangleShape resignBtn(sf::Vector2f(120, 40));
    resignBtn.setFillColor(sf::Color(180, 50, 50));
    resignBtn.setPosition(20, WINDOW_H - 120);

    sf::RectangleShape drawBtn(sf::Vector2f(120, 40));
    drawBtn.setFillColor(sf::Color(80, 80, 180));
    drawBtn.setPosition(20, WINDOW_H - 60);

    sf::RectangleShape confirmYes(sf::Vector2f(55, 35));
    confirmYes.setFillColor(sf::Color(50, 150, 50));
    confirmYes.setPosition(20, WINDOW_H - 170);

    sf::RectangleShape confirmNo(sf::Vector2f(55, 35));
    confirmNo.setFillColor(sf::Color(180, 50, 50));
    confirmNo.setPosition(85, WINDOW_H - 170);

    while (window.isOpen()) {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (gameOver) continue;

            if (event.type == sf::Event::MouseButtonPressed &&
                event.mouseButton.button == sf::Mouse::Left) {

                int mx = event.mouseButton.x;
                int my = event.mouseButton.y;

                // Resign/draw confirm buttons
                if (resignConfirm || drawOffered) {
                    if (confirmYes.getGlobalBounds().contains(mx, my)) {
                        if (resignConfirm) {
                            gameResult = (board.getTurn() == 1 ? "White" : "Black");
                            gameResult += " resigns. ";
                            gameResult += (board.getTurn() == 1 ? "Black" : "White");
                            gameResult += " wins!";
                            gameOver = true;
                        } else {
                            gameResult = "Draw agreed!";
                            gameOver = true;
                        }
                        resignConfirm = drawOffered = false;
                        continue;
                    }
                    if (confirmNo.getGlobalBounds().contains(mx, my)) {
                        resignConfirm = drawOffered = false;
                        continue;
                    }
                }

                // Resign button
                if (resignBtn.getGlobalBounds().contains(mx, my)) {
                    resignConfirm = true;
                    drawOffered   = false;
                    continue;
                }

                // Draw button
                if (drawBtn.getGlobalBounds().contains(mx, my)) {
                    drawOffered   = true;
                    resignConfirm = false;
                    continue;
                }

                // Promotion picker
                if (pendingPromo) {
                    std::string promoOptions = "QRBN";
                    for (int i = 0; i < 4; i++) {
                        sf::FloatRect btn(BOARD_OFFSET + i * TILE_SIZE * 2, WINDOW_H / 2 - TILE_SIZE, TILE_SIZE * 2, TILE_SIZE * 2);
                        if (btn.contains(mx, my)) {
                            board.promotePawn(promoOptions[i]);
                            pendingPromo = false;
                            if (board.checkGameOver(gameResult))
                                gameOver = true;
                        }
                    }
                    continue;
                }

                // Board click
                int col = (mx - BOARD_OFFSET) / TILE_SIZE;
                int row = my / TILE_SIZE;

                if (col < 0 || col > 7 || row < 0 || row > 7) {
                    selectedRow = selectedCol = -1;
                    legalMoves.clear();
                    continue;
                }

                if (selectedRow == -1) {
                    char piece = board.getBoard(row, col);
                    int turn = board.getTurn();
                    bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                    if (piece != '.' && isOwn) {
                        selectedRow = row;
                        selectedCol = col;
                        legalMoves = board.getLegalMovesFor(row, col);
                    }
                } else {
                    bool isLegal = false;
                    for (auto& m : legalMoves)
                        if (m.first == row && m.second == col) { isLegal = true; break; }

                    if (isLegal) {
                        std::string from = "", to = "";
                        from += (char)('a' + selectedCol);
                        from += (char)('8' - selectedRow);
                        to   += (char)('a' + col);
                        to   += (char)('8' - row);

                        try {
                            board.makeMove(from, to);
                            if (board.needsPromotion()) {
                                pendingPromo = true;
                            } else {
                                if (board.checkGameOver(gameResult))
                                    gameOver = true;
                            }
                        } catch (const ChessException& e) {
                            std::cerr << "Move error: " << e.what() << "\n";
                        }
                    }

                    char piece = board.getBoard(row, col);
                    int turn = board.getTurn();
                    bool isOwn = (turn == 1) ? isupper(piece) : islower(piece);
                    if (!isLegal && piece != '.' && isOwn) {
                        selectedRow = row;
                        selectedCol = col;
                        legalMoves = board.getLegalMovesFor(row, col);
                    } else {
                        selectedRow = selectedCol = -1;
                        legalMoves.clear();
                    }
                }
            }
        }

        window.clear(sf::Color(30, 30, 30));

        // Draw board
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                sf::RectangleShape tile(sf::Vector2f(TILE_SIZE, TILE_SIZE));
                tile.setPosition(BOARD_OFFSET + c * TILE_SIZE, r * TILE_SIZE);

                bool isSelected = (r == selectedRow && c == selectedCol);
                bool isLegal    = false;
                for (auto& m : legalMoves)
                    if (m.first == r && m.second == c) { isLegal = true; break; }

                // Check highlight — find king position
                bool isKingInCheck = false;
                if (board.isInCheck(board.getTurn())) {
                    char kingChar = (board.getTurn() == 1) ? 'K' : 'k';
                    if (board.getBoard(r, c) == kingChar)
                        isKingInCheck = true;
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

                if (isLegal) {
                    sf::CircleShape dot(TILE_SIZE * 0.15f);
                    dot.setFillColor(LEGAL_MOVE);
                    dot.setOrigin(TILE_SIZE * 0.15f, TILE_SIZE * 0.15f);
                    dot.setPosition(
                        BOARD_OFFSET + c * TILE_SIZE + TILE_SIZE / 2,
                        r * TILE_SIZE + TILE_SIZE / 2
                    );
                    window.draw(dot);
                }

                char piece = board.getBoard(r, c);
                if (piece != '.') {
                    sf::Sprite sprite(textures[piece]);
                    float scale = (float)TILE_SIZE / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);
                    sprite.setPosition(BOARD_OFFSET + c * TILE_SIZE, r * TILE_SIZE);
                    window.draw(sprite);
                }
            }
        }

        // Rank/file labels
        if (fontLoaded) {
            for (int c = 0; c < 8; c++) {
                sf::Text label(std::string(1, 'a' + c), font, 16);
                label.setFillColor(sf::Color(200, 200, 200));
                label.setPosition(BOARD_OFFSET + c * TILE_SIZE + TILE_SIZE / 2 - 5, 8 * TILE_SIZE + 5);
                window.draw(label);
            }
            for (int r = 0; r < 8; r++) {
                sf::Text label(std::string(1, '8' - r), font, 16);
                label.setFillColor(sf::Color(200, 200, 200));
                label.setPosition(BOARD_OFFSET - 20, r * TILE_SIZE + TILE_SIZE / 2 - 8);
                window.draw(label);
            }
        }

        // Move history panel
        if (fontLoaded) {
            sf::RectangleShape panel(sf::Vector2f(260, WINDOW_H));
            panel.setFillColor(sf::Color(20, 20, 20));
            panel.setPosition(0, 0);
            window.draw(panel);

            sf::Text title("Move History", font, 18);
            title.setFillColor(sf::Color(220, 220, 220));
            title.setPosition(10, 10);
            window.draw(title);

            auto history = board.getMoveHistory();
            int yOffset = 40;
            for (int i = 0; i < (int)history.size() && yOffset < WINDOW_H - 140; i += 2) {
                std::string line = std::to_string(i / 2 + 1) + ". " + history[i];
                if (i + 1 < (int)history.size()) line += "  " + history[i + 1];
                sf::Text moveText(line, font, 14);
                moveText.setFillColor(sf::Color(180, 180, 180));
                moveText.setPosition(10, yOffset);
                window.draw(moveText);
                yOffset += 22;
            }

            // Turn indicator
            sf::Text turnText(board.getTurn() == 1 ? "White's turn" : "Black's turn", font, 16);
            turnText.setFillColor(board.getTurn() == 1 ? sf::Color::White : sf::Color(150, 150, 150));
            turnText.setPosition(10, WINDOW_H - 140);
            window.draw(turnText);

            // Resign button
            window.draw(resignBtn);
            sf::Text resignText("Resign", font, 16);
            resignText.setFillColor(sf::Color::White);
            resignText.setPosition(35, WINDOW_H - 115);
            window.draw(resignText);

            // Draw button
            window.draw(drawBtn);
            sf::Text drawText("Offer Draw", font, 16);
            drawText.setFillColor(sf::Color::White);
            drawText.setPosition(22, WINDOW_H - 55);
            window.draw(drawText);

            // Confirm prompt
            if (resignConfirm || drawOffered) {
                sf::Text prompt(resignConfirm ? "Confirm resign?" : "Accept draw?", font, 14);
                prompt.setFillColor(sf::Color::Yellow);
                prompt.setPosition(10, WINDOW_H - 190);
                window.draw(prompt);

                window.draw(confirmYes);
                sf::Text yesText("Yes", font, 14);
                yesText.setFillColor(sf::Color::White);
                yesText.setPosition(30, WINDOW_H - 163);
                window.draw(yesText);

                window.draw(confirmNo);
                sf::Text noText("No", font, 14);
                noText.setFillColor(sf::Color::White);
                noText.setPosition(100, WINDOW_H - 163);
                window.draw(noText);
            }
        }

        // Promotion overlay
        if (pendingPromo) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            std::string promoOptions = "QRBN";
            for (int i = 0; i < 4; i++) {
                sf::RectangleShape btn(sf::Vector2f(TILE_SIZE * 2, TILE_SIZE * 2));
                btn.setPosition(BOARD_OFFSET + i * TILE_SIZE * 2, WINDOW_H / 2 - TILE_SIZE);
                btn.setFillColor(sf::Color(255, 255, 255));
                window.draw(btn);

                char pc = (board.getTurn() == 2) ? toupper(promoOptions[i]) : tolower(promoOptions[i]);
                if (textures.count(pc)) {
                    sf::Sprite sprite(textures[pc]);
                    float scale = (float)(TILE_SIZE * 2) / sprite.getLocalBounds().width;
                    sprite.setScale(scale, scale);
                    sprite.setPosition(BOARD_OFFSET + i * TILE_SIZE * 2, WINDOW_H / 2 - TILE_SIZE);
                    window.draw(sprite);
                }
            }
        }

        // Game over overlay
        if (gameOver && fontLoaded) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(0, 0, 0, 150));
            window.draw(overlay);

            sf::Text text(gameResult, font, 32);
            text.setFillColor(sf::Color::White);
            sf::FloatRect bounds = text.getLocalBounds();
            text.setPosition((WINDOW_W - bounds.width) / 2, WINDOW_H / 2 - 20);
            window.draw(text);
        }

        window.display();
    }

    return 0;
}
