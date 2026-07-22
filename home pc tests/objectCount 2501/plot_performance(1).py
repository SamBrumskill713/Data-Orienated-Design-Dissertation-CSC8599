#!/usr/bin/env python3
"""Create dissertation-ready charts from engine performance CSV files.

Examples:
    python plot_performance.py upload/performance_OOP.csv
    python plot_performance.py performance_OOP.csv performance_AOS.csv performance_SOA.csv
    python plot_performance.py *.csv --output-dir charts --relative-time

Expected columns:
    Elapsed_Time_s, Avg_FPS, Avg_Frame_Time_ms,
    Avg_Physics_Time_ms, Avg_Render_Time_ms, Object_Count
"""

from __future__ import annotations

import argparse
import csv
import re
import statistics
import sys
from collections import defaultdict
from pathlib import Path

import matplotlib.pyplot as plt


METRICS = {
    "Avg_FPS": ("Average frame rate", "Frames per second (FPS)"),
    "Avg_Frame_Time_ms": ("Average frame time", "Time (ms)"),
    "Avg_Physics_Time_ms": ("Average physics-system time", "Time (ms)"),
    "Avg_Render_Time_ms": ("Average rendering-system time", "Time (ms)"),
}

COLOURS = {
    "OOP": "#0072B2",
    "AOS": "#D55E00",
    "SOA": "#009E73",
}

# Accepted CSV headings for the wasted-memory measurement. Headings are
# normalised before matching, so spaces, underscores and capitalisation do not
# matter (for example, "Wasted Memory (MB)" and "Wasted_Memory_MB" both work).
WASTED_MEMORY_COLUMNS = {
    "wastedmemorymb": 1.0,
    "wastedmemorymib": 1.0,
    "wastedmb": 1.0,
    "wastedmib": 1.0,
    "unusedmemorymb": 1.0,
    "unusedmemorymib": 1.0,
    "wastedmemorykb": 1.0 / 1000.0,
    "wastedmemorybytes": 1.0 / 1_000_000.0,
    "wastedbytes": 1.0 / 1_000_000.0,
}


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Plot OOP, AoS and SoA performance measurements."
    )
    parser.add_argument(
        "csv_files",
        nargs="+",
        type=Path,
        help="One or more performance CSV files.",
    )
    parser.add_argument(
        "--labels",
        nargs="+",
        help="Optional labels in the same order as the input files.",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("charts"),
        help="Directory for generated figures (default: charts).",
    )
    parser.add_argument(
        "--output-prefix",
        default="performance",
        help="Prefix for output image names (default: performance).",
    )
    parser.add_argument(
        "--title",
        help="Optional experiment title displayed above both figures.",
    )
    parser.add_argument(
        "--garbage-floats",
        type=int,
        metavar="COUNT",
        help="Add the number of unused float values per object to the title.",
    )
    parser.add_argument(
        "--relative-time",
        action="store_true",
        help="Make the first recorded sample start at 0 seconds.",
    )
    parser.add_argument(
        "--sample-index",
        action="store_true",
        help="Use consecutive one-second measurement intervals for the x-axis.",
    )
    parser.add_argument(
        "--show",
        action="store_true",
        help="Open the figures after saving them.",
    )
    args = parser.parse_args()

    if args.labels and len(args.labels) != len(args.csv_files):
        parser.error("--labels must contain exactly one label per CSV file")
    return args


def infer_label(path: Path) -> str:
    """Infer OOP/AoS/SoA from names such as performance_OOP_trial_1.csv."""
    name = path.stem
    name = re.sub(r"^performance[_-]?", "", name, flags=re.IGNORECASE)
    name = re.sub(r"[_-]?trial[_-]?\d+$", "", name, flags=re.IGNORECASE)
    label = name.upper()
    return {"AOS": "AoS", "SOA": "SoA"}.get(label, label)


def colour_for(label: str, index: int) -> str:
    key = label.upper()
    if key in COLOURS:
        return COLOURS[key]
    palette = plt.get_cmap("tab10")
    return palette(index % 10)


def normalise_heading(heading: str) -> str:
    """Remove punctuation and case differences from a CSV heading."""
    return re.sub(r"[^a-z0-9]", "", heading.lower())


def find_wasted_memory_column(fieldnames: list[str]) -> tuple[str, float] | None:
    """Return the original heading and its conversion factor to MB."""
    for heading in fieldnames:
        normalised = normalise_heading(heading)
        if normalised in WASTED_MEMORY_COLUMNS:
            return heading, WASTED_MEMORY_COLUMNS[normalised]
    return None


def load_csv(path: Path, label: str, relative_time: bool) -> dict:
    required = {"Elapsed_Time_s", "Object_Count", *METRICS.keys()}
    try:
        with path.open(newline="", encoding="utf-8-sig") as handle:
            reader = csv.DictReader(handle)
            missing = required.difference(reader.fieldnames or [])
            if missing:
                raise ValueError(
                    f"{path}: missing required columns: {', '.join(sorted(missing))}"
                )

            fieldnames = reader.fieldnames or []
            wasted_memory_column = find_wasted_memory_column(fieldnames)
            rows = list(reader)
    except OSError as error:
        raise SystemExit(f"Could not read {path}: {error}") from error

    if not rows:
        raise ValueError(f"{path}: file contains no measurements")

    try:
        elapsed = [float(row["Elapsed_Time_s"]) for row in rows]
        values = {
            metric: [float(row[metric]) for row in rows] for metric in METRICS
        }
        object_counts = {int(float(row["Object_Count"])) for row in rows}
        wasted_memory_mb: set[float] = set()
        if wasted_memory_column is not None:
            heading, conversion_to_mb = wasted_memory_column
            wasted_memory_mb = {
                float(row[heading]) * conversion_to_mb
                for row in rows
                if row.get(heading, "").strip()
            }
    except (TypeError, ValueError) as error:
        raise ValueError(f"{path}: a measurement is not numeric") from error

    if relative_time:
        start = elapsed[0]
        elapsed = [time - start for time in elapsed]

    return {
        "path": path,
        "label": label,
        "elapsed": elapsed,
        "values": values,
        "object_counts": object_counts,
        "wasted_memory_mb": wasted_memory_mb,
    }


def style_axis(axis: plt.Axes) -> None:
    axis.grid(True, color="#D9D9D9", linewidth=0.7, alpha=0.8)
    axis.set_axisbelow(True)
    axis.spines["top"].set_visible(False)
    axis.spines["right"].set_visible(False)


def experiment_title(datasets: list[dict], args: argparse.Namespace) -> str:
    if args.title:
        title = args.title
    else:
        labels = " vs ".join(dict.fromkeys(d["label"] for d in datasets))
        title = f"{labels} performance"

    if args.garbage_floats is not None:
        title += f" — {args.garbage_floats:,} unused floats per object"
    return title


def format_wasted_memory(datasets: list[dict]) -> str:
    """Format CSV wasted-memory values for the figure heading."""
    by_label: dict[str, set[float]] = defaultdict(set)
    all_values: set[float] = set()

    for dataset in datasets:
        values = dataset["wasted_memory_mb"]
        by_label[dataset["label"]].update(values)
        all_values.update(values)

    if not all_values:
        return ""

    def format_values(values: set[float]) -> str:
        ordered = sorted(values)
        if len(ordered) == 1:
            return f"{ordered[0]:,.2f} MB"
        return f"{ordered[0]:,.2f}-{ordered[-1]:,.2f} MB"

    # If every input reports the same value, avoid repeating it per label.
    if len(all_values) == 1:
        return f"; wasted memory: {format_values(all_values)}"

    parts = [
        f"{label}: {format_values(values)}"
        for label, values in by_label.items()
        if values
    ]
    return f"; wasted memory: {', '.join(parts)}" if parts else ""


def plot_time_series(
    datasets: list[dict], output_dir: Path, args: argparse.Namespace
) -> Path:
    figure, axes = plt.subplots(2, 2, figsize=(11.5, 7.5), constrained_layout=True)
    flat_axes = axes.flatten()

    repeated_labels = {
        label for label in {d["label"] for d in datasets}
        if sum(d["label"] == label for d in datasets) > 1
    }

    for index, dataset in enumerate(datasets):
        legend_label = dataset["label"]
        if dataset["label"] in repeated_labels:
            legend_label = f"{dataset['label']} ({dataset['path'].stem})"
        colour = colour_for(dataset["label"], index)

        for axis, (metric, (title, y_label)) in zip(flat_axes, METRICS.items()):
            axis.plot(
                dataset["elapsed"],
                dataset["values"][metric],
                color=colour,
                linewidth=1.8,
                label=legend_label,
            )
            axis.set_title(title)
            axis.set_xlabel(dataset["x_label"])
            axis.set_ylabel(y_label)
            style_axis(axis)

    for axis in flat_axes:
        axis.legend(frameon=False)

    counts = sorted({count for d in datasets for count in d["object_counts"]})
    count_text = ", ".join(f"{count:,}" for count in counts)
    wasted_memory_text = format_wasted_memory(datasets)
    figure.suptitle(
        f"{experiment_title(datasets, args)}\n"
        f"Performance over time ({count_text} objects{wasted_memory_text})",
        fontsize=14,
    )

    output_path = output_dir / f"{args.output_prefix}_time_series.png"
    figure.savefig(output_path, dpi=300, bbox_inches="tight")
    return output_path


def plot_summary(
    datasets: list[dict], output_dir: Path, args: argparse.Namespace
) -> Path:
    """Plot trial means; error bars represent SD between trial means."""
    grouped: dict[str, list[dict]] = defaultdict(list)
    for dataset in datasets:
        grouped[dataset["label"]].append(dataset)

    labels = list(grouped)
    figure, axes = plt.subplots(2, 2, figsize=(10.5, 7.2), constrained_layout=True)

    for axis, (metric, (title, y_label)) in zip(axes.flatten(), METRICS.items()):
        means = []
        errors = []
        colours = []

        for index, label in enumerate(labels):
            trial_means = [
                statistics.fmean(dataset["values"][metric])
                for dataset in grouped[label]
            ]
            means.append(statistics.fmean(trial_means))
            errors.append(
                statistics.stdev(trial_means) if len(trial_means) > 1 else 0.0
            )
            colours.append(colour_for(label, index))

        axis.bar(
            labels,
            means,
            yerr=errors if any(errors) else None,
            capsize=5,
            color=colours,
            edgecolor="#333333",
            linewidth=0.7,
        )
        axis.set_title(title)
        axis.set_ylabel(y_label)
        style_axis(axis)

        for position, value in enumerate(means):
            axis.annotate(
                f"{value:.2f}",
                (position, value),
                xytext=(0, 4),
                textcoords="offset points",
                ha="center",
                va="bottom",
                fontsize=9,
            )

    max_trials = max(len(trials) for trials in grouped.values())
    subtitle = "Mean across recorded samples"
    if max_trials > 1:
        subtitle += "; error bars show SD between trial means"
    counts = sorted({count for d in datasets for count in d["object_counts"]})
    count_text = ", ".join(f"{count:,}" for count in counts)
    wasted_memory_text = format_wasted_memory(datasets)
    figure.suptitle(
        f"{experiment_title(datasets, args)}\n"
        f"Performance summary ({count_text} objects{wasted_memory_text})\n"
        f"{subtitle}",
        fontsize=13,
    )

    output_path = output_dir / f"{args.output_prefix}_summary.png"
    figure.savefig(output_path, dpi=300, bbox_inches="tight")
    return output_path


def print_summary(datasets: list[dict]) -> None:
    print("\nMean values per input file")
    print("-" * 86)
    print(
        f"{'Dataset':<24}{'FPS':>12}{'Frame (ms)':>14}"
        f"{'Physics (ms)':>18}{'Render (ms)':>17}"
    )
    for dataset in datasets:
        values = dataset["values"]
        print(
            f"{dataset['path'].stem:<24}"
            f"{statistics.fmean(values['Avg_FPS']):>12.2f}"
            f"{statistics.fmean(values['Avg_Frame_Time_ms']):>14.2f}"
            f"{statistics.fmean(values['Avg_Physics_Time_ms']):>18.2f}"
            f"{statistics.fmean(values['Avg_Render_Time_ms']):>17.3f}"
        )


def main() -> None:
    args = parse_arguments()
    labels = args.labels or [infer_label(path) for path in args.csv_files]
    datasets = [
        load_csv(path, label, args.relative_time)
        for path, label in zip(args.csv_files, labels)
    ]

    non_monotonic = []
    for dataset in datasets:
        elapsed = dataset["elapsed"]
        if any(current <= previous for previous, current in zip(elapsed, elapsed[1:])):
            non_monotonic.append(dataset["path"])

    use_sample_index = args.sample_index or bool(non_monotonic)
    if non_monotonic and not args.sample_index:
        names = ", ".join(str(path) for path in non_monotonic)
        print(
            "Warning: elapsed time is not strictly increasing in "
            f"{names}. Using measurement intervals on the x-axis instead.",
            file=sys.stderr,
        )

    for dataset in datasets:
        if use_sample_index:
            dataset["elapsed"] = list(range(1, len(dataset["elapsed"]) + 1))
            dataset["x_label"] = "Measurement interval (approximately 1 s)"
        else:
            dataset["x_label"] = "Elapsed time (s)"

    args.output_dir.mkdir(parents=True, exist_ok=True)
    time_series_path = plot_time_series(datasets, args.output_dir, args)
    summary_path = plot_summary(datasets, args.output_dir, args)
    print_summary(datasets)
    print(f"\nSaved: {time_series_path}")
    print(f"Saved: {summary_path}")

    if args.show:
        plt.show()
    else:
        plt.close("all")


if __name__ == "__main__":
    main()
