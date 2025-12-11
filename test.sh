#!/usr/bin/env bash
set -e

USAGE="Usage: $0 train|evaluate [options]
  train:    run self-play training
  evaluate: run evaluation (forwards all extra args to binary)
Examples:
  $0 train
  $0 evaluate --games 10000 --iters 10 --opponent greedy
"

if [[ $# -lt 1 ]]; then
    echo "$USAGE" >&2
    exit 1
fi

MODE=$1
shift

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
    echo "=== Evaluating Secret Hitler MCTS Bot ==="
    echo "Forwarding args to secret_hitler_bot..."
    echo "./secret_hitler_bot --mode evaluate $@"
    ./secret_hitler_bot --mode evaluate "$@"
    ;;
*)
    echo "Unknown mode: $MODE" >&2
    echo "$USAGE" >&2
    exit 1
    ;;
esac
