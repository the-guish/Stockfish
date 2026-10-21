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

// The club evaluation: scaccosplicatrice's `club` evaluator in place of the
// network. Eleven terms a club player counts, with Michniewski's values:
// material, the bishop pair, development, the early queen, the centre, the
// king's shelter, castling rights, pawn structure, passed pawns, open files
// and the king's activity; four of them count in the middlegame and one in
// the ending, by the phase of the game. The definitions are those of
// scaccosplicatrice/engines/shared/club_terms.py, term for term, and the
// port was first made against Stockfish 15.1 (the forking-fomalhaut
// experiment of that project).

#ifndef CLUB_EVAL_H_INCLUDED
#define CLUB_EVAL_H_INCLUDED

#include <string>
#include <vector>

#include "types.h"

namespace Stockfish {

class Position;

namespace Club {

// The eleven terms, in the evaluator's order.
enum Term {
    MATERIAL,
    BISHOP_PAIR,
    DEVELOPMENT,
    EARLY_QUEEN,
    CENTRE,
    KING_SHELTER,
    CASTLING_RIGHTS,
    PAWN_STRUCTURE,
    PASSED_PAWNS,
    OPEN_FILES,
    KING_ACTIVITY,
    TERM_NB
};

extern const char* const TermNames[TERM_NB];

// The 27 weights, each settable through a UCI option named "Club <Weight>",
// 0 dropping the count it weighs; and one switch per term, "Club Term
// <term>", false dropping the whole term. Both are for trimming experiments:
// the defaults are the club's.
struct Weights {
    int pawn               = 100;
    int knight             = 320;
    int bishop             = 330;
    int rook               = 500;
    int queen              = 900;
    int bishopPair         = 30;
    int minorAtHome        = -15;
    int minorAtHomeQueenOut = -10;
    int minorInTheCentre   = 10;
    int knightOnTheRim     = -10;
    int pawnInTheCentre    = 20;
    int shieldPawn         = 15;
    int kingOnACentreFile  = -40;
    int kingOffItsBackRank = -20;
    int eachFurtherRank    = -20;
    int castlingRight      = 10;
    int doubledPawn        = -15;
    int isolatedPawn       = -15;
    int passedPawnOnRank2  = 10;
    int passedPawnOnRank3  = 15;
    int passedPawnOnRank4  = 25;
    int passedPawnOnRank5  = 40;
    int passedPawnOnRank6  = 65;
    int passedPawnOnRank7  = 100;
    int rookOnAnOpenFile   = 15;
    int rookOnAHalfOpenFile = 8;
    int stepTowardTheCentre = 10;
};

extern Weights W;
extern bool    Enabled[TERM_NB];

struct WeightOption {
    const char* name;   // the UCI option, "Club Pawn"
    int*        value;  // the weight it sets
    int         def;    // the club's value
};

// Every weight with its option name and default, in the order of Weights.
const std::vector<WeightOption>& weight_options();

// The UCI option of each term's switch, "Club Term Material".
extern const char* const TermOptionNames[TERM_NB];

struct Scores {
    int term[TERM_NB];  // each term's centipawns for White
    int phase;          // the phase of the game, 24 at the start
};

// Every term's score for White, in centipawns.
Scores score(const Position& pos);

// The sum of the terms: the score for White in centipawns.
int score_for_white(const Position& pos);

// The score for the side to move in Stockfish's internal units: the
// centipawns times Scale / 100. Scale is PawnValue by default and the UCI
// option "Club Scale" sets it.
extern int Scale;
Value      evaluate(const Position& pos);

// The terms and the score, one line per term, for the `eval` command.
std::string trace(const Position& pos);

}  // namespace Club

}  // namespace Stockfish

#endif  // #ifndef CLUB_EVAL_H_INCLUDED
