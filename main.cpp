#include <iostream>
#include <map>
#include <regex>
std::map<char, int> fileMap = {
    {'a', 0},
    {'b', 1},
    {'c', 2},
    {'d', 3},
    {'e', 4},
    {'f', 5},
    {'g', 6},
    {'h', 7}
};
std::map<char, float> pieceValues = {
    {'P', 1},   {'p', -1},
    {'N', 3},   {'n', -3},
    {'B', 4},   {'b', -4},
    {'R', 5},   {'r', -5},
    {'Q', 9},   {'q', -9},
    {'K', 100}, {'k', -100}
};

class chessboard{
    private:
        char board[8][8];
        int moveCounter = 0;
    public:
        chessboard(){
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
            for(int i = 0; i < 8; i++){
                for(int j = 0; j < 8; j++){
                    board[i][j] = initialBoard[i][j];
                }
            }
        };
        void printBoard(){
            for(int i = 0; i < 8; i++){
                for(int j = 0; j < 8; j++){
                    std::cout << board[i][j] << " ";
                }
                std::cout << std::endl;
            }
        };
        int getTurn(){
            if(moveCounter % 2 == 0){
                return 1; // white
            }
            else{
                return 2; // black
            }
        }
        void makeMove(std::string from, std::string to) {
            const std::regex movePattern("[a-h][1-8]");
            if(!std::regex_match(from, movePattern) || !std::regex_match(to, movePattern)){
                std::cout << "Invalid format." << std::endl;
                return;
            }
            if(from == to){
                std::cout << "Source and destination are the same." << std::endl;
                return;
            }
            int fromletter = fileMap[from[0]];
            int toletter = fileMap[to[0]];
            int fromnum = 8 - (from[1] - '0');
            int tonum = 8 - (to[1] - '0');

            if(board[fromnum][fromletter] == '.'){
                std::cout << "No piece on that square." << std::endl;
                return;
            }
            if(getTurn() == 1 && pieceValues[board[fromnum][fromletter]] < 0){
                std::cout << "It's white's turn. Please move a white piece." << std::endl;
                return;
            }
            if(getTurn() == 2 && pieceValues[board[fromnum][fromletter]] > 0){
                std::cout << "It's black's turn. Please move a black piece." << std::endl;
                return;
            }
            board[tonum][toletter] = board[fromnum][fromletter];
            board[fromnum][fromletter] = '.';
            moveCounter++;
        }
};

int main(){
    std::cout << "Welcome to the chess game!" << std::endl;
    chessboard board;
    bool checkmate = false;
    while(!checkmate){
        board.printBoard();
        std::string from, to;
        if(board.getTurn() == 1){
            std::cout << "White's turn. Enter move (e.g., e2 e4): ";
        }
        else{
            std::cout << "Black's turn. Enter move (e.g., e7 e5): ";
        }
        std::cin >> from >> to;
        board.makeMove(from, to);
    }
    return 0;
}