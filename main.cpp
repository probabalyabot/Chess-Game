#include <iostream>
#include <map>

using namespace std;
static int moveCounter = 0;


char board[8][8] = {
    {'r', 'n', 'b', 'q', 'k', 'b', 'n', 'r'},
    {'p', 'p', 'p', 'p', 'p', 'p', 'p', 'p'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'.', '.', '.', '.', '.', '.', '.', '.'},
    {'P', 'P', 'P', 'P', 'P', 'P', 'P', 'P'},
    {'R', 'N', 'B', 'Q', 'K', 'B', 'N', 'R'}
};
map<char, int> fileMap = {
    {'a', 0},
    {'b', 1},
    {'c', 2},
    {'d', 3},
    {'e', 4},
    {'f', 5},
    {'g', 6},
    {'h', 7}
};
void makeMove(string from, string to) {
    if (from == to){
        cout << "Invalid move: source and destination are the same." << endl;
        return;
    }

    // Convert file letters to column numbers
    int fromCol = fileMap[from[0]];
    int toCol = fileMap[to[0]];

    // Convert chess ranks to board rows
    int fromRow = 8 - (from[1] - '0');//subtract 0 as ascii value to get the actual number, and subtract 8 to reverse order of rows
    int toRow = 8 - (to[1] - '0');
    
    cout << from << " -> " << to << endl;

    board[toRow][toCol] = board[fromRow][fromCol];

    board[fromRow][fromCol] = '.';

}
void printBoard() {
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            cout << board[i][j] << " ";
        }
        cout << endl;
    }
}
void GetTurn(int counter){
    if(counter % 2 == 0){
        cout << "White's turn" << endl;
    }
    else{
        cout << "Black's turn" << endl;



int main(){
    cout<< "Welcome to the chess game!" << endl;
    printBoard();

    while (true){
        cout << "Turn: " << (GetTurn(moveCounter)) << endl;
        string from, to;
        cout << "Enter your move (e.g., e2 e4): ";
        cin >> from >> to;
        makeMove(from, to);
        printBoard();
        moveCounter++;
    }
}


