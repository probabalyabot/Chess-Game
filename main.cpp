#include <iostream>
#include <map>
#include "C:\Codes\chess game\src\constants.h"

class chessboard{
    private:
        char board[8][8];
        int moveCounter = 1;
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
            for(int i =0;i<8;i++){
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
            if(moveCounter % 2 == 1){
                return 1;//white's turn
            }
            else{
                return 2;//black's turn
            }
          
        }
        void makeMove(std::string from, std::string to) {
            if (from == to){
                std::cout << "Invalid move: source and destination are the same." << std::endl;
                return;
            }

            // Convert file letters to column numbers
            int fromCol = fileMap[from[0]];
            int toCol = fileMap[to[0]];

            // Convert chess ranks to board rows
            int fromRow = 8 - (from[1] - '0');//subtract 0 as ascii value to get the actual number, and subtract 8 to reverse order of rows
            int toRow = 8 - (to[1] - '0');
    
            std::cout << from << " -> " << to << std::endl;

            board[toRow][toCol] = board[fromRow][fromCol];

            board[fromRow][fromCol] = '.';

        }
};



int main(){
    std::cout<< "Welcome to the chess game!" << std::endl;
    chessboard board;


    

 


