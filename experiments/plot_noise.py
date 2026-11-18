"""Plot the one-pixel disparity offsets reported by stereo_noise."""

import argparse
import csv
import io
import math
from pathlib import Path


COLUMNS = {
    "true_depth_m",
    "disparity_offset_px",
    "observed_disparity_px",
    "signed_depth_error_m",
    "linearized_depth_error_m",
    "status",
}


def read_errors(path):
    series = {-1: {}, 1: {}}
    with path.open(encoding="utf-8-sig", newline="") as source:
        rows = csv.DictReader(source, strict=True)
        if not COLUMNS.issubset(rows.fieldnames or ()):
            raise ValueError("expected columns from stereo_noise")
        for row in rows:
            try:
                if None in row or any(row[name] is None for name in COLUMNS):
                    raise ValueError("wrong number of CSV fields")
                offset = int(row["disparity_offset_px"])
                if offset not in series:
                    continue

                depth = float(row["true_depth_m"])
                disparity = float(row["observed_disparity_px"])
                if not math.isfinite(depth) or depth <= 0 or not math.isfinite(disparity):
                    raise ValueError("expected positive finite depth and finite disparity")
                if depth in series[offset]:
                    raise ValueError("duplicate depth and disparity offset")

                status = row["status"]
                actual = row["signed_depth_error_m"]
                linear = row["linearized_depth_error_m"]
                if disparity > 0 and status == "ok":
                    errors = (float(actual), float(linear))
                    if not all(math.isfinite(error) for error in errors):
                        raise ValueError("errors must be finite for positive disparity")
                elif disparity <= 0 and status == "invalid_disparity" and actual == linear == "":
                    # NaN leaves a gap in the line at this depth.
                    errors = (math.nan, math.nan)
                else:
                    raise ValueError("status or errors disagree with observed disparity")
                series[offset][depth] = errors
            except ValueError as error:
                raise ValueError(f"CSV line {rows.line_num}: {error}") from error

    if not series[-1] or series[-1].keys() != series[1].keys():
        raise ValueError("expected a −1 px and +1 px row at every plotted depth")
    return series


def plot_errors(series, output):
    from matplotlib import use

    use("Agg")
    from matplotlib import pyplot as plt

    depths = sorted(series[-1])
    with plt.rc_context({"svg.fonttype": "none", "svg.hashsalt": "stereo-depth-lab"}):
        fig, ax = plt.subplots(figsize=(8.2, 4.6), layout="constrained")
        ax.grid(axis="y", color="#e1e7ee")
        ax.set_axisbelow(True)
        ax.axhline(0, color="#657382", linewidth=0.8)
        for offset, label, color in [(-1, "−1 px", "#175294"), (1, "+1 px", "#a33b20")]:
            actual, linear = zip(*(series[offset][depth] for depth in depths))
            ax.plot(depths, actual, color=color, marker="o", label=f"{label} reconstructed")
            ax.plot(depths, linear, color=color, linestyle="--", label=f"{label} linearized")

        ax.set_xscale("log", base=2)
        ax.set_xticks(depths, [f"{depth:g}" for depth in depths])
        ax.tick_params(axis="x", which="minor", bottom=False, labelbottom=False)
        ax.set_title("Depth error from a one-pixel disparity offset", loc="left", pad=12)
        ax.set_xlabel("True depth (m, log scale)")
        ax.set_ylabel("Estimated depth − true depth (m)")
        ax.spines[["right", "top"]].set_visible(False)
        ax.legend(loc="upper left", frameon=False, ncols=2)
        svg = io.StringIO()
        fig.savefig(svg, format="svg", metadata={"Date": None})
        plt.close(fig)
    output.write_text("\n".join(line.rstrip() for line in svg.getvalue().splitlines()) + "\n",
                      encoding="utf-8", newline="\n")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("csv", type=Path, help="CSV output of stereo_noise")
    parser.add_argument("svg", type=Path, help="output SVG path")
    args = parser.parse_args()
    if args.svg.suffix.lower() != ".svg":
        parser.error("output path must end in .svg")
    try:
        plot_errors(read_errors(args.csv), args.svg)
    except (csv.Error, ImportError, OSError, ValueError) as error:
        parser.exit(1, f"{parser.prog}: {error}\n")


if __name__ == "__main__":
    main()
