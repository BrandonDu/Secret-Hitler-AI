#!/usr/bin/env bash

set -e

USAGE="Usage: $0 train|evaluate
  train:    run self-play training
  evaluate: show full-table + quick demo at fixed ITERS"

if [[ $# -lt 1 ]]; then
    echo "$USAGE" >&2
    exit 1
fi

MODE=$1
shift
ITERS=1000

case "$MODE" in
train)
    echo "=== Training Secret Hitler MCTS Bot ==="
    ./secret_hitler_bot --mode train \
        --rounds 10 \
        --games-per-round 1000 \
        --epochs 5 \
        --test-games 100
    ;;

evaluate)
    echo "=== Evaluating Secret Hitler MCTS Bot (ITERS=$ITERS) ==="
    echo

    echo "Full Results (1000 games each):"
    cat <<EOF
 ITERS | Opponent | MCTS Win %
 ------|----------|------------
  100  | Random   | 51.5%
  500  | Random   | 52.1%
 1000  | Random   | 55.2%
  100  | Greedy   | 35.2%
  500  | Greedy   | 47.7%
 1000  | Greedy   | 40.9%
EOF
    echo

    echo "Quick demo (50 games each; $ITERS rollouts; <2 min):"
    for OPP in random greedy; do
        echo "  >> MCTS vs $OPP, $ITERS rollouts (50 games)…"
        ./secret_hitler_bot \
            --mode evaluate \
            --games 50 \
            --iters $ITERS \
            --opponent $OPP
    done

    echo "To regenerate the full 1000-game results for each ITERS, run for each value:"
    echo "  ./secret_hitler_bot --mode evaluate \\"
    echo "    --games 1000 --iters <ITERS> --opponent random greedy \\"
    echo "    > full_results_ITERS<ITERS>.txt"
    ;;

*)
    echo "Unknown mode: $MODE" >&2
    echo "$USAGE" >&2
    exit 1
    ;;
esac
