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
    {'K', 100}, {'k', -100}//black = negative 
};

class chessboard{
    private:
        char board[8][8];
        int moveCounter = 0;
        int enPassantRow = -1;
        int enPassantCol = -1;
        bool isPathClearRook(int fromRow, int fromCol, int toRow, int toCol){
            int rowStep = (toRow > fromRow) ? 1 : (toRow < fromRow) ? -1 : 0;
            int colStep = (toCol > fromCol) ? 1 : (toCol < fromCol) ? -1 : 0;
    
            int currentRow = fromRow + rowStep;
            int currentCol = fromCol + colStep;
    
            while(currentRow != toRow || currentCol != toCol){
                if(board[currentRow][currentCol] != '.') return false;
                currentRow += rowStep;
                currentCol += colStep;
            }
            return true;
        }
        bool isPathClearBishop(int fromRow, int fromCol, int toRow, int toCol){
            int rowStep = (toRow > fromRow) ? 1 : -1;
            int colStep = (toCol > fromCol) ? 1 : -1;
    
            int currentRow = fromRow + rowStep;
            int currentCol = fromCol + colStep;
    
            while(currentRow != toRow && currentCol != toCol){
                if(board[currentRow][currentCol] != '.') return false;
                currentRow += rowStep;
                currentCol += colStep;
            }
            return true;
        }

        bool isValidKnightMove(int fromRow, int fromCol, int toRow, int toCol){
            int rowDiff = abs(toRow - fromRow);
            int colDiff = abs(toCol - fromCol);
            return (rowDiff == 2 && colDiff == 1) || (rowDiff == 1 && colDiff == 2);// handes all L as general offset is +- 2 and 1 in any order
        }
        bool isValidBishopMove(int fromRow, int fromCol, int toRow, int toCol){
            if(!isPathClearBishop(fromRow, fromCol, toRow, toCol)) return false;
            return abs(toRow - fromRow) == abs(toCol - fromCol);// need to add path checking later
        }
        bool isValidRookMove(int fromRow, int fromCol, int toRow, int toCol){
            if(!isPathClearRook(fromRow, fromCol, toRow, toCol)) return false;
            return (fromRow == toRow || fromCol == toCol);
        }
        bool isValidQueenMove(int fromRow, int fromCol, int toRow, int toCol){
            return isValidRookMove(fromRow, fromCol, toRow, toCol) || isValidBishopMove(fromRow, fromCol, toRow, toCol);
        }
        bool isValidKingMove(int fromRow, int fromCol, int toRow, int toCol){
            return abs(toRow - fromRow) <= 1 && abs(toCol - fromCol) <= 1;
        }
        bool isValidMove(char piece, int fromRow, int fromCol, int toRow, int toCol){
            switch(piece){
                case 'P': case 'p': return isValidPawnMove(piece, fromRow, fromCol, toRow, toCol);
                case 'N': case 'n': return isValidKnightMove(fromRow, fromCol, toRow, toCol);
                case 'B': case 'b': return isValidBishopMove(fromRow, fromCol, toRow, toCol);
                case 'R': case 'r': return isValidRookMove(fromRow, fromCol, toRow, toCol);
                case 'Q': case 'q': return isValidQueenMove(fromRow, fromCol, toRow, toCol);
                case 'K': case 'k': return isValidKingMove(fromRow, fromCol, toRow, toCol);
                default: return false;
            }
        }
        bool isValidPawnMove(char piece, int fromRow, int fromCol, int toRow, int toCol){
            int direction = (piece == 'P') ? -1 : 1; // white moves up, black moves down
            if(fromCol == toCol){ // moving forward
                if(toRow - fromRow == direction && board[toRow][toCol] == '.'){
                    return true;
                }
                if((fromRow == 6 && piece == 'P') || (fromRow == 1 && piece == 'p')){ // initial double move
                    if(toRow - fromRow == 2 * direction && board[fromRow + direction][toCol] == '.' && board[toRow][toCol] == '.'){
                        return true;
                    }
                }
            }
            else if(abs(toCol - fromCol) == 1 && toRow - fromRow == direction){ // capturing
                if(board[toRow][toCol] != '.' && ((piece == 'P' && islower(board[toRow][toCol])) || (piece == 'p' && isupper(board[toRow][toCol])))){
                    return true;
                }
                // en passant
                if(toRow == enPassantRow && toCol == enPassantCol){
                    return true;
                }
            }
            return false;
        }

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
            int tonum = 8 - (to[1] - '0');// convert to 0-based index and invert row number

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
            char targetPiece = board[tonum][toletter];
            if(getTurn() == 1 && isupper(targetPiece)){
                std::cout << "You cannot capture your own piece." << std::endl;
                return;
            }
            if(getTurn() == 2 && islower(targetPiece)){
                std::cout << "You cannot capture your own piece." << std::endl;
                return;
            }
            if(!isValidMove(board[fromnum][fromletter], fromnum, fromletter, tonum, toletter)){
                std::cout << "Invalid move for this piece." << std::endl;
                return;
            }

            // reset en passant each move
            enPassantRow = enPassantCol = -1;

            // en passant and double push handling
            if(board[fromnum][fromletter] == 'P' || board[fromnum][fromletter] == 'p'){
                int direction = (board[fromnum][fromletter] == 'P') ? -1 : 1;
                if(tonum - fromnum == 2 * direction){
                    enPassantRow = fromnum + direction;
                    enPassantCol = fromletter;
                }
                // remove captured pawn on en passant
                if(abs(toletter - fromletter) == 1 && board[tonum][toletter] == '.'){
                    board[fromnum][toletter] = '.';
                }
            }

            board[tonum][toletter] = board[fromnum][fromletter];
            board[fromnum][fromletter] = '.';
            moveCounter++;

            // pawn promotion
            if((board[tonum][toletter] == 'P' && tonum == 0) || (board[tonum][toletter] == 'p' && tonum == 7)){
                char choice = 'Q';
                std::cout << "Pawn promotion! Choose piece (Q/R/B/N): ";
                std::cin >> choice;
                choice = toupper(choice);
                if(choice != 'Q' && choice != 'R' && choice != 'B' && choice != 'N') choice = 'Q';
                board[tonum][toletter] = (board[tonum][toletter] == 'P') ? toupper(choice) : tolower(choice);
            }
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