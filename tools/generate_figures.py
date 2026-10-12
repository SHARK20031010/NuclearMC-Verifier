#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
tools/generate_figures.py
Regenerates publication-quality Figure 1 and Figure 2 for NuclearMC-Verifier
with perfect alignment, no visual clipping/overlapping, and 300 DPI output.
"""

import matplotlib.pyplot as plt
import numpy as np
import os

plt.rcParams['font.family'] = 'DejaVu Sans'
plt.rcParams['font.size'] = 11
plt.rcParams['axes.linewidth'] = 1.0

OUT_DIR = "figures"
os.makedirs(OUT_DIR, exist_ok=True)


def plot_fig1():
    """Figure 1: Cross-Complexity Accuracy across 190 Benchmark Evaluations"""
    tiers = [
        "Tier 1 (Synthetic\nSingle-slot, 60)",
        "Tier 2 (Coupled\nMulti-vol, 60)",
        "Tier 3 (Deep\nBateman, 60)",
        "Wild Corpus\n(CERN/GitHub, 10)"
    ]
    
    arm_a = [41.7, 18.3, 23.3, 0.0]
    arm_b = [86.7, 36.7, 53.3, 0.0]
    arm_c = [100.0, 100.0, 100.0, 100.0]

    x = np.arange(len(tiers))
    width = 0.25

    fig, ax = plt.subplots(figsize=(8.8, 4.9), dpi=300)

    # Plot bars
    c_a = '#848fa5'  # Muted slate blue
    c_b = '#5193d5'  # Crisp academic blue
    c_c = '#e8782a'  # Vivid alert orange

    rects1 = ax.bar(x - width, arm_a, width, label='Arm A: Raw LLM Baseline',
                    color=c_a, edgecolor='black', linewidth=1.0, zorder=3)
    rects2 = ax.bar(x, arm_b, width, label='Arm B: Self-Reflection CoT',
                    color=c_b, edgecolor='black', linewidth=1.0, zorder=3)
    rects3 = ax.bar(x + width, arm_c, width, label='Arm C: Conservation Bus Guardrail (Ours)',
                    color=c_c, edgecolor='black', linewidth=1.0, zorder=3)

    # Reference dashed line at 100%
    ax.axhline(100.0, color='gray', linestyle='--', linewidth=0.9, alpha=0.7, zorder=2)

    # Subtle horizontal grid
    ax.yaxis.grid(True, linestyle=':', alpha=0.5, zorder=1)

    # Add data labels on top of bars
    def autolabel(rects):
        for rect in rects:
            height = rect.get_height()
            if height == 0.0:
                ax.annotate('0.0%',
                            xy=(rect.get_x() + rect.get_width() / 2, 1.8),
                            xytext=(0, 0), textcoords="offset points",
                            ha='center', va='bottom', fontsize=10.5, fontweight='bold', zorder=4)
            else:
                ax.annotate(f'{height:.1f}%',
                            xy=(rect.get_x() + rect.get_width() / 2, height),
                            xytext=(0, 2.5), textcoords="offset points",
                            ha='center', va='bottom', fontsize=10.5, fontweight='bold', zorder=4)

    autolabel(rects1)
    autolabel(rects2)
    autolabel(rects3)

    ax.set_ylabel('Physical Accuracy Rate (%)', fontsize=13, fontweight='bold', labelpad=8)
    ax.set_title('Cross-Complexity Accuracy across 190 Benchmark Evaluations',
                 fontsize=15, fontweight='bold', pad=12)
    ax.set_xticks(x)
    ax.set_xticklabels(tiers, fontsize=11)
    
    # Expand ylim to 126 and place horizontal legend cleanly above the bars
    ax.set_ylim(0, 126)
    ax.set_yticks(np.arange(0, 101, 20))
    ax.tick_params(axis='both', which='major', labelsize=11)

    # Horizontal 3-column legend at the top center with zero bar clashing
    ax.legend(loc='upper center', bbox_to_anchor=(0.5, 0.98), ncol=3,
              framealpha=0.95, edgecolor='#d0d0d0', fontsize=10.2, handletextpad=0.5, columnspacing=1.2)

    plt.tight_layout()
    png_path = os.path.join(OUT_DIR, "fig1_accuracy_gradient.png")
    pdf_path = os.path.join(OUT_DIR, "fig1_accuracy_gradient.pdf")
    plt.savefig(png_path, dpi=300)
    plt.savefig(pdf_path)
    plt.close()
    print(f"Generated: {png_path} and {pdf_path}")


def plot_fig2():
    """Figure 2: Error Distribution across Core Actions & Failure Slots"""
    actions = ['Define', 'Sample', 'Fetch', 'Accumulate', 'Convert']
    counts = [4, 9, 6, 8, 8]
    colors = ['#3f72af', '#e8782a', '#95a5a6', '#f39c12', '#5dade2']

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(10.0, 4.5), dpi=300)

    # (a) Bar Chart
    bars = ax1.bar(actions, counts, color=colors, edgecolor='black', linewidth=1.1, width=0.68, zorder=3)
    ax1.yaxis.grid(True, linestyle=':', alpha=0.5, zorder=1)
    for bar in bars:
        h = bar.get_height()
        ax1.annotate(f'{h}',
                     xy=(bar.get_x() + bar.get_width() / 2, h),
                     xytext=(0, 4), textcoords="offset points",
                     ha='center', va='bottom', fontsize=12, fontweight='bold', zorder=4)

    ax1.set_ylabel('Critical Physical Errors (Arm A)', fontsize=13, fontweight='bold', labelpad=8)
    ax1.set_title('(a) Errors by Core MC Action', fontsize=14, fontweight='bold', pad=12)
    ax1.set_ylim(0, 12)
    ax1.set_yticks(np.arange(0, 13, 2))
    ax1.tick_params(axis='both', which='major', labelsize=11)

    # (b) Donut Chart
    wedges, texts, autotexts = ax2.pie(
        counts,
        labels=actions,
        autopct='%1.1f%%',
        startangle=140,
        colors=colors,
        pctdistance=0.72,
        textprops=dict(color="black", fontsize=11.5),
        wedgeprops=dict(width=0.45, edgecolor='black', linewidth=1.1)
    )

    for autotext in autotexts:
        autotext.set_fontsize(11)
        autotext.set_fontweight('bold')

    ax2.set_title('(b) Proportion across 31 Failure Slots', fontsize=14, fontweight='bold', pad=12)

    plt.tight_layout()
    png_path = os.path.join(OUT_DIR, "fig2_action_slots_distribution.png")
    pdf_path = os.path.join(OUT_DIR, "fig2_action_slots_distribution.pdf")
    plt.savefig(png_path, dpi=300)
    plt.savefig(pdf_path)
    plt.close()
    print(f"Generated: {png_path} and {pdf_path}")


if __name__ == "__main__":
    plot_fig1()
    plot_fig2()
