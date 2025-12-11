#!/usr/bin/env python3
"""
Generate comparison graphs for MCTS with and without transformer (retrained model)
"""
import subprocess
import re
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
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
    print("Running comprehensive tests for graph generation...")
    print("This will take a while...\n")
    
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
            win_rate = run_test(iters, mode_flag, games=100)
            if win_rate is not None:
                results[mode_name].append(win_rate)
                print(f"{win_rate:.2f}%")
            else:
                print("FAILED")
        print()
    
    # Create graphs
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Graph 1: Line plot
    for mode_name in modes.keys():
        if len(results[mode_name]) == len(iterations_list):
            ax1.plot(iterations_list, results[mode_name], marker='o', label=mode_name, linewidth=2.5, markersize=10)
    
    ax1.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax1.set_ylabel('Win Rate (%)', fontsize=13, fontweight='bold')
    ax1.set_title('MCTS Performance: Manual vs Transformer vs Both\n(vs Random Opponent, 100 games)', fontsize=14, fontweight='bold', pad=15)
    ax1.legend(fontsize=12, loc='best')
    ax1.grid(True, alpha=0.3, linestyle='--')
    ax1.set_ylim([45, 65])
    ax1.set_xticks(iterations_list)
    
    # Graph 2: Bar chart
    x = np.arange(len(iterations_list))
    width = 0.25
    
    bars1 = ax2.bar(x - width, results["Manual"], width, label='Manual', alpha=0.85, color='#e74c3c')
    bars2 = ax2.bar(x, results["Transformer"], width, label='Transformer', alpha=0.85, color='#2ecc71')
    bars3 = ax2.bar(x + width, results["Both"], width, label='Both', alpha=0.85, color='#3498db')
    
    ax2.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax2.set_ylabel('Win Rate (%)', fontsize=13, fontweight='bold')
    ax2.set_title('Win Rate by Iteration Count', fontsize=14, fontweight='bold', pad=15)
    ax2.set_xticks(x)
    ax2.set_xticklabels(iterations_list)
    ax2.legend(fontsize=12)
    ax2.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax2.set_ylim([45, 65])
    
    # Add value labels on bars
    for bars in [bars1, bars2, bars3]:
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%',
                    ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('mcts_comparison_retrained.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'mcts_comparison_retrained.png'")
    
    # Create improvement graph
    fig2, ax3 = plt.subplots(1, 1, figsize=(10, 6))
    
    if len(results["Manual"]) == len(iterations_list) and len(results["Transformer"]) == len(iterations_list):
        improvement_transformer = [t - m for t, m in zip(results["Transformer"], results["Manual"])]
        improvement_both = [b - m for b, m in zip(results["Both"], results["Manual"])]
        
        x_pos = np.arange(len(iterations_list))
        width = 0.35
        
        bars1 = ax3.bar(x_pos - width/2, improvement_transformer, width, 
                        label='Transformer vs Manual', alpha=0.85, color='#2ecc71')
        bars2 = ax3.bar(x_pos + width/2, improvement_both, width, 
                        label='Both vs Manual', alpha=0.85, color='#3498db')
        
        ax3.axhline(y=0, color='black', linestyle='-', linewidth=1.2)
        ax3.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
        ax3.set_ylabel('Win Rate Improvement (%)', fontsize=13, fontweight='bold')
        ax3.set_title('Transformer Improvement Over Manual MCTS (Retrained Model)', fontsize=14, fontweight='bold', pad=15)
        ax3.set_xticks(x_pos)
        ax3.set_xticklabels(iterations_list)
        ax3.legend(fontsize=12)
        ax3.grid(True, alpha=0.3, axis='y', linestyle='--')
        
        # Add value labels
        for bars in [bars1, bars2]:
            for bar in bars:
                height = bar.get_height()
                ax3.text(bar.get_x() + bar.get_width()/2., height,
                        f'{height:+.1f}%',
                        ha='center', va='bottom' if height > 0 else 'top', 
                        fontsize=10, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('mcts_improvement_retrained.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'mcts_improvement_retrained.png'")
    
    # Print summary table
    print("\n" + "="*70)
    print("SUMMARY TABLE (Retrained Model)")
    print("="*70)
    print(f"{'Iterations':<12} {'Manual':<12} {'Transformer':<14} {'Both':<12} {'Best':<12}")
    print("-"*70)
    for i, iters in enumerate(iterations_list):
        m, t, b = results["Manual"][i], results["Transformer"][i], results["Both"][i]
        best = max(m, t, b)
        best_name = "Manual" if best == m else ("Transformer" if best == t else "Both")
        print(f"{iters:<12} {m:<12.2f} {t:<14.2f} {b:<12.2f} {best_name:<12}")
    
    print("\n✓ All graphs generated successfully!")

if __name__ == "__main__":
    main()

