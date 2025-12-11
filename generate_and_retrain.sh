#!/bin/bash
set -e

export KMP_DUPLICATE_LIB_OK=TRUE

echo "=== Step 1: Generating Self-Play Data ==="
echo "Running 1000 games of self-play..."
./secret_hitler_bot --mode train --rounds 1 --games-per-round 1000 --epochs 5 --test-games 50 --feat-mode manual

echo ""
echo "=== Step 2: Converting Training Data ==="
if [ -f "src/training_buffer.bin" ]; then
    python3 src/convert_training_data.py src/training_buffer.bin /tmp/new_training_data.npz
    echo "Converted training buffer to /tmp/new_training_data.npz"
else
    echo "ERROR: training_buffer.bin not found!"
    exit 1
fi

echo ""
echo "=== Step 3: Combining with Existing Data ==="
python3 << 'PYTHON_SCRIPT'
import numpy as np
import sys

# Load existing data
try:
    existing = np.load("src/sh_data.npz")
    X_old = existing["X"]
    y_old = existing["y"]
    print(f"Existing data: {len(X_old)} samples")
except:
    X_old = None
    y_old = None
    print("No existing data found, using new data only")

# Load new data
try:
    new_data = np.load("/tmp/new_training_data.npz")
    X_new = new_data["X"]
    y_new = new_data["y"]
    print(f"New data: {len(X_new)} samples")
except Exception as e:
    print(f"ERROR loading new data: {e}")
    sys.exit(1)

# Combine
if X_old is not None:
    X_combined = np.vstack([X_old, X_new])
    y_combined = np.concatenate([y_old, y_new])
    print(f"Combined data: {len(X_combined)} samples")
else:
    X_combined = X_new
    y_combined = y_new

# Save combined dataset
np.savez("src/sh_data_combined.npz", X=X_combined, y=y_combined)
print(f"Saved combined dataset to src/sh_data_combined.npz")
PYTHON_SCRIPT

echo ""
echo "=== Step 4: Training Transformer on Combined Data ==="
python3 src/train_transformer.py src/sh_data_combined.npz 32 src/sh_transformer.pt \
    --epochs 100 --lr 0.001 --val-split 0.2 --patience 15

echo ""
echo "=== Done! ==="
echo "New model saved to: src/sh_transformer.pt"
echo "Combined dataset: src/sh_data_combined.npz"

