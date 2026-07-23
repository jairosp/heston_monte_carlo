from pathlib import Path
import pandas as pd
import matplotlib.pyplot as plt


def setup_directories() -> tuple[Path, Path]:
    script_dir = Path(__file__).resolve().parent
    project_root = script_dir.parent

    csv_path = project_root / "benchmarks" / "benchmarks.csv"
    output_dir = project_root / "benchmarks" / "reports"
    output_dir.mkdir(parents=True, exist_ok=True)

    return csv_path, output_dir


def apply_paper_style():
    plt.rcParams.update({
        "font.size": 11,
        "axes.titlesize": 15,
        "axes.labelsize": 12,
        "legend.fontsize": 11,
        "xtick.labelsize": 10,
        "ytick.labelsize": 10,
        "figure.dpi": 120
    })


def style_axis(ax):
    ax.grid(
        True,
        which="major",
        linestyle="--",
        alpha=0.20
    )

    ax.spines["top"].set_visible(False)
    ax.spines["right"].set_visible(False)


def main():
    csv_path, output_dir = setup_directories()

    apply_paper_style()

    df = pd.read_csv(csv_path)

    euler = (
        df[(df["Scheme"] == "EulerMaruyama") & (df["Engine"] == "CPU")]
        .sort_values("Paths")
        .copy()
    )

    euler_parallel = (
        df[(df["Scheme"] == "EulerMaruyama") & (df["Engine"] == "CPU_Parallel")]
        .sort_values("Paths")
        .copy()
    )

    qe = (
        df[(df["Scheme"] == "QE") & (df["Engine"] == "CPU")]
        .sort_values("Paths")
        .copy()
    )

    qe_parallel = (
        df[(df["Scheme"] == "QE") & (df["Engine"] == "CPU_Parallel")]
        .sort_values("Paths")
        .copy()
    )
    

    # ============================================================
    # FIGURE 1 : PRICE VS PATHS
    # ============================================================

    fig, ax = plt.subplots(figsize=(10, 6))

    to_plot = [euler, qe]
    labels = ["EM", "QE"]


    for case, lab in zip(to_plot, labels):
        ax.plot(
            case["Paths"],
            case["Price"],
            marker="o",
            linewidth=2.5,
            label=lab
        )

        ax.fill_between(
            case["Paths"],
            case["Price"] - 1.96 * case["StdError"],
            case["Price"] + 1.96 * case["StdError"],
            alpha=0.20
        )   

    ax.set_xscale("log")

    ax.set_title(
        "Monte Carlo Price Estimates vs Number of Paths"
    )

    ax.set_xlabel("Number of Paths")
    ax.set_ylabel("Option Price")

    style_axis(ax)

    ax.legend(frameon=False)

    fig.tight_layout()

    fig.savefig(
        output_dir / "price_vs_paths.png",
        dpi=300,
        bbox_inches="tight"
    )

    # ============================================================
    # FIGURE 2 : Execution Time (Euler)
    # ============================================================

    fig, ax = plt.subplots(figsize=(10, 6))

    to_plot = [euler, euler_parallel]
    labels = ["EM", "EM Parallel CPU"]


    for case, lab in zip(to_plot, labels):
        ax.plot(
            case["Paths"],
            case["TimeSeconds"],
            marker="o",
            linewidth=2.5,
            label=lab
        ) 

    ax.set_xscale("log")
    ax.set_yscale("log")

    ax.set_title(
        "Simulation Time vs Number of Paths"
    )

    ax.set_xlabel("Number of Paths")
    ax.set_ylabel("Time (Seconds)")

    style_axis(ax)

    ax.legend(frameon=False)

    fig.tight_layout()

    fig.savefig(
        output_dir / "time_vs_paths_em.png",
        dpi=300,
        bbox_inches="tight"
    )

    # ============================================================
    # FIGURE 2 : Execution Time (QE)
    # ============================================================

    fig, ax = plt.subplots(figsize=(10, 6))

    to_plot = [qe, qe_parallel]
    labels = ["QE", "QE Parallel CPU"]


    for case, lab in zip(to_plot, labels):
        ax.plot(
            case["Paths"],
            case["TimeSeconds"],
            marker="o",
            linewidth=2.5,
            label=lab
        ) 

    ax.set_xscale("log")
    ax.set_yscale("log")

    ax.set_title(
        "Simulation Time vs Number of Paths"
    )

    ax.set_xlabel("Number of Paths")
    ax.set_ylabel("Time (Seconds)")

    style_axis(ax)

    ax.legend(frameon=False)

    fig.tight_layout()

    fig.savefig(
        output_dir / "time_vs_paths_qe.png",
        dpi=300,
        bbox_inches="tight"
    )

    # ============================================================
    # FIGURE 4 : STANDARD ERROR VS PATHS
    # ============================================================

    fig, ax = plt.subplots(figsize=(10, 6))

    ax.plot(
        euler["Paths"],
        euler["StdError"],
        marker="o",
        linewidth=2.5,
        label="Euler-Maruyama"
    )

    ax.plot(
        qe["Paths"],
        qe["StdError"],
        marker="s",
        linewidth=2.5,
        label="Quadratic-Exponential"
    )

    ax.set_xscale("log")
    # ax.set_yscale("log")

    ax.set_title(
        "Monte Carlo Standard Error vs Number of Paths"
    )

    ax.set_xlabel("Number of Paths")
    ax.set_ylabel("Standard Error")

    style_axis(ax)

    ax.legend(frameon=False)

    fig.tight_layout()

    fig.savefig(
        output_dir / "stderr_vs_paths.png",
        dpi=300,
        bbox_inches="tight"
    )


if __name__ == "__main__":
    main()
    print("[Python] Benchmark plots completed!")