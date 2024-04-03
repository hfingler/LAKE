#!/usr/bin/env python

import os

# Initialize variables to store cumulative sums
total_avg_read_latency = 0
total_read_latency_p95 = 0
total_read_latency_p99 = 0
total_avg_write_latency = 0
total_io_inter_arrival_time = 0

# Count the number of entries
count = 0

# Change to user's directory
os.chdir("/home/cyuan/LAKE/src/linnos/io_replayer")

# Change to the specific policy's output file
with open("baseline_random_cancel_mix.out", "r") as file:
    lines = file.readlines()
    for i in range(0, len(lines), 6):
        count += 1
        avg_read_latency = float(lines[i+1].split(":")[1].strip().split()[0])
        read_latency_p95 = float(lines[i+2].split(":")[1].strip().split()[0])
        read_latency_p99 = float(lines[i+3].split(":")[1].strip().split()[0])
        avg_write_latency = float(lines[i+4].split(":")[1].strip().split()[0])
        io_inter_arrival_time = float(lines[i+5].split(":")[1].strip().split()[0])

        # Add to cumulative sums
        total_avg_read_latency += avg_read_latency
        total_read_latency_p95 += read_latency_p95
        total_read_latency_p99 += read_latency_p99
        total_avg_write_latency += avg_write_latency
        total_io_inter_arrival_time += io_inter_arrival_time

# Calculate averages
average_avg_read_latency = total_avg_read_latency / count
average_read_latency_p95 = total_read_latency_p95 / count
average_read_latency_p99 = total_read_latency_p99 / count
average_avg_write_latency = total_avg_write_latency / count
average_io_inter_arrival_time = total_io_inter_arrival_time / count

# Print results
print("Average read latency:", average_avg_read_latency, "us")
print("Average read latency p95:", average_read_latency_p95, "us")
print("Average read latency p99:", average_read_latency_p99, "us")
print("Average write latency:", average_avg_write_latency, "us")
print("Average IO inter arrival time:", average_io_inter_arrival_time, "us")
