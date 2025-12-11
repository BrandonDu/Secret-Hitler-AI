import sys
import math
from typing import Dict, Any

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import TensorDataset, DataLoader


class PositionalEncoding(nn.Module):
    def __init__(self, d_model: int, max_len: int = 5000):
        super().__init__()
        pe = torch.zeros(max_len, d_model, dtype=torch.float32)
        position = torch.arange(0, max_len, dtype=torch.float32).unsqueeze(1)
        div_term = torch.exp(
            torch.arange(0, d_model, 2, dtype=torch.float32)
            * (-math.log(10000.0) / float(d_model))
        )
        pe[:, 0::2] = torch.sin(position * div_term)
        if d_model % 2 == 1:
            pe[:, 1::2] = torch.cos(position * div_term)[:, :-1]
        else:
            pe[:, 1::2] = torch.cos(position * div_term)
        pe = pe.unsqueeze(0)
        self.register_buffer("pe", pe)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        seq_len = x.size(1)
        return x + self.pe[:, :seq_len]


class SHTransformerDecision(nn.Module):
    __annotations__: Dict[str, Any] = {}

    def __init__(
        self,
        input_dim: int,
        d_model: int = 128,
        nhead: int = 4,
        num_layers: int = 2,
        dim_feedforward: int = 256,
        dropout: float = 0.1,
    ):
        super().__init__()
        self.input_proj = nn.Linear(input_dim, d_model)
        self.pos = PositionalEncoding(d_model)
        encoder_layer = nn.TransformerEncoderLayer(
            d_model=d_model,
            nhead=nhead,
            dim_feedforward=dim_feedforward,
            dropout=dropout,
            batch_first=True,
        )
        self.encoder = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
        self.norm = nn.LayerNorm(d_model)
        self.head = nn.Linear(d_model, 1)

    def forward(self, x: torch.Tensor) -> torch.Tensor:
        if x.dim() == 2:
            x = x.unsqueeze(1)
        x = self.input_proj(x)
        x = self.pos(x)
        h = self.encoder(x)
        h_last = h[:, -1, :]
        h_last = self.norm(h_last)
        out = self.head(h_last)
        return out.squeeze(-1)


def train_and_export(
    data_path: str, 
    input_dim: int, 
    out_path: str,
    epochs: int = 50,
    batch_size: int = 256,
    learning_rate: float = 1e-3,
    validation_split: float = 0.2,
    early_stopping_patience: int = 10,
    verbose: bool = True
) -> None:
    data = np.load(data_path)
    X = data["X"].astype(np.float32)
    y = data["y"].astype(np.float32)

    if X.ndim == 2 and X.shape[1] != input_dim:
        raise ValueError(f"input_dim={input_dim}, but X has shape {X.shape}")
    if X.ndim == 1 and X.shape[0] != input_dim:
        raise ValueError(f"input_dim={input_dim}, but X has shape {X.shape}")

    if X.ndim == 1:
        X = X.reshape(1, -1)

    # Split into train/validation
    n_samples = X.shape[0]
    n_val = int(n_samples * validation_split)
    indices = np.random.permutation(n_samples)
    train_indices = indices[n_val:]
    val_indices = indices[:n_val]
    
    X_train, X_val = X[train_indices], X[val_indices]
    y_train, y_val = y[train_indices], y[val_indices]

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    if verbose:
        print(f"Using device: {device}")
        print(f"Training samples: {len(X_train)}, Validation samples: {len(X_val)}")

    model = SHTransformerDecision(input_dim=input_dim).to(device)

    X_train_t = torch.from_numpy(X_train)
    y_train_t = torch.from_numpy(y_train)
    X_val_t = torch.from_numpy(X_val)
    y_val_t = torch.from_numpy(y_val)

    if y_train_t.ndim == 2 and y_train_t.size(1) == 1:
        y_train_t = y_train_t.squeeze(1)
    if y_val_t.ndim == 2 and y_val_t.size(1) == 1:
        y_val_t = y_val_t.squeeze(1)

    train_dataset = TensorDataset(X_train_t, y_train_t)
    val_dataset = TensorDataset(X_val_t, y_val_t)
    train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
    val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False)

    y_min = float(y_train_t.min().item())
    y_max = float(y_train_t.max().item())
    if y_min >= 0.0 and y_max <= 1.0:
        criterion: nn.Module = nn.BCEWithLogitsLoss()
        is_classification = True
    else:
        criterion = nn.MSELoss()
        is_classification = False

    optimizer = optim.Adam(model.parameters(), lr=learning_rate)
    scheduler = optim.lr_scheduler.ReduceLROnPlateau(
        optimizer, mode='min', factor=0.5, patience=5
    )

    best_val_loss = float('inf')
    patience_counter = 0
    train_losses = []
    val_losses = []

    if verbose:
        print(f"\nTraining for {epochs} epochs...")
        print("-" * 60)

    for epoch in range(epochs):
        # Training
        model.train()
        train_loss = 0.0
        train_batches = 0
        for xb, yb in train_loader:
            xb = xb.to(device)
            yb = yb.to(device)
            optimizer.zero_grad()
            preds = model(xb)
            loss = criterion(preds, yb)
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), max_norm=1.0)
            optimizer.step()
            train_loss += loss.item()
            train_batches += 1
        
        avg_train_loss = train_loss / train_batches
        train_losses.append(avg_train_loss)

        # Validation
        model.eval()
        val_loss = 0.0
        val_batches = 0
        with torch.no_grad():
            for xb, yb in val_loader:
                xb = xb.to(device)
                yb = yb.to(device)
                preds = model(xb)
                loss = criterion(preds, yb)
                val_loss += loss.item()
                val_batches += 1
        
        avg_val_loss = val_loss / val_batches
        val_losses.append(avg_val_loss)

        scheduler.step(avg_val_loss)

        if verbose and (epoch + 1) % 5 == 0:
            current_lr = optimizer.param_groups[0]['lr']
            print(f"Epoch {epoch+1:3d}/{epochs} | "
                  f"Train Loss: {avg_train_loss:.6f} | "
                  f"Val Loss: {avg_val_loss:.6f} | "
                  f"LR: {current_lr:.2e}")

        # Early stopping
        if avg_val_loss < best_val_loss:
            best_val_loss = avg_val_loss
            patience_counter = 0
            # Save best model
            best_model_state = model.state_dict().copy()
        else:
            patience_counter += 1
            if patience_counter >= early_stopping_patience:
                if verbose:
                    print(f"\nEarly stopping at epoch {epoch+1} (patience: {early_stopping_patience})")
                break

    # Load best model
    model.load_state_dict(best_model_state)
    
    if verbose:
        print(f"\nBest validation loss: {best_val_loss:.6f}")
        print("Exporting model...")

    model.eval()
    model_cpu = SHTransformerDecision(input_dim=input_dim)
    model_cpu.load_state_dict(model.state_dict())
    model_cpu.eval()
    scripted = torch.jit.script(model_cpu)
    scripted.save(out_path)
    
    if verbose:
        print(f"Model saved to {out_path}")


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description='Train transformer model for Secret Hitler')
    parser.add_argument('data_path', type=str, help='Path to training data (.npz file)')
    parser.add_argument('input_dim', type=int, help='Input dimension')
    parser.add_argument('out_path', type=str, help='Output path for trained model')
    parser.add_argument('--epochs', type=int, default=50, help='Number of training epochs (default: 50)')
    parser.add_argument('--batch-size', type=int, default=256, help='Batch size (default: 256)')
    parser.add_argument('--lr', type=float, default=1e-3, help='Learning rate (default: 1e-3)')
    parser.add_argument('--val-split', type=float, default=0.2, help='Validation split ratio (default: 0.2)')
    parser.add_argument('--patience', type=int, default=10, help='Early stopping patience (default: 10)')
    parser.add_argument('--quiet', action='store_true', help='Suppress training output')
    
    args = parser.parse_args()
    
    train_and_export(
        args.data_path,
        args.input_dim,
        args.out_path,
        epochs=args.epochs,
        batch_size=args.batch_size,
        learning_rate=args.lr,
        validation_split=args.val_split,
        early_stopping_patience=args.patience,
        verbose=not args.quiet
    )
