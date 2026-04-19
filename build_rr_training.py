#!/usr/bin/env python3
# Brent Ortizo and Kayode Binitie
import math
import csv


def load_rr_results(filename):
    results = []

    with open(filename, "r", encoding="utf-8") as file_obj:
        reader = csv.DictReader(file_obj)
        for row in reader:
            results.append(row)

    return results


def find_best_quantum_per_task(rr_results):
    best_results = {}

    for row in rr_results:
        task = row["taskset"]
        quantum = int(row["quantum"])
        avg_turnaround = float(row["avg_turnaround"])

        if task not in best_results:
            best_results[task] = {
                "quantum": quantum,
                "avg_turnaround": avg_turnaround
            }
        else:
            if avg_turnaround < best_results[task]["avg_turnaround"]:
                best_results[task] = {
                    "quantum": quantum,
                    "avg_turnaround": avg_turnaround
                }

    return best_results


def parse_task_file(task_filename):
    burst_times = []
    arrival_times = []

    with open(task_filename, "r", encoding="utf-8") as file_obj:
        lines = file_obj.readlines()

    # Remove header
    if len(lines) > 0:
        lines.pop(0)

    for row in lines:
        row = row.strip()
        if row == "":
            continue

        parts = row.split()

        # Expected format: PID Arrival Burst Priority
        arrival = int(parts[1])
        burst = int(parts[2])

        arrival_times.append(arrival)
        burst_times.append(burst)

    return burst_times, arrival_times


def compute_workload_features(burst_times, arrival_times):
    num_processes = len(burst_times)

    avg_burst = sum(burst_times) / num_processes
    max_burst = max(burst_times)
    min_burst = min(burst_times)

    variance = 0.0
    for burst in burst_times:
        variance += (burst - avg_burst) ** 2
    variance /= num_processes
    std_burst = math.sqrt(variance)

    if num_processes > 1:
        sorted_arrivals = sorted(arrival_times)
        gaps = []
        for i in range(1, len(sorted_arrivals)):
            gaps.append(sorted_arrivals[i] - sorted_arrivals[i - 1])
        avg_arrival_gap = sum(gaps) / len(gaps)
    else:
        avg_arrival_gap = 0.0

    return (
        num_processes,
        avg_burst,
        max_burst,
        min_burst,
        std_burst,
        avg_arrival_gap
    )


def main():
    rr_results = load_rr_results("rr_results.csv")
    best_results = find_best_quantum_per_task(rr_results)

    with open("rr_training_data.csv", "w", encoding="utf-8", newline="") as training_file:
        writer = csv.writer(training_file)

        # EXACT columns expected by ml_rr_predict.py
        writer.writerow([
            "num_processes",
            "avg_burst",
            "max_burst",
            "min_burst",
            "std_burst",
            "avg_arrival_gap",
            "optimal_quantum"
        ])

        for task in best_results:
            burst_times, arrival_times = parse_task_file(task)

            (
                num_processes,
                avg_burst,
                max_burst,
                min_burst,
                std_burst,
                avg_arrival_gap
            ) = compute_workload_features(burst_times, arrival_times)

            optimal_quantum = best_results[task]["quantum"]

            writer.writerow([
                num_processes,
                avg_burst,
                max_burst,
                min_burst,
                std_burst,
                avg_arrival_gap,
                optimal_quantum
            ])

    print("Created rr_training_data.csv successfully.")


if __name__ == "__main__":
    main()