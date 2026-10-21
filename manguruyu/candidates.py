#!/usr/bin/env python3
"""Print bot-elo-estimator candidates for Manguruyu: the full club evaluation, then one candidate per weight dropped and one per term switched off.

    python3 manguruyu/candidates.py [--binary PATH] [--command manguruyu] [--go "go movetime 100"]
        [--weights] [--terms] [--prefix manguruyu]

Asks the binary for its UCI options (`uci`), keeps the "Club ..." ones, and writes
one `[candidates.<prefix>-...]` table per setting in bots.toml's format: the
baseline with no options, then for every weight option "Club X" a candidate
`<prefix>-no-x` with that weight at 0, and for every term switch "Club Term Y" a
candidate `<prefix>-without-y` with the term off. --weights or --terms prints
only the one kind. Paste the output into bots.toml's candidates section and run
estimate/play_matches.py; the summary then shows what each weight was worth.

The list comes from the binary, so it never goes stale against the engine.
"""

from __future__ import annotations

import argparse
import re
import subprocess
import sys

OPTION = re.compile(r"^option name (?P<name>Club .+?) type (?P<type>spin|check) default (?P<default>\S+)")


def club_options(binary: str) -> list[tuple[str, str, str]]:
    """Every Club option the binary offers: name, type and default."""
    out = subprocess.run([binary], input="uci\nquit\n", capture_output=True, text=True, check=True).stdout
    found: list[tuple[str, str, str]] = []
    for line in out.splitlines():
        m = OPTION.match(line)
        if m:
            found.append((m.group("name"), m.group("type"), m.group("default")))
    return found


def slug(text: str) -> str:
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def table(name: str, command: str, go: str, options: dict[str, str] | None, comment: str) -> str:
    lines = [f"# {comment}", f"[candidates.{name}]", f'command = "{command}"', f'go = "{go}"']
    if options:
        inner = ", ".join(f'"{key}" = {value}' for key, value in options.items())
        lines.append(f"options = {{ {inner} }}")
    return "\n".join(lines) + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--binary", default="src/stockfish", help="the Manguruyu binary to ask (default src/stockfish)")
    parser.add_argument("--command", default="manguruyu", help="the command name in bots.toml (default manguruyu)")
    parser.add_argument("--go", default="go movetime 100", help="the candidates' search (default 'go movetime 100')")
    parser.add_argument("--prefix", default="manguruyu", help="the candidates' name prefix (default manguruyu)")
    parser.add_argument("--weights", action="store_true", help="only the one-weight-dropped candidates")
    parser.add_argument("--terms", action="store_true", help="only the one-term-off candidates")
    args = parser.parse_args()
    both = not args.weights and not args.terms

    options = club_options(args.binary)
    weights = [(name, default) for name, kind, default in options if kind == "spin" and name != "Club Scale"]
    terms = [name for name, kind, _default in options if kind == "check" and name.startswith("Club Term ")]

    blocks = [table(args.prefix, args.command, args.go, None, "Manguruyu with every club weight, the baseline")]
    if both or args.weights:
        for name, default in weights:
            short = name[len("Club "):]
            blocks.append(table(f"{args.prefix}-no-{slug(short)}", args.command, args.go, {name: "0"}, f"{short} dropped (the club's {default})"))
    if both or args.terms:
        for name in terms:
            short = name[len("Club Term "):]
            blocks.append(table(f"{args.prefix}-without-{slug(short)}", args.command, args.go, {name: "\"false\""}, f"the {short.lower()} term switched off"))
    sys.stdout.write("\n".join(blocks))
    return 0


if __name__ == "__main__":
    sys.exit(main())
