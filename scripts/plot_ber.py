#!/usr/bin/env python3
"""
Plot simulated vs. theoretical BER curves produced by nr_phy_sim /
nr_phy_sim_all_mod. Reads results/ber_results_bps{2,4,6}.csv (falls back to
results/ber_results.csv for a single-modulation run) and produces
results/ber_curve.png -- the core proof-of-correctness artifact for the
project README / CV.
"""
import pandas as pd
import matplotlib.pyplot as plt
import os

RESULTS_DIR = os.path.join(os.path.dirname(__file__), "..", "results")

MOD_LABELS = {2: "QPSK", 4: "16-QAM", 6: "64-QAM"}
COLORS = {2: "#1f77b4", 4: "#d62728", 6: "#2ca02c"}


def main():
    fig, ax = plt.subplots(figsize=(8, 6))

    found_any = False
    for bps, label in MOD_LABELS.items():
        path = os.path.join(RESULTS_DIR, f"ber_results_bps{bps}.csv")
        if not os.path.exists(path):
            continue
        found_any = True
        df = pd.read_csv(path)

        # Simulated: markers, only where BER > 0 (log scale can't show 0)
        sim = df[df["simulated_ber"] > 0]
        ax.semilogy(sim["snr_db"], sim["simulated_ber"], "o",
                    color=COLORS[bps], label=f"{label} simulated", markersize=6)

        # Theoretical: solid line
        ax.semilogy(df["snr_db"], df["theoretical_ber"], "-",
                    color=COLORS[bps], label=f"{label} theoretical", linewidth=1.5, alpha=0.7)

    if not found_any:
        single_path = os.path.join(RESULTS_DIR, "ber_results.csv")
        df = pd.read_csv(single_path)
        sim = df[df["simulated_ber"] > 0]
        ax.semilogy(sim["snr_db"], sim["simulated_ber"], "o", color="#1f77b4", label="Simulated")
        ax.semilogy(df["snr_db"], df["theoretical_ber"], "-", color="#1f77b4", label="Theoretical", alpha=0.7)

    ax.set_xlabel("Eb/N0 (dB)", fontsize=12)
    ax.set_ylabel("Bit Error Rate", fontsize=12)
    ax.set_title("OFDM Link-Level BER Performance: Simulated vs. Theoretical AWGN Bound", fontsize=12)
    ax.grid(True, which="both", linestyle="--", alpha=0.4)
    ax.set_ylim(1e-6, 1)
    ax.legend(fontsize=9)
    fig.tight_layout()

    out_path = os.path.join(RESULTS_DIR, "ber_curve.png")
    fig.savefig(out_path, dpi=150)
    print(f"Saved plot to {out_path}")


if __name__ == "__main__":
    main()
