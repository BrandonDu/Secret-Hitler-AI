import numpy as np
import sys
import struct

path = sys.argv[1]
out = sys.argv[2]

with open(path, "rb") as f:
    N = struct.unpack("i", f.read(4))[0]
    input_dim = struct.unpack("i", f.read(4))[0]
    X = np.zeros((N, input_dim), dtype=np.float32)
    y = np.zeros(N, dtype=np.int64)
    for i in range(N):
        buf_x = f.read(input_dim * 4)
        X[i] = np.frombuffer(buf_x, dtype=np.float32)
        y[i] = struct.unpack("i", f.read(4))[0]

np.savez(out, X=X, y=y)
print("wrote", out, "N =", N, "input_dim =", input_dim)
