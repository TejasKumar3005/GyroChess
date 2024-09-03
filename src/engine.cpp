#include <algorithm>
#include <random>
#include <iostream>
#include <climits>
#include <unordered_map>
#include "engine.hpp"

// use namespace std
using namespace std;

struct evalweights {
    float king;
    float bishop;
    float rook;
    float pawn;
    float isCheck;
    float totalValidMoves;
    float kingValidMoves;
    float pawnsToPromotion;
    float boardControl;
    float pieceSynergy;
};


constexpr U8 cw_90[64] = {
    48, 40, 32, 24, 16, 8,  0,  7,
    49, 41, 33, 25, 17, 9,  1,  15,
    50, 42, 18, 19, 20, 10, 2,  23,
    51, 43, 26, 27, 28, 11, 3,  31,
    52, 44, 34, 35, 36, 12, 4,  39,
    53, 45, 37, 29, 21, 13, 5,  47,
    54, 46, 38, 30, 22, 14, 6,  55,
    56, 57, 58, 59, 60, 61, 62, 63
};

constexpr U8 acw_90[64] = {
     6, 14, 22, 30, 38, 46, 54, 7,
     5, 13, 21, 29, 37, 45, 53, 15,
     4, 12, 18, 19, 20, 44, 52, 23,
     3, 11, 26, 27, 28, 43, 51, 31,
     2, 10, 34, 35, 36, 42, 50, 39,
     1,  9, 17, 25, 33, 41, 49, 47,
     0,  8, 16, 24, 32, 40, 48, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};

constexpr U8 cw_180[64] = {
    54, 53, 52, 51, 50, 49, 48, 7,
    46, 45, 44, 43, 42, 41, 40, 15,
    38, 37, 18, 19, 20, 33, 32, 23,
    30, 29, 26, 27, 28, 25, 24, 31,
    22, 21, 34, 35, 36, 17, 16, 39,
    14, 13, 12, 11, 10,  9,  8, 47,
     6,  5,  4,  3,  2,  1,  0, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};

constexpr U8 id[64] = {
     0,  1,  2,  3,  4,  5,  6,  7,
     8,  9, 10, 11, 12, 13, 14, 15,
    16, 17, 18, 19, 20, 21, 22, 23,
    24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55,
    56, 57, 58, 59, 60, 61, 62, 63
};
#define color(p) ((PlayerColor)(p & (WHITE | BLACK)))


std::unordered_set<U16> Transform_moves(const std::unordered_set<U16>& moves, const U8 *transform) {

    std::unordered_set<U16> rot_moves;

    for (U16 move : moves) {
        rot_moves.insert(move_promo(transform[getp0(move)], transform[getp1(move)], getpromo(move)));
    }

    return rot_moves;
}


std::unordered_set<U16> Construct_bottom_rook_moves_with_board(const U8 p0, const U8* board) {

    int left_rook_reflect[7] = {0, 8, 16, 24, 32, 40, 48};
    U8 color = WHITE & BLACK;
    std::unordered_set<U16> rook_moves;
    bool refl_blocked = false;

    if (p0 < 8 || p0 == 13) {
        if (!(board[p0+pos(0,1)] & color)) rook_moves.insert(move(p0, p0+pos(0,1))); // top
        if (p0 == 1) { // top continued on the edge
            for (int y = 1; y<=6; y++) {
                U8 p1 = pos(1, y);
                if (board[p1]) {
                    if (board[p1] & color) break;         // our piece
                    else rook_moves.insert(move(p0, p1)); // their piece - capture
                    break;
                }
                else rook_moves.insert(move(p0, p1));
            }
        }
    }
    if (p0 >= 8) {
        if (!(board[p0-pos(0,1)] & color)) rook_moves.insert(move(p0, p0-pos(0,1))); // bottom
    }

    if (p0 != 6) {
        if (!(board[p0+pos(1,0)] & color)) rook_moves.insert(move(p0, p0+pos(1,0))); // right
    }

    for (int x=getx(p0)-1; x>=0; x--) {
        U8 p1 = pos(x, gety(p0));
        if (board[p1]) {
            refl_blocked = true;
            if (board[p1] & color) break;         // our piece
            else rook_moves.insert(move(p0, p1)); // their piece - capture
            break;
        }
        else {
            rook_moves.insert(move(p0, p1));
        }
    }

    if (refl_blocked) return rook_moves;
    
    if (p0 < 8) {
        for (int p1 : left_rook_reflect) {
            if (board[p1]) {
                if (board[p1] & color) break;         // our piece
                else rook_moves.insert(move(p0, p1)); // their piece - capture
                break;
            }
            else {
                rook_moves.insert(move(p0, p1));
            }
        }
    }

    return rook_moves;
}

std::unordered_set<U16> Construct_bottom_bishop_moves_with_board(const U8 p0, const U8* board) {

    U8 color = WHITE & BLACK;
    std::unordered_set<U16> bishop_moves;

    // top right - move back
    if (p0 < 6 || p0 == 13) {
        if (!(board[p0+pos(0,1)+pos(1,0)] & color)) bishop_moves.insert(move(p0, p0+pos(0,1)+pos(1,0)));
    }
    // bottom right - move back
    if (p0 > 6) {
        if (!(board[p0-pos(0,1)+pos(1,0)] & color)) bishop_moves.insert(move(p0, p0-pos(0,1)+pos(1,0)));
    }

    std::vector<U8> p1s;
    std::vector<U8> p1s_2;

    // top left - forward / reflections
    if (p0 == 1) {
        p1s.push_back(pos(0,1));
        p1s.push_back(pos(1,2));
    }
    else if (p0 == 2) {
        p1s.push_back(pos(1,1));
        p1s.push_back(pos(0,2));
        p1s.push_back(pos(1,3));
    }
    else if (p0 == 3) {
        p1s.push_back(pos(2,1));
        p1s.push_back(pos(1,2));
        p1s.push_back(pos(0,3));
        p1s.push_back(pos(1,4));
        p1s.push_back(pos(2,5));
        p1s.push_back(pos(3,6));
    }
    else if (p0 == 4 || p0 == 5) {
        p1s.push_back(p0+pos(0,1)-pos(1,0));
        p1s.push_back(p0-pos(2,0));
    }
    else if (p0 == 6) {
        p1s.push_back(pos(5,1));
    }
    else if (p0 == 10) {
        p1s_2.push_back(pos(1,0));
        p1s_2.push_back(pos(0,1));

        p1s.push_back(pos(1,2));
        p1s.push_back(pos(0,3));
        p1s.push_back(pos(1,4));
        p1s.push_back(pos(2,5));
        p1s.push_back(pos(3,6));
    }
    else if (p0 == 11) {
        p1s.push_back(pos(2,0));
        p1s.push_back(pos(1,1));
        p1s.push_back(pos(0,2));
    }
    else if (p0 == 12) {
        p1s.push_back(pos(3,0));
        p1s.push_back(pos(2,1));
        p1s.push_back(pos(1,2));
        p1s.push_back(pos(0,3));
    }
    else if (p0 == 13) {
        p1s.push_back(pos(4,0));
        p1s.push_back(pos(3,1));
    }

    for (auto p1 : p1s) {
        if (board[p1]) {
            if (board[p1] & color) break;           // our piece
            else bishop_moves.insert(move(p0, p1)); // their piece - capture
            break;
        }
        else {
            bishop_moves.insert(move(p0, p1));
        }
    }

    for (auto p1 : p1s_2) {
        if (board[p1]) {
            if (board[p1] & color) break;           // our piece
            else bishop_moves.insert(move(p0, p1)); // their piece - capture
            break;
        }
        else {
            bishop_moves.insert(move(p0, p1));
        }
    }

    return bishop_moves;
}

std::unordered_set<U16> Construct_bottom_pawn_moves_with_board(const U8 p0, const U8 *board, bool promote = false) {
    
    U8 color = WHITE & BLACK;
    std::unordered_set<U16> pawn_moves;

    if (!(board[pos(getx(p0)-1,0)] & color)) {
        if (promote) {
            pawn_moves.insert(move_promo(p0, pos(getx(p0)-1,0), PAWN_ROOK));
            pawn_moves.insert(move_promo(p0, pos(getx(p0)-1,0), PAWN_BISHOP));
        }
        else {
            pawn_moves.insert(move(p0, pos(getx(p0)-1,0)));
        }
    }
    if (!(board[pos(getx(p0)-1,1)] & color)) {
        if (promote) {
            pawn_moves.insert(move_promo(p0, pos(getx(p0)-1,1), PAWN_ROOK));
            pawn_moves.insert(move_promo(p0, pos(getx(p0)-1,1), PAWN_BISHOP));
        }
        else {
            pawn_moves.insert(move(p0, pos(getx(p0)-1,1)));
        }
    }
    if (p0 == 10 && !(board[17] & color)) pawn_moves.insert(move(p0, 17));

    return pawn_moves;
}

std::unordered_set<U16> Construct_bottom_king_moves_with_board(const U8 p0, const U8 *board) {

    // king can't move into check. See if these squares are under threat from 
    // enemy pieces as well.
    
    U8 color = WHITE & BLACK;
    std::unordered_set<U16> king_moves;
    if (!(board[pos(getx(p0)-1,0)] & color)) king_moves.insert(move(p0, pos(getx(p0)-1,0)));
    if (!(board[pos(getx(p0)-1,1)] & color)) king_moves.insert(move(p0, pos(getx(p0)-1,1)));
    if (p0 == 10 && !(board[pos(getx(p0)-1,2)] & color)) king_moves.insert(move(p0, pos(getx(p0)-1,2)));
    if (p0 != 6 && !(board[pos(getx(p0)+1,0)] & color)) king_moves.insert(move(p0, pos(getx(p0)+1,0)));
    if (p0 != 6 && !(board[pos(getx(p0)+1,1)] & color)) king_moves.insert(move(p0, pos(getx(p0)+1,1)));
    if (p0 >= 12 && !(board[pos(getx(p0)+1,2)] & color)) king_moves.insert(move(p0, pos(getx(p0)+1,2)));
    if (p0 == 13 && !(board[pos(getx(p0),2)] & color)) king_moves.insert(move(p0, pos(getx(p0),2)));
    if (!(board[pos(getx(p0),gety(p0)^1)] & color)) king_moves.insert(move(p0, pos(getx(p0),gety(p0)^1)));
    cout << "king moves: " << king_moves.size() << endl;
    return king_moves;
}


std::unordered_set<U16> get_pseudolegal_moves_for_piece(U8 piece_pos, Board b) {

    std::unordered_set<U16> moves;
    U8 piece_id = b.data.board_0[piece_pos];

    std::unordered_set<U8> bottom({ 1, 2, 3, 4, 5, 6, 10, 11, 12, 13 });
    std::unordered_set<U8> left({ 0, 8, 16, 24, 32, 40, 9, 17, 25, 33 });
    std::unordered_set<U8> top({ 48, 49, 50, 51, 52, 53, 41, 42, 43, 44 });
    std::unordered_set<U8> right({ 54, 46, 38, 30, 22, 14, 45, 37, 29, 21 });

    const U8 *board = b.data.board_0;
    const U8 *coord_map = id;
    const U8 *inv_coord_map = id;
    if      (left.count(piece_pos))  { board = b.data.board_270;  coord_map = acw_90; inv_coord_map = cw_90;  }
    else if (top.count(piece_pos))   { board = b.data.board_180; coord_map = cw_180; inv_coord_map = cw_180; }
    else if (right.count(piece_pos)) { board = b.data.board_90; coord_map = cw_90;  inv_coord_map = acw_90; }

    if (piece_id & PAWN) {
        if (((piece_pos == 51 || piece_pos == 43) && (piece_id & WHITE)) || 
            ((piece_pos == 11 || piece_pos == 3)  && (piece_id & BLACK)) ) {
            moves = Construct_bottom_pawn_moves_with_board(coord_map[piece_pos], board, true);
        }
        else {
            moves = Construct_bottom_pawn_moves_with_board(coord_map[piece_pos], board);
        }
    }
    else if (piece_id & ROOK) {
        moves = Construct_bottom_rook_moves_with_board(coord_map[piece_pos], board);
    }
    else if (piece_id & BISHOP) {
        moves = Construct_bottom_bishop_moves_with_board(coord_map[piece_pos], board);
    }
    else if (piece_id & KING) {
        moves = Construct_bottom_king_moves_with_board(coord_map[piece_pos], board);
    }

    moves = Transform_moves(moves, inv_coord_map);

    return moves;
}

    U8 b_rook_ws  = pos(2,5);
    U8 b_rook_bs  = pos(2,6);
    U8 b_king     = pos(3,5);
    U8 b_bishop   = pos(3,6);
    U8 b_pawn_ws  = pos(4,5);
    U8 b_pawn_bs  = pos(4,6);

    U8 w_rook_ws  = pos(4,1);
    U8 w_rook_bs  = pos(4,0);
    U8 w_king     = pos(3,1);
    U8 w_bishop   = pos(3,0);
    U8 w_pawn_ws  = pos(2,1);
    U8 w_pawn_bs  = pos(2,0);

// Constructor
EvalStats::EvalStats()
{
    this->board = Board();
    // set as if new board
    this->kings[0] = 1;
    this->kings[1] = 1;
    this->bishops[0] = 1;
    this->bishops[1] = 1;
    this->rooks[0] = 2;
    this->rooks[1] = 2;
    this->pawns[0] = 2;
    this->pawns[1] = 2;
    this->isCheck[0] = false;
    this->isCheck[1] = false;
    this->totalValidMoves[0] = 18;
    this->totalValidMoves[1] = 18;
    this->kingValidMoves[0] = 0;
    this->kingValidMoves[1] = 0;
    this->pawnsToPromotion[0] = 20;
    this->pawnsToPromotion[1] = 20;
    this->boardControl[0] = 0;
    this->boardControl[1] = 0;
    this->pieceSynergy[0] = 11;
    this->pieceSynergy[1] = 11;
    this->positionsAttackedBy = std::unordered_map<std::string, std::unordered_set<U8>>(
        {{"w_king", std::unordered_set<U8>({
            pos(3,0),
            pos(4,1),
            pos(2,1),
            pos(2,0),
            pos(4,0),
        })},
        {"w_bishop", std::unordered_set<U8>({
            pos(4,1),
            pos(2,1),
        })},
        {"w_rook_ws", std::unordered_set<U8>({
            pos(5,1),
            pos(4,0),
            pos(3,1)
        })},
        {"w_rook_bs", std::unordered_set<U8>({
            pos(4,1),
            pos(3,0),
            pos(5,0)
        })},
        {"w_pawn_ws", std::unordered_set<U8>({
            pos(1,0),
            pos(1,1),
            pos(1,2)
        })},
        {"w_pawn_bs", std::unordered_set<U8>({
            pos(1,0),
            pos(1,1)
        })},
        {"b_king", std::unordered_set<U8>({
            pos(2,5),
            pos(4,5),
            pos(3,6),
            pos(2,6),
            pos(4,6)
        })},
        {"b_bishop", std::unordered_set<U8>({
            pos(4,5),
            pos(2,5)
        })},
        {"b_rook_ws", std::unordered_set<U8>({
            pos(1,5),
            pos(2,6),
            pos(3,5)
        })},
        {"b_rook_bs", std::unordered_set<U8>({
            pos(2,5),
            pos(3,6),
            pos(1,6)
        })},
        {"b_pawn_ws", std::unordered_set<U8>({
            pos(5,6),
            pos(5,5),
            pos(5,4)
        })},
        {"b_pawn_bs", std::unordered_set<U8>({
            pos(5,6),
            pos(5,5)
        })}}
    );
    this->positionUnderAttackBy = std::unordered_map<U8, std::unordered_set<AttackInfo, AttackInfoHash>>({
    {pos(3,0), {{"w_king", w_king}, {"w_rook_bs", w_rook_bs}}},
    {pos(4,1), {{"w_king", w_king}, {"w_bishop", w_bishop}, {"w_rook_bs", w_rook_bs}}},
    {pos(2,1), {{"w_king", w_king}, {"w_bishop", w_bishop}}},
    {pos(2,0), {{"w_king", w_king}}},
    {pos(4,0), {{"w_king", w_king}, {"w_rook_ws", w_rook_ws}}},
    {pos(5,1), {{"w_rook_ws", w_rook_ws}}},
    {pos(3,1), {{"w_rook_ws", w_rook_ws}}},
    {pos(1,0), {{"w_pawn_ws", w_pawn_ws}, {"w_pawn_bs", w_pawn_bs}}},
    {pos(1,1), {{"w_pawn_ws", w_pawn_ws}, {"w_pawn_bs", w_pawn_bs}}},
    {pos(1,2), {{"w_pawn_ws", w_pawn_ws}}},
    {pos(5,0), {{"w_rook_bs", w_rook_bs}}},

    {pos(2,5), {{"b_king", b_king}, {"b_bishop", b_bishop}, {"b_rook_bs", b_rook_bs}}},
    {pos(4,5), {{"b_king", b_king}, {"b_bishop", b_bishop}}},
    {pos(3,6), {{"b_king", b_king}, {"b_rook_bs", b_rook_bs}}},
    {pos(2,6), {{"b_king", b_king}, {"b_rook_ws", b_rook_ws}}},
    {pos(4,6), {{"b_king", b_king}}},
    {pos(1,5), {{"b_rook_ws", b_rook_ws}}},
    {pos(1,6), {{"b_rook_bs", b_rook_bs}}},
    {pos(5,6), {{"b_pawn_ws", b_pawn_ws}, {"b_pawn_bs", b_pawn_bs}}},
    {pos(5,5), {{"b_pawn_ws", b_pawn_ws}, {"b_pawn_bs", b_pawn_bs}}},
    {pos(5,4), {{"b_pawn_ws", b_pawn_ws}}},
    {pos(3,5), {{"b_rook_ws", b_rook_ws}}},

    {pos(0,0), {}},
    {pos(0,1), {}},
    {pos(0,2), {}},
    {pos(0,3), {}},
    {pos(0,4), {}},
    {pos(0,5), {}},
    {pos(0,6), {}},

    {pos(1,3), {}},
    {pos(1,4), {}},

    {pos(5,2), {}},
    {pos(5,3), {}},

    {pos(6,0), {}},
    {pos(6,1), {}},
    {pos(6,2), {}},
    {pos(6,3), {}},
    {pos(6,4), {}},
    {pos(6,5), {}},
    {pos(6,6), {}},

});




}

std::unordered_set<U8> getp1ofmoves(std::unordered_set<U16> moves){
    std::unordered_set<U8> p1ofmoves;
    for (auto mov: moves){
        U8 p1 = getp1(mov);
        p1ofmoves.insert(p1);
    }
    return p1ofmoves;
}


// Update the statistics after a move is made
int manhattanDistance(U8 pos1, U8 pos2)
{
    // Define hole boundaries
    const int holeLeft = 2, holeRight = 4, holeTop = 2, holeBottom = 4;

    int x1 = getx(pos1);
    int y1 = gety(pos1);
    int x2 = getx(pos2);
    int y2 = gety(pos2);

    // Check if a position is in the hole
    auto inHole = [holeLeft, holeRight, holeTop, holeBottom](int x, int y)
    {
        return x >= holeLeft && x <= holeRight && y >= holeTop && y <= holeBottom;
    };

    // If either position is in the hole, return a large distance (or -1)
    if (inHole(x1, y1) || inHole(x2, y2))
        return -1; // or any large number

    // Determine quadrant for a position
    auto getQuadrant = [holeLeft, holeRight, holeTop, holeBottom](int x, int y)
    {
        if (x < holeLeft && y < holeTop)
            return 1;
        if (x >= holeLeft && x <= holeRight && y < holeTop)
            return 2;
        if (x > holeRight && y < holeTop)
            return 3;
        if (x < holeLeft && y >= holeTop && y <= holeBottom)
            return 4;
        if (x > holeRight && y >= holeTop && y <= holeBottom)
            return 5;
        if (x < holeLeft && y > holeBottom)
            return 6;
        if (x >= holeLeft && x <= holeRight && y > holeBottom)
            return 7;
        if (x > holeRight && y > holeBottom)
            return 8;
        return 0;
    };

    int quadrant1 = getQuadrant(x1, y1);
    int quadrant2 = getQuadrant(x2, y2);

    // If the points are in the special quadrants, calculate distance accordingly
    if ((quadrant1 == 2 && quadrant2 == 7) || (quadrant1 == 7 && quadrant2 == 2))
    {
        return abs(x1 - x2) + abs(y1 - y2) + 2;
    }

    if ((quadrant1 == 4 && quadrant2 == 5) || (quadrant1 == 5 && quadrant2 == 4))
    {
        return abs(x1 - x2) + abs(y1 - y2) + 2;
    }

    // Return normal Manhattan distance for the rest
    return abs(x1 - x2) + abs(y1 - y2);
}


int getPawnPromotionDist(U8 pos, int i)
{
    if (i == 0)
    {
        U8 goal;
        if (gety(pos) == 6)
            goal = pos(4, 6);
        else
            goal = pos(4, 5);
        return manhattanDistance(pos, goal);
    }
    else
    {
        U8 goal;
        if (gety(pos) == 0)
            goal = pos(4, 0);
        else
            goal = pos(4, 1);
        return manhattanDistance(pos, goal);
    }
}

void printBinary(uint8_t num) {
    for(int i = 7; i >= 0; --i) {
        std::cout << ((num & (1 << i)) ? '1' : '0');
    }
}

EvalStats* EvalStats::aftermove(U16 mov)
{
    std::cout << "aftermove started" << endl;
    cout << "mov: " << move_to_str(mov) << endl;
    cout << "Board" << endl;
    cout << all_boards_to_str(this->board) << endl;
    U8 p0 = getp0(mov);
    U8 p1 = getp1(mov);
    U8 promo = getpromo(mov);

    U8 peice = this->board.data.board_0[p0];
    U8 killedPeice = this->board.data.board_0[p1];

    std::string peiceName = "";
    std::string killedPeiceName = "";
    U8 color_op = 0;
    U8 color_same = 0;
    int index;
    cout << peice << endl;
    std::cout << "naming peice" << endl;
    if (peice & WHITE)
    {
        color_op = BLACK;
        color_same = WHITE;
        index = 0;
        if (peice & KING)
        {
            peiceName = "w_king";
        }
        else if (peice & BISHOP)
        {
            if (p0 == this->board.data.w_bishop)
            {
                peiceName = "w_bishop";
            }
            else if (p0 == this->board.data.w_pawn_bs)
            {
                peiceName = "w_pawn_bs";
            }
            else if (p0 == this->board.data.w_pawn_ws)
            {
                peiceName = "w_pawn_ws";
            }
        }
        else if (peice & ROOK)
        {
            if (p0 == this->board.data.w_rook_ws)
            {
                peiceName = "w_rook_ws";
            }
            else if (p0 == this->board.data.w_rook_bs)
            {
                peiceName = "w_rook_bs";
            }
            else if (p0 == this->board.data.w_pawn_bs)
            {
                peiceName = "w_pawn_bs";
            }
            else if (p0 == this->board.data.w_pawn_ws)
            {
                peiceName = "w_pawn_ws";
            }
        }
        else if (peice & PAWN)
        {
            if (p0 == this->board.data.w_pawn_ws)
            {
                peiceName = "w_pawn_ws";
            }
            else if (p0 == this->board.data.w_pawn_bs)
            {
                peiceName = "w_pawn_bs";
            }
        }
    }
    else if (peice & BLACK)
    {
        color_op = WHITE;
        color_same = BLACK;
        index = 1;
        if (peice & KING)
        {
            peiceName = "b_king";
        }
        else if (peice & BISHOP)
        {
            if (p0 == this->board.data.b_bishop)
            {
                peiceName = "b_bishop";
            }
            else if (p0 == this->board.data.b_pawn_bs)
            {
                peiceName = "b_pawn_bs";
            }
            else if (p0 == this->board.data.b_pawn_ws)
            {
                peiceName = "b_pawn_ws";
            }
        }
        else if (peice & ROOK)
        {
            if (p0 == this->board.data.b_rook_ws)
            {
                peiceName = "b_rook_ws";
            }
            else if (p0 == this->board.data.b_rook_bs)
            {
                peiceName = "b_rook_bs";
            }
            else if (p0 == this->board.data.b_pawn_bs)
            {
                peiceName = "b_pawn_bs";
            }
            else if (p0 == this->board.data.b_pawn_ws)
            {
                peiceName = "b_pawn_ws";
            }
        }
        else if (peice & PAWN)
        {
            if (p0 == this->board.data.b_pawn_ws)
            {
                peiceName = "b_pawn_ws";
            }
            else if (p0 == this->board.data.b_pawn_bs)
            {
                peiceName = "b_pawn_bs";
            }
        }
    }

    std::cout << "naming killed peice" << endl;
    if (killedPeice & WHITE)
    {
        if (killedPeice & KING)
        {
            killedPeiceName = "w_king";
        }
        else if (killedPeice & BISHOP)
        {
            if (p1 == this->board.data.w_bishop)
            {
                killedPeiceName = "w_bishop";
            }
            else if (p1 == this->board.data.w_pawn_bs)
            {
                killedPeiceName = "w_pawn_bs";
            }
            else if (p1 == this->board.data.w_pawn_ws)
            {
                killedPeiceName = "w_pawn_ws";
            }
        }
        else if (killedPeice & ROOK)
        {
            if (p1 == this->board.data.w_rook_ws)
            {
                killedPeiceName = "w_rook_ws";
            }
            else if (p1 == this->board.data.w_rook_bs)
            {
                killedPeiceName = "w_rook_bs";
            }
            else if (p1 == this->board.data.w_pawn_bs)
            {
                killedPeiceName = "w_pawn_bs";
            }
            else if (p1 == this->board.data.w_pawn_ws)
            {
                killedPeiceName = "w_pawn_ws";
            }
        }
        else if (killedPeice & PAWN)
        {
            if (p1 == this->board.data.w_pawn_ws)
            {
                killedPeiceName = "w_pawn_ws";
            }
            else if (p1 == this->board.data.w_pawn_bs)
            {
                killedPeiceName = "w_pawn_bs";
            }
        }
    }
    else if (killedPeice & BLACK)
    {
        cout << "killed peice is black" << endl;
        printBinary(killedPeice);
        printBinary(BISHOP);
        cout << endl;
        if (killedPeice & KING)
        {
            cout << "killed peice is black king" << endl;
            killedPeiceName = "b_king";
        }
        else if (killedPeice & BISHOP)
        {
            cout << "killed peice is black bishop" << endl;
            printBinary(this->board.data.b_bishop);
            cout << endl;
            if (p1 == this->board.data.b_bishop)
            {
                cout << "killed peice is black bishop" << endl;
                killedPeiceName = "b_bishop";
            }
            else if (p1 == this->board.data.b_pawn_bs)
            {
                cout << "killed peice is black pawn bs promoted to bishop" << endl;
                killedPeiceName = "b_pawn_bs";
            }
            else if (p1 == this->board.data.b_pawn_ws)
            {
                cout << "killed peice is black pawn ws promoted to bishop" << endl;
                killedPeiceName = "b_pawn_ws";
            }
        }
        else if (killedPeice & ROOK)
        {
            if (p1 == this->board.data.b_rook_ws)
            {
                cout << "killed peice is black rook ws" << endl;
                killedPeiceName = "b_rook_ws";
            }
            else if (p1 == this->board.data.b_rook_bs)
            {
                cout << "killed peice is black rook bs" << endl;
                killedPeiceName = "b_rook_bs";
            }
            else if (p1 == this->board.data.b_pawn_bs)
            {
                cout << "killed peice is black pawn bs promoted to rook" << endl;
                killedPeiceName = "b_pawn_bs";
            }
            else if (p1 == this->board.data.b_pawn_ws)
            {
                cout << "killed peice is black pawn ws promoted to rook" << endl;
                killedPeiceName = "b_pawn_ws";
            }
        }
        else if (killedPeice & PAWN)
        {
            if (p1 == this->board.data.b_pawn_ws)
            {
                cout << "killed peice is black pawn ws" << endl;
                killedPeiceName = "b_pawn_ws";
            }
            else if (p1 == this->board.data.b_pawn_bs)
            {
                cout << "killed peice is black pawn bs" << endl;
                killedPeiceName = "b_pawn_bs";
            }
        }
    }

    // handle number of peices
    std::cout << "handling number of peices" << endl;
    if (killedPeice & WHITE)
    {
        std::cout << "killed peice is white" << endl;
        if (killedPeice & KING)
        {
            this->kings[0] -= 1;
        }
        else if (killedPeice & BISHOP)
        {
            this->bishops[0] -= 1;
        }
        else if (killedPeice & ROOK)
        {
            this->rooks[0] -= 1;
        }
        else if (killedPeice & PAWN)
        {
            this->pawns[0] -= 1;
            // this->pawnsToPromotion[0] -= getPawnPromotionDist(p1, 0);
        }
        std::cout << 1<< endl;
        for (auto pos : this->positionsAttackedBy[killedPeiceName])
        {   
            cout << "erasing " << killedPeiceName << " from " << endl;
            printBinary(pos);
            cout << endl;
            cout << this->positionUnderAttackBy[pos].size() << endl;
            this->positionUnderAttackBy[pos].erase(std::pair(killedPeiceName, p1));
            cout << this->positionUnderAttackBy[pos].size() << endl;
            if (this->board.data.board_0[pos] & color_op)
            {
                this->boardControl[0] -= 1;
            }
            else if (this->board.data.board_0[pos] & color_same)
            {
                this->pieceSynergy[0] -= 1;
            }
        }
        std::cout << 2<< endl;
        this->totalValidMoves[0] -= this->positionsAttackedBy[killedPeiceName].size();
        this->positionsAttackedBy.erase(killedPeiceName);
    }
    else if (killedPeice & BLACK)
    {
        std::cout << "killed peice is black" << endl;
        if (killedPeice & KING)
        {
            this->kings[1] -= 1;
        }
        else if (killedPeice & BISHOP)
        {
            this->bishops[1] -= 1;
        }
        else if (killedPeice & ROOK)
        {
            this->rooks[1] -= 1;
        }
        else if (killedPeice & PAWN)
        {
            this->pawns[1] -= 1;
            // this->pawnsToPromotion[1] -= getPawnPromotionDist(p1, 1);
        }
        std::cout << 3<< endl;
        cout<< killedPeiceName << endl;
        cout << this->positionsAttackedBy[killedPeiceName].size() << endl;
        for (auto pos : this->positionsAttackedBy[killedPeiceName])
        {
            cout << "erasing " << killedPeiceName << " from " << endl;
            printBinary(pos);
            cout << endl;
            cout << this->positionUnderAttackBy[pos].size() << endl;
            this->positionUnderAttackBy[pos].erase(std::pair(killedPeiceName, p1));
            cout << this->positionUnderAttackBy[pos].size() << endl;
            if (this->board.data.board_0[pos] & color_op)
            {
                this->boardControl[1] -= 1;
            }
            else if (this->board.data.board_0[pos] & color_same)
            {
                this->pieceSynergy[1] -= 1;
            }
        }
        std::cout << 4<< endl;   
        cout << this->positionsAttackedBy[killedPeiceName].size() << endl;
        cout << this->positionUnderAttackBy[pos(4,5)].size() << endl;
        cout << this->positionUnderAttackBy[pos(2,5)].size() << endl;
        this->totalValidMoves[1] -= this->positionsAttackedBy[killedPeiceName].size();
        this->positionsAttackedBy.erase(killedPeiceName);
    }

    // remove peice from this->positionsAttackedBy
    cout << index << endl;
    cout << peiceName << endl;
    cout << this->totalValidMoves[index] << endl;
    cout << this->positionsAttackedBy[peiceName].size() << endl;
    this->totalValidMoves[index] -= this->positionsAttackedBy[peiceName].size();
    
    std::cout << 5<< endl;
    for (auto pos : this->positionsAttackedBy[peiceName])
    {
        cout << "erasing " << peiceName << " from " << endl;
        printBinary(pos);
        cout << endl;
        cout << this->positionUnderAttackBy[pos].size() << endl;
        this->positionUnderAttackBy[pos].erase(std::pair(peiceName, p0));
        cout << this->positionUnderAttackBy[pos].size() << endl;
        if (this->board.data.board_0[pos] & color_op)
        {
            this->boardControl[index] -= 1;
        }
        else if (this->board.data.board_0[pos] & color_same)
        {
            this->pieceSynergy[index] -= 1;
        }
    }
    this->positionsAttackedBy.erase(peiceName);
    std::cout << 6<< endl;

    

    // handle promotion
    if (promo)
    {
        if (promo & PAWN_BISHOP)
        {
            this->bishops[index] += 1;
            this->pawns[index] -= 1;
            peice = BISHOP | color_same;
        }
        else if (promo & PAWN_ROOK)
        {
            this->rooks[index] += 1;
            this->pawns[index] -= 1;
            peice = ROOK | color_same;
        }

        this->pawnsToPromotion[index] -= 1;
    }
    

    // now do changes for the move completion 
    this->board.do_move(mov);
    if (peice & PAWN) {
        
        if (peiceName == "w_pawn_ws"){
            U8 p = this->board.data.w_pawn_ws;
            bool neg = getx(p) < getx(p0); 
            if (( (!neg) && (color_same == WHITE)) || (neg && (color_same == BLACK))){
                this->pawnsToPromotion[index] -= manhattanDistance(p, p0);
            }
        }
    }
    Board* b = this->board.copy();
    std::cout << 7<< endl;
    std::cout << this->positionUnderAttackBy[p0].size() << endl;
    for (auto attacker : this->positionUnderAttackBy[p0])
    {
        U8 p_color = this->board.data.board_0[attacker.second] & (WHITE|BLACK);
        U8 op_color;
        int i;
        if (p_color == WHITE){
            op_color = BLACK;
            i = 0;
        }
        else if (p_color == BLACK){
            op_color = WHITE;
            i = 1;
        }
        std::unordered_set<U16> moves = get_pseudolegal_moves_for_piece(attacker.second, *b);
        if (this->board.data.board_0[attacker.second] & KING){
            
            std::cout << 8<< endl;
            for (auto attack : this->positionUnderAttackBy[p0])
            {
                if (attack.second & op_color)
                {
                    cout << "attack.second & op_color" << endl;
                    printBinary(attack.second);
                    printBinary(op_color);
                    U16 mov = move(attacker.second, p0);
                    moves.erase(mov);
                }
            }
            std::cout << 9<< endl;
        } 
        
        std::cout << 10<< endl;
        for (auto mov : moves){
            U8 p2 = getp1(mov);
            if (this->board.data.board_0[p2] & op_color){
                // if not in positionUnderAttackBy then add to boardControl
                if (this->positionUnderAttackBy[p2].find(attacker) == this->positionUnderAttackBy[p2].end()){
                    this->boardControl[i] += 1;
                }
            }
            else if (this->board.data.board_0[p2] & p_color){
                if (this->positionUnderAttackBy[p2].find(attacker) == this->positionUnderAttackBy[p2].end()){
                    this->pieceSynergy[i] += 1;
                }
            }
        }
        std::cout << 11<< endl;
        cout << "number of new moves "<<moves.size() << endl;
        cout << "number of old moves "<<this->positionsAttackedBy[attacker.first].size() << endl;

        this->totalValidMoves[i] += moves.size() - this->positionsAttackedBy[attacker.first].size() ;
        this->positionsAttackedBy.erase(attacker.first);
        this->positionsAttackedBy.insert(std::pair(attacker.first, getp1ofmoves(moves)));
    }
    std::cout << 12<< endl;
    std::unordered_set<U16> moves = get_pseudolegal_moves_for_piece(peice, *b);
    // delete b;
    if (peice & KING)
    {
        std::cout << 13<< endl;
        std::vector<decltype(mov)> toErase;
        for (auto mov: moves){
            cout << "checking king move" << mov << endl;
            U8 p2 = getp1(mov);
            // check if the move is valid
            std::cout << 14<< endl;
            for (auto attack : this->positionUnderAttackBy[p2])
            {
                if (attack.second  & color_op)
                {
                    toErase.push_back(mov);
                    break;
                }
            }
            std::cout << 15<< endl;
        }
        for (const auto &m : toErase) {
            moves.erase(m);
        }
        std::cout << 16<< endl;
    }
    std::cout << 17<< endl;
    for (auto mov : moves){
            U8 p2 = getp1(mov);
            if (this->board.data.board_0[p2] & color_op){
                // if not in positionUnderAttackBy then add to boardControl
                if (this->positionUnderAttackBy[p2].find(std::pair(peiceName, p0)) == this->positionUnderAttackBy[p2].end()){
                    this->boardControl[index] += 1;
                }
            }
            else if (this->board.data.board_0[p2] & color_same){
                if (this->positionUnderAttackBy[p2].find(std::pair(peiceName, p0)) == this->positionUnderAttackBy[p2].end()){
                    this->pieceSynergy[index] += 1;
                }
            }
        }
    std::cout << 18<< endl;
    printBinary(p1);
    cout << this->positionUnderAttackBy[p1].size() << endl;
    for (auto attacker : this->positionUnderAttackBy[p1])
    {
        cout<< "in" << endl;
        cout << attacker.first << endl;
        printBinary(attacker.second);
        printBinary(this->board.data.board_0[attacker.second]);
        U8 p_color = this->board.data.board_0[attacker.second] & (WHITE|BLACK);
        cout << p_color << endl;
        U8 op_color;
        int i;
        if (p_color == WHITE){
            cout << "white" << endl;
            op_color = BLACK;
            i = 0;
        }
        else if (p_color == BLACK){
            cout << "black" << endl;
            op_color = WHITE;
            i = 1;
        }
        std::unordered_set<U16> moves = get_pseudolegal_moves_for_piece(attacker.second, *b);
        

        cout << "seg fult" << endl;

        this->totalValidMoves[i] += moves.size() - this->positionsAttackedBy[attacker.first].size();
        std::cout << 19<< endl;
        for (auto p2 : this->positionsAttackedBy[attacker.first]){
            if (this->board.data.board_0[p2] & op_color){
                // if not in positionUnderAttackBy then add to boardControl
                    this->boardControl[i] -= 1;
                
            }
            else if (this->board.data.board_0[p2] & p_color){
                    this->pieceSynergy[i] -= 1;
                
            }   
        }
        std::cout << 20<< endl;
        this->positionsAttackedBy.erase(attacker.first);
        this->positionsAttackedBy.insert(std::pair(attacker.first, getp1ofmoves(moves)));
        for (auto p2 : this->positionsAttackedBy[attacker.first]){
            if (this->board.data.board_0[p2] & op_color){
                // if not in positionUnderAttackBy then add to boardControl
                    this->boardControl[i] += 1;
            }
            else if (this->board.data.board_0[p2] & p_color){
                    this->pieceSynergy[i] += 1;
                
            }   
        }
        std::cout << 21<< endl;
    }
    std::cout << 22<< endl;
    delete b;

    this->totalValidMoves[index] += moves.size();
    this->positionsAttackedBy.insert(std::pair(peiceName, getp1ofmoves(moves)));

    U8 king_w = this->board.data.w_king;
    U8 king_b = this->board.data.b_king;

    if (this->positionUnderAttackBy[king_w].size() > 0)
    {
        std::cout << 23<< endl; 
        for (auto attack : this->positionUnderAttackBy[king_w])
        {
            if (attack.second & BLACK)
            {
                this->isCheck[0] = true;
                break;
            }
            else
            {
                this->isCheck[0] = false;
            }
        }
        std::cout << 24<< endl;
    }
    else
    {
        this->isCheck[0] = false;
    }
    
    if (this->positionUnderAttackBy[king_b].size() > 0)
    {
        std::cout << 25<< endl;
        for (auto attack : this->positionUnderAttackBy[king_b])
        {
            if (attack.second & WHITE)
            {
                this->isCheck[1] = true;
                break;
            }
            else
            {
                this->isCheck[1] = false;
            }
        }
        std::cout << 26<< endl;
    }
    else
    {
        this->isCheck[1] = false;
    }

    this->kingValidMoves[0] = positionUnderAttackBy[king_w].size();
    this->kingValidMoves[1] = positionUnderAttackBy[king_b].size();

    EvalStats *e = new EvalStats();
    e->board = *this->board.copy();
    memcpy(&(e->kings), &(this->kings), sizeof(this->kings));
    memcpy(&(e->bishops), &(this->bishops), sizeof(this->bishops));
    memcpy(&(e->rooks), &(this->rooks), sizeof(this->rooks));
    memcpy(&(e->pawns), &(this->pawns), sizeof(this->pawns));
    memcpy(&(e->isCheck), &(this->isCheck), sizeof(this->isCheck));
    memcpy(&(e->totalValidMoves), &(this->totalValidMoves), sizeof(this->totalValidMoves));
    memcpy(&(e->kingValidMoves), &(this->kingValidMoves), sizeof(this->kingValidMoves));
    memcpy(&(e->pawnsToPromotion), &(this->pawnsToPromotion), sizeof(this->pawnsToPromotion));
    memcpy(&(e->boardControl), &(this->boardControl), sizeof(this->boardControl));
    memcpy(&(e->pieceSynergy), &(this->pieceSynergy), sizeof(this->pieceSynergy));
    e->positionsAttackedBy = this->positionsAttackedBy;
    e->positionUnderAttackBy = this->positionUnderAttackBy;

    return e;
}

EvalStats* EvalStats::copy(){
    EvalStats *e = new EvalStats();
    e->board = *this->board.copy();
    memcpy(&(e->kings), &(this->kings), sizeof(this->kings));
    memcpy(&(e->bishops), &(this->bishops), sizeof(this->bishops));
    memcpy(&(e->rooks), &(this->rooks), sizeof(this->rooks));
    memcpy(&(e->pawns), &(this->pawns), sizeof(this->pawns));
    memcpy(&(e->isCheck), &(this->isCheck), sizeof(this->isCheck));
    memcpy(&(e->totalValidMoves), &(this->totalValidMoves), sizeof(this->totalValidMoves));
    memcpy(&(e->kingValidMoves), &(this->kingValidMoves), sizeof(this->kingValidMoves));
    memcpy(&(e->pawnsToPromotion), &(this->pawnsToPromotion), sizeof(this->pawnsToPromotion));
    memcpy(&(e->boardControl), &(this->boardControl), sizeof(this->boardControl));
    memcpy(&(e->pieceSynergy), &(this->pieceSynergy), sizeof(this->pieceSynergy));
    e->positionsAttackedBy = this->positionsAttackedBy;
    e->positionUnderAttackBy = this->positionUnderAttackBy;


    return e;
}


evalweights weights = {10000.0, 50.0, 30.0, 10.0, 100.0, 5.0, 20.0, 3.0, 25.0, 25.0}; //weights for the evaluation function

// struct eval {
//     /* Heuristic parameters we are considering 
//         King,
//         Bishop
//         Rook,
//         Pawn,
//         isCheck,
//         Total num. of valid moves 
//         Total num. of valid moves for the king 
//         King Safety - How many neighbouring squares of the king are guarded 
//         Development - How many peices are there in their non original position 
//         Dominancy - If peices of one color dominate a quadrant while others are spread over throughout 
//         Pawn Promotion - How many pawns are close to promotion and by how many moves
//         Board control - How many pieces are attacking the other player's pieces
//         Corner control - How many corners are controlled by each player
//         Piece Synergy - How many pieces are protected by other pieces
//         */
//     int king[2]; //handled this
//     int bishop[2]; //handled this
//     int rook[2]; //handled this
//     int pawn[2]; //handled this
//     int total_moves[2];
//     int king_moves[2];
//     int king_safety[2];
//     int development[2];
// };

float getEvaluation(EvalStats eval_scor,U16 mov){

    cout << "getEvaluation" << endl;
    EvalStats eval_score = *eval_scor.aftermove(mov);
    cout << "aftermove done in eval" << endl;
    


    // for (int i = 0; i < 64; i ++){
    //     auto piece = b.data.board_0[i];
    //     if (piece & KING){
    //         if (piece & WHITE){
    //             evaluation.king[0] += 1;
    //         }else{
    //             evaluation.king[1] += 1;
    //         }
    //     } else if (piece & BISHOP){
    //         if (piece & WHITE){
    //             evaluation.bishop[0] += 1;
    //         }else{
    //             evaluation.bishop[1] += 1;
    //         }
    //     } else if (piece & ROOK){
    //         if (piece & WHITE){
    //             evaluation.rook[0] += 1;
    //         }else{
    //             evaluation.rook[1] += 1;
    //         }
    //     } else if (piece & PAWN){
    //         if (piece & WHITE){
    //             evaluation.pawn[0] += 1;
    //         }else{
    //             evaluation.pawn[1] += 1;
    //         }
    //     }
    // }
    
    float score;
    score = weights.king * (eval_score.kings[0] - eval_score.kings[1]) 
                + weights.bishop * (eval_score.bishops[0] - eval_score.bishops[1]) 
                + weights.rook * (eval_score.rooks[0] - eval_score.rooks[1]) 
                + weights.pawn * (eval_score.pawns[0] - eval_score.pawns[1])
                + weights.isCheck * (eval_score.isCheck[0] - eval_score.isCheck[1])
                + weights.totalValidMoves * (eval_score.totalValidMoves[0] - eval_score.pieceSynergy[0] - eval_score.totalValidMoves[1] + eval_score.pieceSynergy[1])
                + weights.kingValidMoves * (eval_score.kingValidMoves[0] - eval_score.kingValidMoves[1])
                + weights.pawnsToPromotion * (eval_score.pawnsToPromotion[0] - eval_score.pawnsToPromotion[1])
                + weights.boardControl * (eval_score.boardControl[0] - eval_score.boardControl[1])
                + weights.pieceSynergy * (eval_score.pieceSynergy[0] - eval_score.pieceSynergy[1])
                ;
    std::cout << "Score: " << score << std::endl;
    return score;
}

// int mini(int alpha, int beta, int depth, const Board& b);
int mini(int alpha, int beta, int depth, const Board& b, EvalStats& prev_stats, U16 prev_move);

int maxi(int alpha, int beta, int depth, const Board& b, EvalStats& prev_stats, U16 prev_move){
    std::cout << "maxi" << endl;
    cout << "depth: " << depth << endl;
    printBinary(b.data.b_bishop);
    printBinary(b.data.w_bishop);
    cout << prev_stats.positionsAttackedBy["w_king"].size() << endl;

    if (depth == 0) return getEvaluation(prev_stats, prev_move);
    auto moveset = b.get_legal_moves();
    if (moveset.size() == 0) return -INT_MAX;
    std::cout << "moveset size: " << moveset.size() << endl;
    EvalStats* stats =  prev_stats.aftermove(prev_move);
    for (auto mov : moveset){
        Board* tempBoard = b.copy();
        cout << "move: " << mov << endl;
        tempBoard->do_move(mov);
        EvalStats* st =  stats->copy();
        int score = mini(alpha, beta, depth-1, *tempBoard, *st, mov);
        delete tempBoard;
        delete st;
        if (score > alpha){
            cout << "alpha updated in maxi" << endl;
            alpha = score;
        }
        if (alpha >= beta){
            cout << "beta pruned in maxi" << endl;
            delete stats;
            // delete &prev_stats;
            return beta;
        }
    }
    cout << "returning alpha in maxi" << endl;
    delete stats;
    // delete &prev_stats;
    return alpha;


}

int mini(int alpha, int beta, int depth, const Board& b, EvalStats& prev_stats, U16 prev_move){
    std::cout << "mini" << endl;
    cout << "depth: " << depth << endl;
    printBinary(b.data.b_bishop);
    printBinary(b.data.w_bishop);
    cout << prev_stats.positionsAttackedBy["w_king"].size() << endl;
    if (depth == 0) {
        std::cout << "depth 0" << endl;
        return getEvaluation(prev_stats, prev_move);
        }

    auto moveset = b.get_legal_moves();
    if (moveset.size() == 0) return INT_MAX;
    std::cout << "moveset size: " << moveset.size() << endl;
    EvalStats* stats =  prev_stats.aftermove(prev_move);
    std::cout << "after move" << endl;
    for (auto mov : moveset){
        std::cout << "move: " << mov << endl;
        Board* tempBoard = b.copy();
        tempBoard->do_move(mov);
        EvalStats* st =  stats->copy();
        int score = maxi(alpha, beta, depth-1, *tempBoard, *st, mov);
        delete tempBoard;
        delete st;
        if (score < beta){
            std::cout << "beta updated in mini" << endl;
            beta = score;
        }
        if (alpha >= beta){
            std::cout << "alpha pruned in mini" << endl;
            delete stats;
            // delete &prev_stats;
            return alpha;
        }
    }
    std::cout << "returning beta in mini" << endl;
    delete stats;
    cout << "deleting prev_stats" << endl;
    // delete &prev_stats;
    cout << "deleted prev_stats" << endl;
    return beta;
}


U16 last_move(Board& b1, Board& b2){

    U8 w_king1 = b1.data.w_king;
    U8 w_king2 = b2.data.w_king;
    U8 b_king1 = b1.data.b_king;
    U8 b_king2 = b2.data.b_king;
    U8 w_rook_ws1 = b1.data.w_rook_ws;
    U8 w_rook_ws2 = b2.data.w_rook_ws;
    U8 w_rook_bs1 = b1.data.w_rook_bs;
    U8 w_rook_bs2 = b2.data.w_rook_bs;
    U8 b_rook_ws1 = b1.data.b_rook_ws;
    U8 b_rook_ws2 = b2.data.b_rook_ws;
    U8 b_rook_bs1 = b1.data.b_rook_bs;
    U8 b_rook_bs2 = b2.data.b_rook_bs;
    U8 w_bishop1 = b1.data.w_bishop;
    U8 w_bishop2 = b2.data.w_bishop;
    U8 b_bishop1 = b1.data.b_bishop;
    U8 b_bishop2 = b2.data.b_bishop;
    U8 w_pawn_ws1 = b1.data.w_pawn_ws;
    U8 w_pawn_ws2 = b2.data.w_pawn_ws;
    U8 w_pawn_bs1 = b1.data.w_pawn_bs;
    U8 w_pawn_bs2 = b2.data.w_pawn_bs;
    U8 b_pawn_ws1 = b1.data.b_pawn_ws;
    U8 b_pawn_ws2 = b2.data.b_pawn_ws;
    U8 b_pawn_bs1 = b1.data.b_pawn_bs;
    U8 b_pawn_bs2 = b2.data.b_pawn_bs;

    if (w_king1 != w_king2 && (w_king1 != DEAD ) && (w_king2 != DEAD)){
        return move(w_king2, w_king1);
    } else if (b_king1 != b_king2 && (b_king1 != DEAD ) && (b_king2 != DEAD)){
        return move(b_king2, b_king1);
    } else if (w_rook_ws1 != w_rook_ws2 && (w_rook_ws1 != DEAD ) && (w_rook_ws2 != DEAD)){
        return move(w_rook_ws2, w_rook_ws1);
    } else if (w_rook_bs1 != w_rook_bs2 && (w_rook_bs1 != DEAD ) && (w_rook_bs2 != DEAD)){
        return move(w_rook_bs2, w_rook_bs1);
    } else if (b_rook_ws1 != b_rook_ws2 && (b_rook_ws1 != DEAD ) && (b_rook_ws2 != DEAD)){
        return move(b_rook_ws2, b_rook_ws1);
    } else if (b_rook_bs1 != b_rook_bs2 && (b_rook_bs1 != DEAD ) && (b_rook_bs2 != DEAD)){
        return move(b_rook_bs2, b_rook_bs1);
    } else if (w_bishop1 != w_bishop2 && (w_bishop1 != DEAD ) && (w_bishop2 != DEAD)){
        return move(w_bishop2, w_bishop1);
    } else if (b_bishop1 != b_bishop2 && (b_bishop1 != DEAD ) && (b_bishop2 != DEAD)){
        return move(b_bishop2, b_bishop1);
    } else if (w_pawn_ws1 != w_pawn_ws2 && (w_pawn_ws1 != DEAD ) && (w_pawn_ws2 != DEAD)){
        return move(w_pawn_ws2, w_pawn_ws1);
    } else if (w_pawn_bs1 != w_pawn_bs2 && (w_pawn_bs1 != DEAD ) && (w_pawn_bs2 != DEAD)){
        return move(w_pawn_bs2, w_pawn_bs1);
    } else if (b_pawn_ws1 != b_pawn_ws2 && (b_pawn_ws1 != DEAD ) && (b_pawn_ws2 != DEAD)){
        return move(b_pawn_ws2, b_pawn_ws1);
    } else if (b_pawn_bs1 != b_pawn_bs2 && (b_pawn_bs1 != DEAD ) && (b_pawn_bs2 != DEAD)){
        return move(b_pawn_bs2, b_pawn_bs1);
    } else{
        return 0;
    }

}

void Engine::find_best_move(const Board& b) {
    std::cout << "Finding best move" << std::endl;
    
    printBinary(b.data.b_bishop);
    printBinary(b.data.w_bishop);
    auto moveset = b.get_legal_moves();
    if (moveset.size() == 0) {
        this->best_move = 0;
    }
    else {
        Board* tempB = b.copy();
        U16 last_mov = last_move(*tempB, this->lastBoard);
        std::cout << "Last move: " << last_mov << endl;
        EvalStats* eval_stats;
        if (last_mov == 0){
            eval_stats = new EvalStats();
        } else{
        eval_stats = this->lastEvalStats.aftermove(last_mov);
        }
        if (b.data.player_to_play == WHITE){
            std::cout << "White to play" << endl;
            int bestScore = -INT_MAX;
            for (auto moves: moveset){
                cout <<"looped" << endl;
                Board* tempBoard = b.copy() ;
                tempBoard->do_move(moves);
                EvalStats* e = eval_stats->copy();
                int score = mini(bestScore, INT_MAX, 3, *tempBoard, *e,  moves );

                if (score > bestScore){
                    bestScore = score;
                    this->best_move = moves;
                }
                delete e;
                delete tempBoard;
            }
        }else{
            int bestScore = INT_MAX;
            for (auto moves: moveset){
                Board* tempBoard = b.copy();
                tempBoard->do_move(moves);
                EvalStats* e = eval_stats->copy();
                int score = maxi(-INT_MAX, bestScore, 3, *tempBoard, *e, moves);
                if (score < bestScore){
                    bestScore = score;
                    this->best_move = moves;
                }
                delete e;
                delete tempBoard;
            }
        }
        EvalStats* e = eval_stats->aftermove(this->best_move);
        tempB->do_move(this->best_move);
        this->lastEvalStats = *e;
        this->lastBoard = *tempB;
        delete eval_stats;

    }
}

