#!/usr/bin/env python3
"""
Generate comparison graphs for MCTS with and without transformer
"""

import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np

def run_test(iterations, feat_mode, opponent="random", games=100):
    """Run a test and extract win rate"""
    cmd = [
        "./secret_hitler_bot",
        "--mode", "evaluate",
        "--games", str(games),
        "--iters", str(iterations),
        "--feat-mode", feat_mode,
        "--opponent", opponent
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    # Extract win percentage
    match = re.search(r'MCTS ([\d.]+)%', result.stdout)
    if match:
        return float(match.group(1))
    return None

def main():
    print("Running comprehensive tests...")
    
    iterations_list = [25, 50, 100, 200]
    modes = {
        "Manual": "manual",
        "Transformer": "transformer", 
        "Both": "both"
    }
    
    results = {mode: [] for mode in modes.keys()}
    
    for iters in iterations_list:
        print(f"Testing with {iters} iterations...")
        for mode_name, mode_flag in modes.items():
            print(f"  {mode_name}...", end=" ", flush=True)
            win_rate = run_test(iters, mode_flag)
            if win_rate is not None:
                results[mode_name].append(win_rate)
                print(f"{win_rate:.2f}%")
            else:
                print("FAILED")
    
    # Create graphs
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))
    
    # Graph 1: Win rate vs iterations
    for mode_name in modes.keys():
        if len(results[mode_name]) == len(iterations_list):
            ax1.plot(iterations_list, results[mode_name], marker='o', label=mode_name, linewidth=2, markersize=8)
    
    ax1.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax1.set_title('MCTS Performance: Manual vs Transformer vs Both\n(vs Random Opponent)', fontsize=13, fontweight='bold')
    ax1.legend(fontsize=11)
    ax1.grid(True, alpha=0.3)
    ax1.set_ylim([40, 70])
    ax1.set_xticks(iterations_list)
    
    # Graph 2: Bar chart comparison
    x = np.arange(len(iterations_list))
    width = 0.25
    
    for i, mode_name in enumerate(modes.keys()):
        if len(results[mode_name]) == len(iterations_list):
            offset = (i - 1) * width
            ax2.bar(x + offset, results[mode_name], width, label=mode_name, alpha=0.8)
    
    ax2.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax2.set_title('Win Rate Comparison by Iteration Count', fontsize=13, fontweight='bold')
    ax2.set_xticks(x)
    ax2.set_xticklabels(iterations_list)
    ax2.legend(fontsize=11)
    ax2.grid(True, alpha=0.3, axis='y')
    ax2.set_ylim([40, 70])
    
    plt.tight_layout()
    plt.savefig('mcts_comparison.png', dpi=300, bbox_inches='tight')
    print("\nGraph saved as 'mcts_comparison.png'")
    
    # Create improvement graph
    fig2, ax3 = plt.subplots(1, 1, figsize=(10, 6))
    
    if len(results["Manual"]) == len(iterations_list) and len(results["Transformer"]) == len(iterations_list):
        improvement_transformer = [t - m for t, m in zip(results["Transformer"], results["Manual"])]
        improvement_both = [b - m for b, m in zip(results["Both"], results["Manual"])]
        
        x_pos = np.arange(len(iterations_list))
        width = 0.35
        
        ax3.bar(x_pos - width/2, improvement_transformer, width, label='Transformer vs Manual', alpha=0.8, color='#2ecc71')
        ax3.bar(x_pos + width/2, improvement_both, width, label='Both vs Manual', alpha=0.8, color='#3498db')
        
        ax3.axhline(y=0, color='black', linestyle='-', linewidth=0.8)
        ax3.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
        ax3.set_ylabel('Win Rate Improvement (%)', fontsize=12, fontweight='bold')
        ax3.set_title('Transformer Improvement Over Manual MCTS', fontsize=13, fontweight='bold')
        ax3.set_xticks(x_pos)
        ax3.set_xticklabels(iterations_list)
        ax3.legend(fontsize=11)
        ax3.grid(True, alpha=0.3, axis='y')
    
    plt.tight_layout()
    plt.savefig('mcts_improvement.png', dpi=300, bbox_inches='tight')
    print("Graph saved as 'mcts_improvement.png'")
    
    # Print summary table
    print("\n" + "="*60)
    print("SUMMARY TABLE")
    print("="*60)
    print(f"{'Iterations':<12} {'Manual':<10} {'Transformer':<12} {'Both':<10} {'Best':<10}")
    print("-"*60)
    for i, iters in enumerate(iterations_list):
        manual = results["Manual"][i] if i < len(results["Manual"]) else 0
        transformer = results["Transformer"][i] if i < len(results["Transformer"]) else 0
        both = results["Both"][i] if i < len(results["Both"]) else 0
        best = max(manual, transformer, both)
        best_name = "Manual" if best == manual else ("Transformer" if best == transformer else "Both")
        print(f"{iters:<12} {manual:<10.2f} {transformer:<12.2f} {both:<10.2f} {best_name:<10}")

if __name__ == "__main__":
    try:
        import matplotlib
        matplotlib.use('Agg')  # Non-interactive backend
        main()
    except ImportError:
        print("matplotlib not found. Installing...")
        subprocess.run(["pip3", "install", "matplotlib", "numpy"], check=True)
        import matplotlib
        matplotlib.use('Agg')
        main()

