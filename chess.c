//   U+2B1B        1110xxxx 10xxxxxx 10xxxxxx
//   0000 2B1B
//   0000 0010 1011 0001 1011 >> 11100010  10101100  10011011
//   E2 AC 9B

// U+2B1C         1110xxxx 10xxxxxx 10xxxxxx
// 0000 2B1C
// 0000 0010 1011 0001 1100 >> 11100010  10101100  10011100
// E2 AC 9C

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>
#include <windows.h>

#define SIZE 8

// typedef struct pieces {
//     char King[4];
//     char Queen[4];
//     char Rook[4];
//     char Knight[4];
//     char Bishop[4];
//     char Pawn[4];
// } Pieces;
// Pieces wpieces = {"♔", "♕", "♖", "♘", "♗", "♙"};
// Pieces bpieces = {"♚", "♛", "♜", "♞", "♝", "♟"};

typedef struct coord {
    int x;
    int y;
} Coord;

typedef struct board {
    char cell[SIZE][SIZE][4];
} Board;


int const tick_ms = 3000;


void init_board(Board *b) {
    char *wpieces_start[] = { "♖", "♘", "♗", "♕", "♔", "♗", "♘", "♖" };
    char *bpieces_start[] = { "♜", "♞", "♝", "♛", "♚", "♝", "♞", "♜" };

    memset(b, 0, sizeof(Board));

    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            char * piece = " ";
            switch (i) {
                case 0:
                    piece = bpieces_start[j];
                    break;
                case 1:
                    piece = "♟";
                    break;
                case 6:
                    piece = "♙";
                    break;
                case 7:
                    piece = wpieces_start[j];
                    break;
            }
            strncpy(b->cell[i][j], piece, 4);
        }
    }
}

int file_to_col(char file) {
    return file - 'a';
}

int rank_to_row(char rank) {
    return 8 - (rank - '0');
}

// Mock validation function. In a real engine, you would verify if a piece 
// can legally move to the destination according to chess rules.
bool is_legal_move(char *piece, Coord src, Coord *dst, Board *b) {
    // For this demonstration, we just check if the source matches the piece type
    // and let the loop locate it.
    return strcmp(b->cell[src.x][src.y], piece) == 0;
}

bool interpret_pgn_move(Board *b, char *pgn, bool is_white, Coord *src, Coord *dst) {
    int len = strlen(pgn);
    if (len < 2) return false;

    // Remove check (+) or checkmate (#) modifiers from the end
    char clean_pgn[10];
    int clean_len = 0;
    for (int i = 0; i < len && pgn[i] != '+' && pgn[i] != '#'; i++) {
        clean_pgn[clean_len++] = pgn[i];
    }
    clean_pgn[clean_len] = '\0';

    char piece = 'P'; // Default is Pawn
    int dest_idx = -1;
    
    // Disambiguation variables (e.g., 'R' in "Rad1" or '1' in "N1f3")
    char spec_file = 0; 
    char spec_rank = 0;

    // 1. Identify the piece type
    int start_idx = 0;
    if (isupper(clean_pgn[0])) {
        piece = clean_pgn[0];
        start_idx = 1;
    }

    // 2. Find the destination square (always the last file+rank pair)
    // Scan backwards to find the rank (digit) and file (letter)
    int r_idx = clean_len - 1;
    while (r_idx > start_idx && !isdigit(clean_pgn[r_idx])) r_idx--;
    int f_idx = r_idx - 1;

    if (f_idx < start_idx || !islower(clean_pgn[f_idx])) {
        return false; // Invalid notation format
    }

    dst->y = file_to_col(clean_pgn[f_idx]);
    dst->x = rank_to_row(clean_pgn[r_idx]);

    // 3. Process any middle disambiguation characters (e.g., "Nbd7" or "R1e4" or "Bxf7")
    for (int i = start_idx; i < f_idx; i++) {
        if (clean_pgn[i] == 'x') continue; // Skip capture indicator
        if (islower(clean_pgn[i])) spec_file = clean_pgn[i];
        if (isdigit(clean_pgn[i])) spec_rank = clean_pgn[i];
    }

    // 4. Search the board for the matching source piece
    char *target_piece;
    switch (piece) {
        case 'K':
            target_piece = is_white ? "♔" : "♚";
            break;
        case 'Q':
            target_piece = is_white ? "♕" : "♛";
            break;
        case 'B':
            target_piece = is_white ? "♗" : "♝";
            break;
        case 'N':
            target_piece = is_white ? "♘" : "♞";
            break;
        case 'R':
            target_piece = is_white ? "♖" : "♜";
            break;
        default:   
            target_piece = is_white ? "♙" : "♟";
            break;
    }

    bool found = false;
    for (int r = 0; r < SIZE; r++) {
        for (int c = 0; c < SIZE; c++) {
            if (strcmp(b->cell[r][c], target_piece) == 0) {
                // Apply optional disambiguation filters if provided in PGN
                if (spec_file && c != file_to_col(spec_file)) continue;
                if (spec_rank && r != rank_to_row(spec_rank)) continue;

                Coord potential_src = {r, c};
                if (is_legal_move(target_piece, potential_src, dst, b)) {
                    *src = potential_src;
                    found = true;
                    break;
                }
            }
        }
        if (found) break;
    }

    return found;
}

void update_board(Board *b, char *move, bool is_white) {
    Coord src, dst;
    interpret_pgn_move(b, move, is_white, &src, &dst);

    strncpy(b->cell[dst.x][dst.y], b->cell[src.x][src.y], 4);
    strncpy(b->cell[src.x][src.y], " ", 4);
}

void print_board(Board *b) {
    char *cell_starts[] = { "\x1b[48;2;238;238;210m", "\x1b[48;2;118;150;86m" };
    char cell_end[] = { "\x1b[0m" };
    char *cell_start = NULL;

    // Reset cursor to top-left to reduce flicker
    // TODO Improve this code to completely eliminate flicker
    COORD cursorPosition;
    cursorPosition.X = 0;
    cursorPosition.Y = 0;
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), cursorPosition);

    for (int i = 0; i < SIZE; i++) {
        // Print y-axis coordinates 8-1
        printf("%d ", 8-i);

        // Print columns of i-th row
        for (int j = 0; j < SIZE; j++) {
            cell_start = cell_starts[(i%2 + j) % 2];
            printf("%s", cell_start);
            printf("%s ", b->cell[i][j]);
            printf("%s", cell_end);
        }
        printf("\n");
    }

    // Print x-axis coordinates a-h
    printf("  ");
    for (int j = 0; j < SIZE; j++) {
        printf("%c ", 'a' + j);
    }
    printf("\n");
}

int main() {
    Board board;

    init_board(&board);
    print_board(&board);

    
    char *moves[] = {
        "e4", "c5", "c3", "Nf6", "e5", "Nd5", "d4", "Nc6", "Nf3", "cxd4", "cxd4", "e6", "a3", "d6", "Bd3", "Qa5+", "Bd2", "Qb6", "Nc3", "Nxc3", "Bxc3", "dxe5", "dxe5", "Be7", "O-O", "Bd7", "Nd2", "Qc7", "Qg4", "O-O-O", "Rfc1", "Kb8", "Qc4", "Rc8", "b4", "f6", "Nf3", "Qb6", "Qe4", "f5", "Qe1", "a6", "Rab1", "g5", "Nd2", "Nd4", "Qe3", "Rxc3", "Rxc3", "f4", "Qe1", "g4", "Ne4", "Bc6", "Nc5", "Ka7", "a4", "Bf3", "a5", "Qd8", "Bc4", "Bxc5", "bxc5", "Qh4", "gxf3", "gxf3", "Kh1", "Rg8", "Qe4", "Rg7", "Qxd4", "Qg5", "c6+", "Kb8", "c7+", "Rxc7", "Rg1", "Qh5", "Rg8+", "Rc8", "Qd6+", "Ka7"
    };
    int num_moves = sizeof(moves) / sizeof(char*);
    for (int i = 0; i < num_moves; i++) {
        Sleep(tick_ms);
        update_board(&board, moves[i], i%2 == 0);
        print_board(&board);
    }

    return 0;
}