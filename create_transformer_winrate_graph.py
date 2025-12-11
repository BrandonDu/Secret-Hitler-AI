#!/usr/bin/env python3
"""
Create graph showing transformer win rate over manual (direct comparison)
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
    print("Generating transformer win rate over manual comparison...")
    print("Running tests...\n")
    
    iterations_list = [25, 50, 100, 200]
    manual_results = []
    transformer_results = []
    
    for iters in iterations_list:
        print(f"Testing with {iters} iterations...")
        print("  Manual...", end=" ", flush=True)
        manual = run_test(iters, "manual", games=100)
        manual_results.append(manual)
        print(f"{manual:.2f}%")
        
        print("  Transformer...", end=" ", flush=True)
        transformer = run_test(iters, "transformer", games=100)
        transformer_results.append(transformer)
        print(f"{transformer:.2f}%")
        print()
    
    # Calculate transformer win rate over manual (percentage point difference)
    transformer_advantage = [t - m for t, m in zip(transformer_results, manual_results)]
    transformer_win_rate = transformer_results  # Direct win rate
    
    # Create focused graphs
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(16, 6))
    
    # Graph 1: Transformer Win Rate (direct)
    ax1.plot(iterations_list, transformer_win_rate, marker='o', label='Transformer Win Rate', 
            linewidth=3, markersize=12, color='#2ecc71')
    ax1.axhline(y=50, color='gray', linestyle='--', linewidth=1, alpha=0.5, label='50% (Even)')
    ax1.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax1.set_ylabel('Transformer Win Rate (%)', fontsize=13, fontweight='bold')
    ax1.set_title('Transformer MCTS Win Rate\n(vs Random Opponent)', 
                  fontsize=14, fontweight='bold', pad=15)
    ax1.legend(fontsize=11)
    ax1.grid(True, alpha=0.3, linestyle='--')
    ax1.set_ylim([48, 62])
    ax1.set_xticks(iterations_list)
    
    # Add value labels
    for i, (x, y) in enumerate(zip(iterations_list, transformer_win_rate)):
        ax1.text(x, y + 0.5, f'{y:.1f}%', ha='center', va='bottom', 
                fontsize=10, fontweight='bold')
    
    # Graph 2: Transformer Advantage Over Manual (percentage points)
    bars = ax2.bar(iterations_list, transformer_advantage, width=20, 
                   alpha=0.85, color=['#e74c3c' if x < 0 else '#2ecc71' for x in transformer_advantage])
    ax2.axhline(y=0, color='black', linestyle='-', linewidth=1.5)
    ax2.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax2.set_ylabel('Transformer Advantage (Percentage Points)', fontsize=13, fontweight='bold')
    ax2.set_title('Transformer Win Rate Over Manual\n(Positive = Transformer Better)', 
                  fontsize=14, fontweight='bold', pad=15)
    ax2.set_xticks(iterations_list)
    ax2.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax2.set_ylim([-2, max(transformer_advantage) + 1])
    
    # Add value labels
    for i, (x, y) in enumerate(zip(iterations_list, transformer_advantage)):
        ax2.text(x, y + (0.1 if y > 0 else -0.3), f'{y:+.2f}%', 
                ha='center', va='bottom' if y > 0 else 'top', 
                fontsize=11, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('transformer_winrate_over_manual.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'transformer_winrate_over_manual.png'")
    
    # Create detailed comparison
    fig2, ax3 = plt.subplots(1, 1, figsize=(10, 6))
    
    x = np.arange(len(iterations_list))
    width = 0.35
    
    bars1 = ax3.bar(x - width/2, manual_results, width, label='Manual (Baseline)', 
                    alpha=0.85, color='#e74c3c')
    bars2 = ax3.bar(x + width/2, transformer_results, width, label='Transformer', 
                    alpha=0.85, color='#2ecc71')
    
    ax3.set_xlabel('MCTS Iterations', fontsize=13, fontweight='bold')
    ax3.set_ylabel('Win Rate (%)', fontsize=13, fontweight='bold')
    ax3.set_title('Transformer vs Manual: Direct Comparison\n(vs Random Opponent)', 
                  fontsize=14, fontweight='bold', pad=15)
    ax3.set_xticks(x)
    ax3.set_xticklabels(iterations_list)
    ax3.legend(fontsize=12)
    ax3.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax3.set_ylim([48, 62])
    
    # Add value labels
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', 
                    fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('transformer_vs_manual_direct.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'transformer_vs_manual_direct.png'")
    
    # Print summary
    print("\n" + "="*70)
    print("TRANSFORMER WIN RATE OVER MANUAL")
    print("="*70)
    print(f"{'Iterations':<12} {'Manual':<12} {'Transformer':<14} {'Advantage':<12}")
    print("-"*70)
    for i, iters in enumerate(iterations_list):
        m = manual_results[i]
        t = transformer_results[i]
        adv = t - m
        print(f"{iters:<12} {m:<12.2f} {t:<14.2f} {adv:+.2f}%")
    
    avg_adv = sum(transformer_advantage) / len(transformer_advantage)
    print(f"\nAverage transformer advantage: {avg_adv:+.2f} percentage points")
    print("✓ All graphs generated!")

if __name__ == "__main__":
    main()


