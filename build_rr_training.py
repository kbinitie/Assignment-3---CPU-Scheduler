# Open CSV file and read data

# Open the CSV file for reading
file_obj = open("rr_results.csv", "r")
data = file_obj.readlines()
file_obj.close()

# Put the data into a 2D list
rr_results = []
for row in data:
    # Remove \n character
    row = row.strip("\n")
    # Split using commas and append to the list
    rr_results.append(row.split(","))

# Remove the header row
rr_results.pop(0)

# print(rr_results)

# Collect the corresponding rows for each task and 
#   selects the quantum with the least turnaround time

# Create a dictionary to store the best quantum for each task
best_quantum = {}
for row in rr_results:
    task = row[0]
    quantum = int(row[1])
    avg_turnaround_time = float(row[2])
    
    if task not in best_quantum:
        best_quantum[task] = quantum
    else:
        if avg_turnaround_time < best_quantum[task]:
            best_quantum[task] = quantum

# Open each task file to compute the 
# - Number of processes
# - Average burst time
# - Maximum burst time
# - Minimum burst time
# - Standard deviation of burst times
# - Average arrival gap
# - optimal quantum

# Open file to store the training data
training_data_file = open("rr_training_data.csv", "w")
training_data_file.write("num_processes,avg_burst_time,max_burst_time,min_burst_time,std_dev_burst_time,avg_arrival_gap,optimal_quantum\n")

for task in best_quantum:
    file_obj = open(task, "r")
    data = file_obj.readlines()
    file_obj.close()
    
    # Remove the header row
    data.pop(0)

    # Create a list to store the burst times and arrival times
    burst_times = []
    arrival_times = []
    for row in data:
        row = row.strip("\n")
        row = row.split(",")
        burst_times.append(int(row[2]))
        arrival_times.append(int(row[1]))
    num_processes = len(burst_times)
    avg_burst_time = sum(burst_times) / num_processes
    max_burst_time = max(burst_times)
    min_burst_time = min(burst_times)
    std_dev_burst_time = (sum([(x - avg_burst_time) ** 2 for x in burst_times]) / num_processes) ** 0.5
    avg_arrival_gap = (max(arrival_times) - min(arrival_times)) / (num_processes - 1) if num_processes > 1 else 0
    optimal_quantum = best_quantum[task]

    # Write the training data to the file
    training_data_file.write(f"{num_processes},{avg_burst_time},{max_burst_time},{min_burst_time},{std_dev_burst_time},{avg_arrival_gap},{optimal_quantum}\n")
training_data_file.close()


    