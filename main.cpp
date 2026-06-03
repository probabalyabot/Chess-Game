#include "chessboard.h"
#include <iostream>
#include <cctype>

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

            
            if (board.isInCheck(board.getTurn()))
                std::cout << (board.getTurn() == 1 ? "White" : "Black") << " is in check!\n";

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
            if (board.needsPromotion()){
            char choice;

            std::cout << "Pawn promotion! Choose piece (Q/R/B/N): ";

            while (true) {
                std::cin >> choice;
                choice = std::toupper(choice);

                if (choice == 'Q' ||
                    choice == 'R' ||
                    choice == 'B' ||
                    choice == 'N')
                    break;

                std::cout << "Invalid choice. Please enter Q, R, B, or N: ";
            }

            board.promotePawn(choice);
        }
        } catch (const ChessException& e) {
            std::cout << "Error: " << e.what() << "\n";
            continue;
        }

    
        if (board.isInCheck(board.getTurn()))
            std::cout << (board.getTurn() == 1 ? "White" : "Black") << " is in check!\n";

        
        std::string result;
        if (board.checkGameOver(result)) {
            board.printBoard();
            std::cout << result << "\n";
            board.printMoveHistory();
            break;
        }

        std::cout << "\nFEN: " << board.toFEN() << "\n";
        std::cout << "Use FEN string to load this position in other chess software\n";
    }

    return 0;
}