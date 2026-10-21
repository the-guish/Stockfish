# Manguruyu

Stockfish 19 with its network evaluation replaced by the scaccosplicatrice `club` evaluation,
eleven terms a club player counts with Michniewski's values, and every weight of it a UCI
option, so that a harness can play the evaluation with any weight dropped and measure what it
was worth. Named after the Paraná's largest catfish. The branch `club-evaluation` is the same
engine without the options; `manguruyu` adds them.

## The evaluation

`src/club_eval.cpp`, a term-for-term port of `scaccosplicatrice/engines/shared/club_terms.py`,
checked against it on the club terms' test positions and 1,300 random positions. The search is
Stockfish 19's, untouched: `Eval::evaluate` keeps its signature and returns the club score for
the side to move in internal units (centipawns times `Club Scale` / 100, 208 by default, so a
club pawn is a Stockfish pawn), with the fifty-move damping and the tablebase clamp of the
original. The nets are still built in and loaded, and never evaluated; the engine says so at
the first search (`info string club evaluation (scaccosplicatrice) enabled`). `eval` prints the
eleven terms for White with the phase, the total, and the weights that differ from the club's.

| Term | Weights (UCI option, default) |
|---|---|
| material | Club Pawn 100, Club Knight 320, Club Bishop 330, Club Rook 500, Club Queen 900 |
| bishop-pair | Club Bishop Pair 30 |
| development (middlegame) | Club Minor At Home -15 |
| early-queen (middlegame) | Club Minor At Home Queen Out -10 |
| centre | Club Minor In The Centre 10, Club Knight On The Rim -10, Club Pawn In The Centre 20 |
| king-shelter (middlegame) | Club Shield Pawn 15, Club King On A Centre File -40, Club King Off Its Back Rank -20, Club Each Further Rank -20 |
| castling-rights (middlegame) | Club Castling Right 10 |
| pawn-structure | Club Doubled Pawn -15, Club Isolated Pawn -15 |
| passed-pawns | Club Passed Pawn Rank 2 to Rank 7: 10, 15, 25, 40, 65, 100 |
| open-files | Club Rook On An Open File 15, Club Rook On A Half Open File 8 |
| king-activity (ending) | Club Step Toward The Centre 10 |

Each weight is a spin option from -1000 to 1000; 0 drops the count it weighs. Each term has a
switch, `Club Term Material` to `Club Term King Activity`, a check option true by default; false
drops the whole term, however many weights it has. The middlegame terms are scaled by the phase
of the game and the ending term by what is left of it, as in the Python.

```
setoption name Club Knight value 0            # knights count nothing
setoption name Club Term King Shelter value false
position startpos moves e2e4 e7e5
eval                                          # ends with "Weights changed: ..."
```

## Build

```sh
cd src && make -j4 build ARCH=x86-64-avx2     # or profile-build, as bot-elo-estimator does
./stockfish bench                             # 3689473 nodes
```

## In bot-elo-estimator

Three steps, following that repository's "Adding an engine":

1. Copy [Dockerfile](Dockerfile) to `images/engines/manguruyu/Dockerfile`. It pins this branch by
   commit (`MANGURUYU_COMMIT`); update the hash when the branch moves.
2. Add the engine and its command to `bots.toml`, and the engine to an image's `engines`:

   ```toml
   [engines.manguruyu]
   [engines.manguruyu.commands]
   manguruyu = { run = "manguruyu/stockfish", expect = "club evaluation" }
   ```

   The smoke test's `expect` matches the info string the engine prints at its first search.
3. Add the candidates. [candidates.py](candidates.py) asks the binary for its `Club` options and
   prints one candidate per setting in `bots.toml`'s format: the baseline, one per weight at 0
   (`manguruyu-no-knight`, ...) and one per term off (`manguruyu-without-king-shelter`, ...):

   ```sh
   python3 manguruyu/candidates.py --binary src/stockfish --go "go movetime 100" >> /path/to/bots.toml
   ```

Then `./build_docker.sh engine manguruyu`, `uv run estimate/play_matches.py --candidates
manguruyu manguruyu-no-knight ...` and `python3 estimate/estimate_elo.py`: the summary's
rating per candidate, against the baseline's, is what the weight was worth.
