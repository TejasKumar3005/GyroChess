#pragma once
// #include "board.hpp"
#include "board.hpp"
#include <atomic>
#include <algorithm>
#include <cstring>
#include <iostream>
#include <climits>
#include <unordered_map>
using AttackInfo = std::pair<std::string, U8>;
struct AttackInfoHash {
    std::size_t operator()(const AttackInfo& p) const {
        return std::hash<std::string>()(p.first);
    }
};


class EvalStats
{
public:
    // Basic statistics stored in arrays for both players: 0 for white, 1 for black.
    Board board;
    int kings[2];           // ezz
    int bishops[2];         // ezz
    int rooks[2];           // ezz
    int pawns[2];           // ezz
    bool isCheck[2];        // ezz
    int totalValidMoves[2]; // ezz through attacking of each piece, every square having what piece is getting attacked
    int kingValidMoves[2];  // ezz handle at last
    // int kingSafety[2];      // ezz
    // int development[2];
    // int dominancy[2]; //
    int pawnsToPromotion[2]; // How many moves away from promotion
    int boardControl[2];     //
    // int cornerControl[2];
    int pieceSynergy[2]; //

    // For this->pieceSynergy, store a count of pieces protecting each piece.
    // int pieceProtectionCounts[2][6]; // Assuming max 6 pieces per player.

    // Bidirectional mapping of which piece blocks another piece's movement
    // Each piece's position mapped to another piece's position that it blocks or is blocked by

    std::unordered_map<std::string, std::unordered_set<U8>> positionsAttackedBy;
    std::unordered_map<U8, std::unordered_set<AttackInfo, AttackInfoHash>> positionUnderAttackBy;

    EvalStats();

    EvalStats* copy();
    EvalStats* aftermove(U16 mov);
};

class Engine {

    public:
    std::atomic<U16> best_move;
    std::atomic<bool> search;
    Board lastBoard = Board();
    EvalStats lastEvalStats = EvalStats();


    virtual void find_best_move(const Board& b);
};
