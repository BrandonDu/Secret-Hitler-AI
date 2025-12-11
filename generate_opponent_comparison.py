#!/usr/bin/env python3
"""
Generate comparison graphs for MCTS against different opponents (greedy and random)
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
    print("Running comprehensive tests against greedy and random opponents...")
    print("This will take a while...\n")
    
    iterations_list = [50, 100, 200]
    modes = {
        "Manual": "manual",
        "Transformer": "transformer", 
        "Both": "both"
    }
    opponents = ["random", "greedy"]
    
    results = {opponent: {mode: [] for mode in modes.keys()} for opponent in opponents}
    
    for opponent in opponents:
        print(f"\n=== Testing against {opponent.upper()} opponent ===")
        for iters in iterations_list:
            print(f"  {iters} iterations:")
            for mode_name, mode_flag in modes.items():
                print(f"    {mode_name}...", end=" ", flush=True)
                win_rate = run_test(iters, mode_flag, opponent=opponent, games=100)
                if win_rate is not None:
                    results[opponent][mode_name].append(win_rate)
                    print(f"{win_rate:.2f}%")
                else:
                    print("FAILED")
                    results[opponent][mode_name].append(0.0)
        print()
    
    # Create comparison graphs
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    
    # Graph 1: Line plot - Random opponent
    ax1 = axes[0, 0]
    for mode_name in modes.keys():
        if len(results["random"][mode_name]) == len(iterations_list):
            ax1.plot(iterations_list, results["random"][mode_name], 
                    marker='o', label=mode_name, linewidth=2.5, markersize=10)
    ax1.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax1.set_title('MCTS Performance vs Random Opponent\n(100 games)', fontsize=13, fontweight='bold', pad=15)
    ax1.legend(fontsize=11)
    ax1.grid(True, alpha=0.3, linestyle='--')
    ax1.set_ylim([40, 70])
    ax1.set_xticks(iterations_list)
    
    # Graph 2: Line plot - Greedy opponent
    ax2 = axes[0, 1]
    for mode_name in modes.keys():
        if len(results["greedy"][mode_name]) == len(iterations_list):
            ax2.plot(iterations_list, results["greedy"][mode_name], 
                    marker='s', label=mode_name, linewidth=2.5, markersize=10)
    ax2.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax2.set_title('MCTS Performance vs Greedy Opponent\n(100 games)', fontsize=13, fontweight='bold', pad=15)
    ax2.legend(fontsize=11)
    ax2.grid(True, alpha=0.3, linestyle='--')
    ax2.set_ylim([40, 70])
    ax2.set_xticks(iterations_list)
    
    # Graph 3: Bar chart - Random opponent
    ax3 = axes[1, 0]
    x = np.arange(len(iterations_list))
    width = 0.25
    bars1 = ax3.bar(x - width, results["random"]["Manual"], width, label='Manual', alpha=0.85, color='#e74c3c')
    bars2 = ax3.bar(x, results["random"]["Transformer"], width, label='Transformer', alpha=0.85, color='#2ecc71')
    bars3 = ax3.bar(x + width, results["random"]["Both"], width, label='Both', alpha=0.85, color='#3498db')
    ax3.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax3.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax3.set_title('Win Rate vs Random Opponent', fontsize=13, fontweight='bold', pad=15)
    ax3.set_xticks(x)
    ax3.set_xticklabels(iterations_list)
    ax3.legend(fontsize=11)
    ax3.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax3.set_ylim([40, 70])
    for bars in [bars1, bars2, bars3]:
        for bar in bars:
            height = bar.get_height()
            ax3.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Graph 4: Bar chart - Greedy opponent
    ax4 = axes[1, 1]
    bars1 = ax4.bar(x - width, results["greedy"]["Manual"], width, label='Manual', alpha=0.85, color='#e74c3c')
    bars2 = ax4.bar(x, results["greedy"]["Transformer"], width, label='Transformer', alpha=0.85, color='#2ecc71')
    bars3 = ax4.bar(x + width, results["greedy"]["Both"], width, label='Both', alpha=0.85, color='#3498db')
    ax4.set_xlabel('MCTS Iterations', fontsize=12, fontweight='bold')
    ax4.set_ylabel('Win Rate (%)', fontsize=12, fontweight='bold')
    ax4.set_title('Win Rate vs Greedy Opponent', fontsize=13, fontweight='bold', pad=15)
    ax4.set_xticks(x)
    ax4.set_xticklabels(iterations_list)
    ax4.legend(fontsize=11)
    ax4.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax4.set_ylim([40, 70])
    for bars in [bars1, bars2, bars3]:
        for bar in bars:
            height = bar.get_height()
            ax4.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plt.savefig('mcts_vs_opponents.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'mcts_vs_opponents.png'")
    
    # Create side-by-side comparison
    fig2, (ax5, ax6) = plt.subplots(1, 2, figsize=(16, 6))
    
    # Comparison at 100 iterations
    iter_idx = iterations_list.index(100) if 100 in iterations_list else 1
    
    categories = list(modes.keys())
    random_vals = [results["random"][mode][iter_idx] for mode in categories]
    greedy_vals = [results["greedy"][mode][iter_idx] for mode in categories]
    
    x = np.arange(len(categories))
    width = 0.35
    
    bars1 = ax5.bar(x - width/2, random_vals, width, label='vs Random', alpha=0.85, color='#3498db')
    bars2 = ax5.bar(x + width/2, greedy_vals, width, label='vs Greedy', alpha=0.85, color='#e67e22')
    
    ax5.set_xlabel('MCTS Mode', fontsize=13, fontweight='bold')
    ax5.set_ylabel('Win Rate (%)', fontsize=13, fontweight='bold')
    ax5.set_title(f'MCTS Performance Comparison at {iterations_list[iter_idx]} Iterations', fontsize=14, fontweight='bold', pad=15)
    ax5.set_xticks(x)
    ax5.set_xticklabels(categories)
    ax5.legend(fontsize=12)
    ax5.grid(True, alpha=0.3, axis='y', linestyle='--')
    ax5.set_ylim([40, 70])
    
    for bars in [bars1, bars2]:
        for bar in bars:
            height = bar.get_height()
            ax5.text(bar.get_x() + bar.get_width()/2., height,
                    f'{height:.1f}%', ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    # Improvement over manual for each opponent
    manual_random = results["random"]["Manual"][iter_idx]
    manual_greedy = results["greedy"]["Manual"][iter_idx]
    
    transformer_improvement_random = [results["random"]["Transformer"][iter_idx] - manual_random]
    transformer_improvement_greedy = [results["greedy"]["Transformer"][iter_idx] - manual_greedy]
    both_improvement_random = [results["random"]["Both"][iter_idx] - manual_random]
    both_improvement_greedy = [results["greedy"]["Both"][iter_idx] - manual_greedy]
    
    x_pos = np.arange(2)
    width = 0.35
    
    ax6.bar(x_pos - width/2, [transformer_improvement_random[0], transformer_improvement_greedy[0]], 
            width, label='Transformer vs Manual', alpha=0.85, color='#2ecc71')
    ax6.bar(x_pos + width/2, [both_improvement_random[0], both_improvement_greedy[0]], 
            width, label='Both vs Manual', alpha=0.85, color='#3498db')
    
    ax6.axhline(y=0, color='black', linestyle='-', linewidth=1.2)
    ax6.set_xlabel('Opponent Type', fontsize=13, fontweight='bold')
    ax6.set_ylabel('Win Rate Improvement (%)', fontsize=13, fontweight='bold')
    ax6.set_title(f'Transformer Improvement Over Manual at {iterations_list[iter_idx]} Iterations', fontsize=14, fontweight='bold', pad=15)
    ax6.set_xticks(x_pos)
    ax6.set_xticklabels(['Random', 'Greedy'])
    ax6.legend(fontsize=12)
    ax6.grid(True, alpha=0.3, axis='y', linestyle='--')
    
    plt.tight_layout()
    plt.savefig('mcts_opponent_comparison.png', dpi=300, bbox_inches='tight')
    print("✓ Graph saved as 'mcts_opponent_comparison.png'")
    
    # Print summary tables
    print("\n" + "="*80)
    print("SUMMARY: MCTS vs RANDOM OPPONENT")
    print("="*80)
    print(f"{'Iterations':<12} {'Manual':<12} {'Transformer':<14} {'Both':<12} {'Best':<12}")
    print("-"*80)
    for i, iters in enumerate(iterations_list):
        m = results["random"]["Manual"][i]
        t = results["random"]["Transformer"][i]
        b = results["random"]["Both"][i]
        best = max(m, t, b)
        best_name = "Manual" if best == m else ("Transformer" if best == t else "Both")
        print(f"{iters:<12} {m:<12.2f} {t:<14.2f} {b:<12.2f} {best_name:<12}")
    
    print("\n" + "="*80)
    print("SUMMARY: MCTS vs GREEDY OPPONENT")
    print("="*80)
    print(f"{'Iterations':<12} {'Manual':<12} {'Transformer':<14} {'Both':<12} {'Best':<12}")
    print("-"*80)
    for i, iters in enumerate(iterations_list):
        m = results["greedy"]["Manual"][i]
        t = results["greedy"]["Transformer"][i]
        b = results["greedy"]["Both"][i]
        best = max(m, t, b)
        best_name = "Manual" if best == m else ("Transformer" if best == t else "Both")
        print(f"{iters:<12} {m:<12.2f} {t:<14.2f} {b:<12.2f} {best_name:<12}")
    
    print("\n✓ All graphs generated successfully!")

if __name__ == "__main__":
    main()


