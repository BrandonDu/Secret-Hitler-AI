#!/usr/bin/env python3
"""
Create a visual diagram of the MCTS + Transformer pipeline
"""
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
from matplotlib.patches import FancyBboxPatch, FancyArrowPatch, BoxStyle

def create_pipeline_diagram():
    fig, ax = plt.subplots(1, 1, figsize=(14, 10))
    ax.set_xlim(0, 10)
    ax.set_ylim(0, 12)
    ax.axis('off')
    
    # Define colors
    input_color = '#3498db'      # Blue
    transformer_color = '#2ecc71'  # Green
    output_color = '#9b59b6'     # Purple
    mcts_color = '#e67e22'       # Orange
    action_color = '#e74c3c'     # Red
    
    # Box styles
    box_style = BoxStyle("Round", pad=0.3)
    
    # Input boxes
    game_state_box = FancyBboxPatch((1, 9), 3, 1.5, 
                                    boxstyle=box_style, 
                                    facecolor=input_color, 
                                    edgecolor='black', 
                                    linewidth=2,
                                    alpha=0.8)
    ax.add_patch(game_state_box)
    ax.text(2.5, 10, 'Game State', ha='center', va='center', 
            fontsize=14, fontweight='bold', color='white')
    
    belief_state_box = FancyBboxPatch((6, 9), 3, 1.5, 
                                      boxstyle=box_style, 
                                      facecolor=input_color, 
                                      edgecolor='black', 
                                      linewidth=2,
                                      alpha=0.8)
    ax.add_patch(belief_state_box)
    ax.text(7.5, 10, 'Belief State', ha='center', va='center', 
            fontsize=14, fontweight='bold', color='white')
    
    # Merge arrow
    arrow1 = FancyArrowPatch((4, 9.75), (5, 8), 
                             arrowstyle='->', 
                             mutation_scale=30, 
                             linewidth=3, 
                             color='black')
    arrow2 = FancyArrowPatch((7, 9.75), (5, 8), 
                             arrowstyle='->', 
                             mutation_scale=30, 
                             linewidth=3, 
                             color='black')
    ax.add_patch(arrow1)
    ax.add_patch(arrow2)
    
    # Feature extraction
    feature_box = FancyBboxPatch((3.5, 7), 3, 1, 
                                 boxstyle=box_style, 
                                 facecolor='#95a5a6', 
                                 edgecolor='black', 
                                 linewidth=2,
                                 alpha=0.8)
    ax.add_patch(feature_box)
    ax.text(5, 7.5, 'Feature Extraction', ha='center', va='center', 
            fontsize=12, fontweight='bold', color='white')
    
    # Arrow to transformer
    arrow3 = FancyArrowPatch((5, 7), (5, 6), 
                             arrowstyle='->', 
                             mutation_scale=30, 
                             linewidth=3, 
                             color='black')
    ax.add_patch(arrow3)
    
    # Transformer Encoder
    transformer_box = FancyBboxPatch((2.5, 4.5), 5, 1, 
                                     boxstyle=box_style, 
                                     facecolor=transformer_color, 
                                     edgecolor='black', 
                                     linewidth=3,
                                     alpha=0.9)
    ax.add_patch(transformer_box)
    ax.text(5, 5, 'Transformer Encoder', ha='center', va='center', 
            fontsize=16, fontweight='bold', color='white')
    
    # Transformer details (smaller text inside)
    ax.text(5, 4.7, '(Positional Encoding + Multi-Head Attention)', 
            ha='center', va='center', fontsize=9, color='white', style='italic')
    
    # Arrow from transformer
    arrow4 = FancyArrowPatch((5, 4.5), (5, 3.5), 
                             arrowstyle='->', 
                             mutation_scale=30, 
                             linewidth=3, 
                             color='black')
    ax.add_patch(arrow4)
    
    # Policy Prior / Value Estimate
    policy_box = FancyBboxPatch((1, 2), 3.5, 1, 
                                boxstyle=box_style, 
                                facecolor=output_color, 
                                edgecolor='black', 
                                linewidth=2,
                                alpha=0.8)
    ax.add_patch(policy_box)
    ax.text(2.75, 2.5, 'Policy Prior', ha='center', va='center', 
            fontsize=13, fontweight='bold', color='white')
    
    value_box = FancyBboxPatch((5.5, 2), 3.5, 1, 
                               boxstyle=box_style, 
                               facecolor=output_color, 
                               edgecolor='black', 
                               linewidth=2,
                               alpha=0.8)
    ax.add_patch(value_box)
    ax.text(7.25, 2.5, 'Value Estimate', ha='center', va='center', 
            fontsize=13, fontweight='bold', color='white')
    
    # Arrow to MCTS
    arrow5 = FancyArrowPatch((2.75, 2), (3, 1.5), 
                             arrowstyle='->', 
                             mutation_scale=25, 
                             linewidth=2.5, 
                             color='black')
    arrow6 = FancyArrowPatch((7.25, 2), (7, 1.5), 
                             arrowstyle='->', 
                             mutation_scale=25, 
                             linewidth=2.5, 
                             color='black')
    ax.add_patch(arrow5)
    ax.add_patch(arrow6)
    
    # Multi-Observer IS-MCTS
    mcts_box = FancyBboxPatch((2, 0.5), 6, 0.8, 
                              boxstyle=box_style, 
                              facecolor=mcts_color, 
                              edgecolor='black', 
                              linewidth=3,
                              alpha=0.9)
    ax.add_patch(mcts_box)
    ax.text(5, 0.9, 'Multi-Observer IS-MCTS', ha='center', va='center', 
            fontsize=15, fontweight='bold', color='white')
    
    # MCTS details
    ax.text(5, 0.7, '(Tree Search with PUCT + Policy Priors)', 
            ha='center', va='center', fontsize=9, color='white', style='italic')
    
    # Title
    ax.text(5, 11.5, 'MCTS + Transformer Pipeline', ha='center', va='center', 
            fontsize=20, fontweight='bold', color='black')
    
    # Add side annotations for clarity
    ax.text(0.3, 10, 'Input:', ha='left', va='center', 
            fontsize=11, fontweight='bold', rotation=90)
    ax.text(0.3, 5, 'Neural\nNetwork:', ha='left', va='center', 
            fontsize=11, fontweight='bold', rotation=90)
    ax.text(0.3, 2.5, 'Policy\nOutput:', ha='left', va='center', 
            fontsize=11, fontweight='bold', rotation=90)
    ax.text(0.3, 0.9, 'Search:', ha='left', va='center', 
            fontsize=11, fontweight='bold', rotation=90)
    
    plt.tight_layout()
    plt.savefig('system_pipeline_diagram.png', dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='none')
    print("✓ Pipeline diagram saved as 'system_pipeline_diagram.png'")

def create_detailed_pipeline_diagram():
    """Create a more detailed version showing the MCTS loop"""
    fig, ax = plt.subplots(1, 1, figsize=(16, 12))
    ax.set_xlim(0, 12)
    ax.set_ylim(0, 14)
    ax.axis('off')
    
    # Define colors
    input_color = '#3498db'
    transformer_color = '#2ecc71'
    output_color = '#9b59b6'
    mcts_color = '#e67e22'
    action_color = '#e74c3c'
    
    box_style = BoxStyle("Round", pad=0.3)
    
    # Title
    ax.text(6, 13.5, 'Complete MCTS + Transformer System Pipeline', ha='center', va='center', 
            fontsize=18, fontweight='bold', color='black')
    
    # Input layer
    game_state_box = FancyBboxPatch((0.5, 11), 3.5, 1.5, boxstyle=box_style, 
                                    facecolor=input_color, edgecolor='black', 
                                    linewidth=2, alpha=0.8)
    ax.add_patch(game_state_box)
    ax.text(2.25, 11.75, 'Game State\n(Board, Policies,\nHistory)', ha='center', va='center', 
            fontsize=11, fontweight='bold', color='white')
    
    belief_state_box = FancyBboxPatch((8, 11), 3.5, 1.5, boxstyle=box_style, 
                                      facecolor=input_color, edgecolor='black', 
                                      linewidth=2, alpha=0.8)
    ax.add_patch(belief_state_box)
    ax.text(9.75, 11.75, 'Belief State\n(Role Probabilities\nper Player)', ha='center', va='center', 
            fontsize=11, fontweight='bold', color='white')
    
    # Merge arrows
    arrow1 = FancyArrowPatch((2.25, 11), (4, 9.5), arrowstyle='->', 
                             mutation_scale=30, linewidth=3, color='black')
    arrow2 = FancyArrowPatch((9.75, 11), (8, 9.5), arrowstyle='->', 
                             mutation_scale=30, linewidth=3, color='black')
    ax.add_patch(arrow1)
    ax.add_patch(arrow2)
    
    # Feature extraction
    feature_box = FancyBboxPatch((4, 8.5), 4, 1.2, boxstyle=box_style, 
                                 facecolor='#95a5a6', edgecolor='black', 
                                 linewidth=2, alpha=0.8)
    ax.add_patch(feature_box)
    ax.text(6, 9.2, 'Feature Extraction', ha='center', va='center', 
            fontsize=13, fontweight='bold', color='white')
    ax.text(6, 8.8, '(Voting/Enactment Features + Role Encoding)', ha='center', va='center', 
            fontsize=9, color='white', style='italic')
    
    arrow3 = FancyArrowPatch((6, 8.5), (6, 7.5), arrowstyle='->', 
                             mutation_scale=30, linewidth=3, color='black')
    ax.add_patch(arrow3)
    
    # Transformer Encoder (detailed)
    transformer_box = FancyBboxPatch((2, 6), 8, 1.2, boxstyle=box_style, 
                                     facecolor=transformer_color, edgecolor='black', 
                                     linewidth=3, alpha=0.9)
    ax.add_patch(transformer_box)
    ax.text(6, 6.8, 'Transformer Encoder', ha='center', va='center', 
            fontsize=16, fontweight='bold', color='white')
    ax.text(6, 6.4, 'Input Projection → Positional Encoding → Multi-Head Attention → Layer Norm', 
            ha='center', va='center', fontsize=9, color='white', style='italic')
    
    arrow4 = FancyArrowPatch((6, 6), (6, 4.8), arrowstyle='->', 
                             mutation_scale=30, linewidth=3, color='black')
    ax.add_patch(arrow4)
    
    # Policy/Value outputs
    policy_box = FancyBboxPatch((0.5, 3.5), 4.5, 1, boxstyle=box_style, 
                                facecolor=output_color, edgecolor='black', 
                                linewidth=2, alpha=0.8)
    ax.add_patch(policy_box)
    ax.text(2.75, 4, 'Policy Prior P(a|s)', ha='center', va='center', 
            fontsize=12, fontweight='bold', color='white')
    ax.text(2.75, 3.7, '(Action Probabilities)', ha='center', va='center', 
            fontsize=9, color='white', style='italic')
    
    value_box = FancyBboxPatch((7, 3.5), 4.5, 1, boxstyle=box_style, 
                               facecolor=output_color, edgecolor='black', 
                               linewidth=2, alpha=0.8)
    ax.add_patch(value_box)
    ax.text(9.25, 4, 'Value Estimate V(s)', ha='center', va='center', 
            fontsize=12, fontweight='bold', color='white')
    ax.text(9.25, 3.7, '(Game Outcome Prediction)', ha='center', va='center', 
            fontsize=9, color='white', style='italic')
    
    # Arrows to MCTS
    arrow5 = FancyArrowPatch((2.75, 3.5), (3.5, 3), arrowstyle='->', 
                             mutation_scale=25, linewidth=2.5, color='black')
    arrow6 = FancyArrowPatch((9.25, 3.5), (8.5, 3), arrowstyle='->', 
                             mutation_scale=25, linewidth=2.5, color='black')
    ax.add_patch(arrow5)
    ax.add_patch(arrow6)
    
    # Multi-Observer IS-MCTS (detailed)
    mcts_box = FancyBboxPatch((1.5, 1.2), 9, 1.5, boxstyle=box_style, 
                              facecolor=mcts_color, edgecolor='black', 
                              linewidth=3, alpha=0.9)
    ax.add_patch(mcts_box)
    ax.text(6, 2.3, 'Multi-Observer IS-MCTS', ha='center', va='center', 
            fontsize=15, fontweight='bold', color='white')
    ax.text(6, 1.9, 'Selection (PUCT) → Expansion (Policy Prior) → Simulation (Transformer Rollouts) → Backpropagation', 
            ha='center', va='center', fontsize=9, color='white', style='italic')
    ax.text(6, 1.6, 'Per-player belief states maintained throughout search', 
            ha='center', va='center', fontsize=9, color='white', style='italic')
    
    arrow7 = FancyArrowPatch((6, 1.2), (6, 0.5), arrowstyle='->', 
                             mutation_scale=30, linewidth=3, color='black')
    ax.add_patch(arrow7)
    
    # Action Selection
    action_box = FancyBboxPatch((4, 0), 4, 0.4, boxstyle=box_style, 
                               facecolor=action_color, edgecolor='black', 
                               linewidth=2, alpha=0.9)
    ax.add_patch(action_box)
    ax.text(6, 0.2, 'Action Selection (Most Visited)', ha='center', va='center', 
            fontsize=12, fontweight='bold', color='white')
    
    plt.tight_layout()
    plt.savefig('system_pipeline_detailed.png', dpi=300, bbox_inches='tight', 
                facecolor='white', edgecolor='none')
    print("✓ Detailed pipeline diagram saved as 'system_pipeline_detailed.png'")

if __name__ == "__main__":
    create_pipeline_diagram()
    create_detailed_pipeline_diagram()
    print("\n✓ All pipeline diagrams generated!")


