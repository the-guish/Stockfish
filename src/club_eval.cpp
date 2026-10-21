/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2025 The Stockfish developers (see AUTHORS file)

  Stockfish is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Stockfish is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

// The club evaluation. See club_eval.h.
//
// Every term follows scaccosplicatrice's club_terms.py. Every count is
// White's less Black's, times an integer weight. The phase of the game is
// the pieces other than pawns and kings, a knight or bishop 1, a rook 2, a
// queen 4, both sides together, 24 at most; a middlegame term is its
// weighed counts times the phase, divided by 24 with the fraction dropped
// toward zero, an ending term the same with 24 less the phase. The score is
// for White, and evaluate() turns it for the side to move. Nothing here
// needs an attack map: every term is read from the inventory.

#include "club_eval.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <sstream>

#include "bitboard.h"
#include "position.h"

namespace Stockfish {

namespace Club {

const char* const TermNames[TERM_NB] = {"material",        "bishop-pair",    "development",
                                        "early-queen",     "centre",         "king-shelter",
                                        "castling-rights", "pawn-structure", "passed-pawns",
                                        "open-files",      "king-activity"};

namespace {

constexpr int FullPhase = 24;

// Michniewski's values, by piece type
constexpr int MaterialValue[PIECE_TYPE_NB] = {0, 100, 320, 330, 500, 900, 0, 0};
constexpr int PhaseWeight[PIECE_TYPE_NB]   = {0, 0, 1, 1, 2, 4, 0, 0};

constexpr int BishopPairBonus         = 30;
constexpr int MinorAtHome             = -15;
constexpr int MinorAtHomeQueenOut     = -10;
constexpr int MinorInTheCentre        = 10;
constexpr int KnightOnTheRim          = -10;
constexpr int PawnInTheCentre         = 20;
constexpr int ShieldPawn              = 15;
constexpr int KingOnACentreFile       = -40;
constexpr int KingOffItsBackRank      = -20;
constexpr int EachFurtherRank         = -20;
constexpr int CastlingRight           = 10;
constexpr int DoubledPawn             = -15;
constexpr int IsolatedPawn            = -15;
constexpr int PassedPawnOnRank[RANK_NB] = {0, 10, 15, 25, 40, 65, 100, 0};  // by relative rank
constexpr int RookOnAnOpenFile        = 15;
constexpr int RookOnAHalfOpenFile     = 8;
constexpr int StepTowardTheCentre     = 10;

// The sixteen central squares, c3 to f6, and the four centre squares
constexpr Bitboard Central =
  (FileCBB | FileDBB | FileEBB | FileFBB) & (Rank3BB | Rank4BB | Rank5BB | Rank6BB);
constexpr Bitboard Centre = (FileDBB | FileEBB) & (Rank4BB | Rank5BB);

// numerator / denominator with the fraction dropped toward zero
int toward_zero(int numerator, int denominator) {
    int whole = std::abs(numerator) / denominator;
    return numerator < 0 ? -whole : whole;
}

// The files next to a square's file
constexpr Bitboard adjacent_files(Square s) {
    return shift(file_bb(s), EAST) | shift(file_bb(s), WEST);
}

// The squares ahead of a pawn on its file and the files next to it: where
// an enemy pawn stops it being passed
constexpr Bitboard passed_pawn_span(Color c, Square s) {
    Bitboard ahead = c == WHITE ? ~Rank1BB << 8 * relative_rank(WHITE, s)
                                : ~Rank8BB >> 8 * relative_rank(BLACK, s);
    return ahead & (file_bb(s) | adjacent_files(s));
}

int game_phase(const Position& pos) {
    int phase = 0;
    for (PieceType pt : {KNIGHT, BISHOP, ROOK, QUEEN})
        phase += PhaseWeight[pt] * popcount(pos.pieces(pt));
    return std::min(FullPhase, phase);
}

// A side's minor pieces still on the squares they started on
template<Color Us>
int minors_at_home(const Position& pos) {
    return bool(pos.pieces(Us, KNIGHT) & relative_square(Us, SQ_B1))
         + bool(pos.pieces(Us, KNIGHT) & relative_square(Us, SQ_G1))
         + bool(pos.pieces(Us, BISHOP) & relative_square(Us, SQ_C1))
         + bool(pos.pieces(Us, BISHOP) & relative_square(Us, SQ_F1));
}

// One side's whole counts, each term's weighed sum before the phase; the
// caller takes White's less Black's.
template<Color Us>
void count_side(const Position& pos, int* t) {

    constexpr Color Them = ~Us;

    const Bitboard ownPawns   = pos.pieces(Us, PAWN);
    const Bitboard theirPawns = pos.pieces(Them, PAWN);
    const Square   ksq        = pos.square<KING>(Us);

    // material: Michniewski's values
    t[MATERIAL] = 0;
    for (PieceType pt : {PAWN, KNIGHT, BISHOP, ROOK, QUEEN})
        t[MATERIAL] += MaterialValue[pt] * popcount(pos.pieces(Us, pt));

    // bishop-pair
    t[BISHOP_PAIR] = pos.count<BISHOP>(Us) >= 2 ? BishopPairBonus : 0;

    // development: minors at home (middlegame)
    int atHome     = minors_at_home<Us>(pos);
    t[DEVELOPMENT] = MinorAtHome * atHome;

    // early-queen: a queen out (one at least, none at home) costs per minor at home (middlegame)
    bool hasQueen    = pos.count<QUEEN>(Us) > 0;
    bool queenAtHome = pos.pieces(Us, QUEEN) & relative_square(Us, SQ_D1);
    t[EARLY_QUEEN]   = (hasQueen && !queenAtHome) ? MinorAtHomeQueenOut * atHome : 0;

    // centre: minors on the sixteen central squares, knights on the rim, pawns on the four centre squares
    t[CENTRE] = MinorInTheCentre * popcount(pos.pieces(Us, KNIGHT, BISHOP) & Central)
              + KnightOnTheRim * popcount(pos.pieces(Us, KNIGHT) & (FileABB | FileHBB))
              + PawnInTheCentre * popcount(ownPawns & Centre);

    // king-shelter (middlegame): each of the king's file and the two beside it
    // with an own pawn one or two ranks ahead, a king on a centre file, a
    // king off its back rank and each rank beyond the first
    Bitboard ahead = shift(rank_bb(ksq), pawn_push(Us));
    ahead |= shift(ahead, pawn_push(Us));
    int shield = 0;
    for (Bitboard f : {shift(file_bb(ksq), WEST), file_bb(ksq), shift(file_bb(ksq), EAST)})
        shield += bool(ownPawns & ahead & f);
    int ranksLeft   = relative_rank(Us, ksq);
    t[KING_SHELTER] = ShieldPawn * shield
                    + (file_of(ksq) >= FILE_C && file_of(ksq) <= FILE_F ? KingOnACentreFile : 0)
                    + (ranksLeft > 0 ? KingOffItsBackRank + EachFurtherRank * (ranksLeft - 1) : 0);

    // castling-rights (middlegame)
    t[CASTLING_RIGHTS] = CastlingRight
                       * (pos.can_castle(Us & KING_SIDE) + pos.can_castle(Us & QUEEN_SIDE));

    // pawn-structure: doubled and isolated
    int doubled = 0, isolated = 0;
    for (File f = FILE_A; f <= FILE_H; ++f)
    {
        int n = popcount(ownPawns & file_bb(f));
        if (n > 1)
            doubled += n - 1;
        if (n > 0 && !(ownPawns & adjacent_files(make_square(f, RANK_1))))
            isolated += n;
    }
    t[PAWN_STRUCTURE] = DoubledPawn * doubled + IsolatedPawn * isolated;

    // passed-pawns, by the rank counted from the pawn's own side
    t[PASSED_PAWNS] = 0;
    for (Bitboard b = ownPawns; b;)
    {
        Square s = pop_lsb(b);
        if (!(theirPawns & passed_pawn_span(Us, s)))
            t[PASSED_PAWNS] += PassedPawnOnRank[relative_rank(Us, s)];
    }

    // open-files: rooks on files with no own pawn, with or without an enemy pawn
    t[OPEN_FILES] = 0;
    for (Bitboard b = pos.pieces(Us, ROOK); b;)
    {
        Square s = pop_lsb(b);
        if (ownPawns & file_bb(s))
            continue;
        t[OPEN_FILES] += (theirPawns & file_bb(s)) ? RookOnAHalfOpenFile : RookOnAnOpenFile;
    }

    // king-activity (ending): 3 less the king's steps to the nearest centre square
    int steps = std::max(std::abs(2 * file_of(ksq) - 7), std::abs(2 * rank_of(ksq) - 7)) / 2;
    t[KING_ACTIVITY] = StepTowardTheCentre * (3 - steps);
}

}  // namespace


Scores score(const Position& pos) {

    int white[TERM_NB], black[TERM_NB];
    count_side<WHITE>(pos, white);
    count_side<BLACK>(pos, black);

    Scores scores;
    scores.phase = game_phase(pos);
    for (int i = 0; i < TERM_NB; ++i)
    {
        int difference = white[i] - black[i];
        if (i == DEVELOPMENT || i == EARLY_QUEEN || i == KING_SHELTER || i == CASTLING_RIGHTS)
            difference = toward_zero(difference * scores.phase, FullPhase);
        else if (i == KING_ACTIVITY)
            difference = toward_zero(difference * (FullPhase - scores.phase), FullPhase);
        scores.term[i] = difference;
    }
    return scores;
}


int score_for_white(const Position& pos) {
    Scores scores = score(pos);
    int    total  = 0;
    for (int i = 0; i < TERM_NB; ++i)
        total += scores.term[i];
    return total;
}


// The club's weights are centipawns with a pawn at 100, and Stockfish's
// search works in units where a pawn is PawnValue; the score is converted so
// a pawn is worth to the search what a pawn has always been worth to it. The
// UCI option "Club Scale" changes the factor, in percent: 100 hands the
// search the centipawns as they are.
int Scale = PawnValue;

Value evaluate(const Position& pos) {
    int v = score_for_white(pos) * Scale / 100;
    return Value(pos.side_to_move() == WHITE ? v : -v);
}


std::string trace(const Position& pos) {

    Scores            scores = score(pos);
    std::stringstream ss;

    ss << " Contributing terms for the club eval (scaccosplicatrice), phase " << scores.phase
       << " of 24:\n"
       << "+-----------------+-------------+\n"
       << "|       Term      | Centipawns  |\n"
       << "+-----------------+-------------+\n";

    int total = 0;
    for (int i = 0; i < TERM_NB; ++i)
    {
        total += scores.term[i];
        ss << "| " << std::left << std::setw(15) << TermNames[i] << std::right << " | "
           << std::setw(11) << scores.term[i] << " |\n";
    }
    ss << "+-----------------+-------------+\n"
       << "| " << std::left << std::setw(15) << "total" << std::right << " | " << std::setw(11)
       << total << " |\n"
       << "+-----------------+-------------+\n";

    ss << "\nClub evaluation        " << std::showpos << total << std::noshowpos
       << " (white side, centipawns)\n";
    return ss.str();
}

}  // namespace Club

}  // namespace Stockfish
