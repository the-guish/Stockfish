/*
  Stockfish, a UCI chess playing engine derived from Glaurung 2.1
  Copyright (C) 2004-2026 The Stockfish developers (see AUTHORS file)

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

#include "evaluate.h"

#include "club_eval.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>

#include "misc.h"
#include "nnue/network.h"
#include "nnue/nnue_misc.h"
#include "position.h"
#include "types.h"
#include "uci.h"
#include "nnue/nnue_accumulator.h"

namespace Stockfish {

// Evaluate is the evaluator for the outer world. It returns a static evaluation
// of the position from the point of view of the side to move. On this branch
// it is the club evaluation (club_eval.h): the network is loaded but never
// asked, and only the fifty-move damping and the tablebase clamp of the
// original wrapper are kept, so that the search is unchanged.
Value Eval::evaluate([[maybe_unused]] const Eval::NNUE::Network&     network,
                     const Position&                                  pos,
                     [[maybe_unused]] Eval::NNUE::AccumulatorStack&  accumulators,
                     [[maybe_unused]] Eval::NNUE::AccumulatorCaches& caches,
                     [[maybe_unused]] int                             optimism) {

    assert(!pos.checkers());

    int v = Club::evaluate(pos);

    // Damp down the evaluation linearly when shuffling
    v -= v * pos.rule50_count() / 199;

    // Guarantee evaluation does not hit the tablebase range
    v = std::clamp(v, VALUE_TB_LOSS_IN_MAX_PLY + 1, VALUE_TB_WIN_IN_MAX_PLY - 1);

    return v;
}

// Like evaluate(), but instead of returning a value, it returns
// a string (suitable for outputting to stdout) that contains the detailed
// descriptions and values of each evaluation term. Useful for debugging.
// Trace scores are from white's point of view. The club terms are printed
// in check as well, where the network's trace declined to, so the port can
// be checked on any position; the search itself never evaluates in check.
std::string Eval::trace(Position& pos, [[maybe_unused]] const Eval::NNUE::Network& network) {

    std::stringstream ss;
    ss << Club::trace(pos);

    int v = Club::evaluate(pos);
    v -= v * pos.rule50_count() / 199;
    v = pos.side_to_move() == WHITE ? v : -v;
    ss << "Final evaluation       " << std::showpos << v << std::noshowpos
       << " (white side, internal units: centipawns * " << Club::Scale
       << " / 100, damped by the fifty-move count)\n";

    return ss.str();
}

}  // namespace Stockfish
