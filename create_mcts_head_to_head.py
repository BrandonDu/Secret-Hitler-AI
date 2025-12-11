#!/usr/bin/env python3
"""
Create head-to-head comparison: ISMCTS_MO (Manual) vs ISMCTS_MO (Transformer)
"""
import subprocess
import re
import matplotlib
matplotlib.use('Agg')
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
    match = re.search(r'MCTS ([\d.]+)%', result.stdout)
    if match:
        return float(match.group(1))
    return None

def main():
    print("Creating head-to-head comparison: ISMCTS_MO (Manual) vs ISMCTS_MO (Transformer)")
    print("Running comprehensive tests...\n")
    
    iterations_list = [25, 50, 100, 200]
    manual_results = []
    transformer_results = []
    
    for iters in iterations_list:
        print(f"Testing with {iters} iterations...")
        print("  Manual (baseline)...", end=" ", flush=True)
        manual = run_test(iters, "manual", games=100)
        manual_results.append(manual)
        print(f"{manual:.2f}%")
        
        print("  Transformer...", end=" ", flush=True)
        transformer = run_test(iters, "transformer", games=100)
        transformer_results.append(transformer)
        print(f"{transformer:.2f}%")
        print()
    
    # Create comparison graphs
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    
    # Graph 1: Direct comparison line plot
    ax1 = axes[0, 0]
    ax1.plot(iterations_list, manual_results, marker='o', label='ISMCTS_MO (Manual)', 
            linewidth=3, markersize=12, color='#e74c3c')
    ax1.plot(iterations_list, transformer_results, marker='s', label='ISMCTS_MO (Transformer)', 
            linewidth=3, markersize=12, color='#2ecc71')
    ax1.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax1.set_ylabel('Win Rate vs Random (%)', fontsize=13, fontweight='bold')
    ax1.set_title('ISMCTS_MO: Manual vs Transformer\n(Head-to-Head Comparison)', 
                  fontsize=14, fontweight='bold', pad=15)
    ax1.legend(fontsize=12)
    ax1.grid(True, alpha=0.3, linestyle='--')
    ax1.set_ylim([45, 65])
    ax1.set_xticks(iterations_list)
    
    # Graph 2: Bar chart comparison
    ax2 = axes[0, 1]
    x = np.arange(len(iterations_list))
    width = 0.35
    bars1 = ax2.bar(x - width/2, manual_results, width, label='Manual (Baseline)', 
                    alpha=0.85, color='#e74c3c')
    bars2 = ax2.bar(x + width/2, transformer_results, width, label='Transformer', 
                    alpha=0.85, color='#2ecc71')
    ax2.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax2.set_ylabel('Win Rate vs Random (%)', fontsize=13, fontweight='bold')
    ax2.set_title('Win Rate Comparison', fontsize=14, fontweight='bold', pad=15)
    ax2.set_xticks(x)
    ax2.set_xticklabels(iterations_list)
    ax2.legend(fontsize=12)
    ax2.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax2.set_ylim([45, 65])
    
    # Add value labels
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax2.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', 
                    fontsize=10, fontweight='bold')
    
    # Graph 3: Improvement graph
    ax3 = axes[1, 0]
    improvement = [t - m for t, m in zip(transformer_results, manual_results)]
    bars = ax3.bar(x, improvement, width=0.6, alpha=0.85, color='#3498db')
    ax3.axhline(y=0, color='black', linestyle='-', linewidth=1.5)
    ax3.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax3.set_ylabel('Win Rate Improvement (%)', fontsize=13, fontweight='bold')
    ax3.set_title('Transformer Improvement Over Manual Mode', fontsize=14, fontweight='bold', pad=15)
    ax3.set_xticks(x)
    ax3.set_xticklabels(iterations_list)
    ax3.grid(True, alpha=0.3, axis='y', linestyle='--')
    
    # Add value labels
    for i, (bar, imp) in enumerate(zip(bars, improvement)):
        height = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width()/2., height,
                f'{imp:+.1f}%',
                ha='center', va='bottom' if height > 0 else 'top', 
                fontsize=11, fontweight='bold')
    
    # Graph 4: Win rate difference visualization
    ax4 = axes[1, 1]
    win_diff = improvement
    colors = ['#2ecc71' if d > 0 else '#e74c3c' for d in win_diff]
    bars = ax4.bar(x, win_diff, width=0.6, alpha=0.85, color=colors)
    ax4.axhline(y=0, color='black', linestyle='-', linewidth=1.5)
    ax4.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax4.set_ylabel('Transformer Advantage (%)', fontsize=13, fontweight='bold')
    ax4.set_title('Transformer Advantage by Iteration Count', fontsize=14, fontweight='bold', pad=15)
    ax4.set_xticks(x)
    ax4.set_xticklabels(iterations_list)
    ax4.grid(True, alpha=0.3, axis='y', linestyle='--')
    
    # Add value labels
    for i, (bar, diff) in enumerate(zip(bars, win_diff)):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                f'{diff:+.2f}%',
                ha='center', va='bottom' if height > 0 else 'top', 
                fontsize=10, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('mcts_transformer_vs_manual.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'mcts_transformer_vs_manual.png'")
    
    # Print summary
    print("\n" + "="*70)
    print("SUMMARY: ISMCTS_MO Transformer vs Manual")
    print("="*70)
    print(f"{'Iterations':<12} {'Manual':<12} {'Transformer':<14} {'Difference':<12}")
    print("-"*70)
    for i, iters in enumerate(iterations_list):
        m = manual_results[i]
        t = transformer_results[i]
        diff = t - m
        print(f"{iters:<12} {m:<12.2f} {t:<14.2f} {diff:+.2f}%")
    
    avg_improvement = sum(improvement) / len(improvement)
    print(f"\nAverage improvement: {avg_improvement:+.2f}%")
    print("✓ Comparison complete!")

if __name__ == "__main__":
    main()


