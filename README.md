# Chess

A terminal chess game written in C++.



## Controls

| Input | Action |
|-------|--------|
| `e2 e4` | Move a piece |
| `O-O` | Castle kingside |
| `O-O-O` | Castle queenside |
| `resign` | Concede the game |
| `draw` | Offer a draw |

## Rules supported

- All standard piece movement
- Castling (kingside and queenside)
- En passant
- Pawn promotion
- Check / checkmate / stalemate detection
- Fifty-move rule
- Threefold repetition
- Insufficient material draw

## FEN

The current position is printed as a FEN string after every move. You can load a position by pasting a FEN string at startup when prompted.

> **Note:** When copying a FEN string into chess.com or any other tool, make sure to copy the entire string — if even one character is missing/ or you have whitespace from the start or end it will be rejected. Copy directly from the terminal to avoid this.

