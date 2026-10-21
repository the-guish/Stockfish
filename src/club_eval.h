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
