import os
import re
import json
import argparse
from datetime import timedelta


def parse_time_string(time_str):
    time_pattern = r'(?:(\d+) days?, )?(?:(\d+) hrs?, )?(?:(\d+) min, )?(?:(\d+) sec)?'
    match = re.match(time_pattern, time_str)
    if match:
        days, hours, minutes, seconds = match.groups()
        return timedelta(
            days=int(days) if days else 0,
            hours=int(hours) if hours else 0,
            minutes=int(minutes) if minutes else 0,
            seconds=int(seconds) if seconds else 0
        )
    return None

def parse_log_file(file_path):
    with open(file_path, 'r') as f:
        log_content = f.read()

    blocks = re.findall(r'inputs:.*?(?=inputs:|Total mutation_inputs:)', log_content, re.DOTALL)
    if not blocks:
        return [], None

    last_block = blocks[-1]
    ranking_entries = re.findall(r'\[top-\d+\] inst_id: \d+, score: [\d\.]+, predicate: .*?, inst: .*? at .*?:\d+', last_block)
    ranking_info = []
    for entry in ranking_entries:
        match = re.search(r'\[top-(\d+)\] inst_id: (\d+), score: ([\d\.]+), predicate: (.*?), inst: (.*?) at (.*?:\d+)', entry)
        if match:
            rank, inst_id, score, predicate, inst, file_loc = match.groups()
            ranking_info.append({
                "rank": int(rank),
                "inst_id": inst_id,
                "score": float(score),
                "predicate": predicate,
                "inst": inst,
                "file_loc": file_loc
            })

    time_used_match = re.search(r'Finished at: (.*?)!', last_block)
    time_used = parse_time_string(time_used_match.group(1).strip()) if time_used_match else None

    return ranking_info, time_used

def get_ground_truth(file_path):
    with open(file_path, 'r') as f:
        lines = f.readlines()

    ground_truth = {}
    for line in lines:
        parts = line.strip().split()
        id = parts[0]
        location = parts[1]
        ground_truth[id] = location

    return ground_truth

def check_single(log_file_path, ground_truth_file_path, testcase_id):
    ranking_info, time_used = parse_log_file(log_file_path)
    ground_truth_loc = get_ground_truth(ground_truth_file_path).get(testcase_id)

    result = {
        "testcase_id": testcase_id,
        "log_file": log_file_path,
        "ground_truth_loc": ground_truth_loc,
        "best_ranking_info": None,
        "time_used": time_used
    }

    if ground_truth_loc:
        filtered_entries = [entry for entry in ranking_info if entry["file_loc"] == ground_truth_loc]
        if filtered_entries:
            best_ranking_info = filtered_entries[0]
            result["best_ranking_info"] = best_ranking_info

    return result

def calculate_averages(values):
    values = [v for v in values if v is not None]
    return round(sum(values) / len(values), 2) if values else None

def calculate_averages2(values):
    values = [v for v in values if v is not None]
    return sum(values, timedelta()) / len(values) if values else None

def process_testcase(base_path, testcase_name, ground_truth_file_path, num_runs):
    results = []
    for run in range(0, num_runs):
        log_file_path = os.path.join(base_path, testcase_name, f"afl-workdir-batch{run}", "ranked_file")
        if not os.path.exists(log_file_path):
            results.append(None)
            continue
        
        result = check_single(log_file_path, ground_truth_file_path, testcase_name)
        # results.append(result if result.get("best_ranking_info") else None)
        results.append(result)
    
    rankings = [result['best_ranking_info']['rank'] if result and result['best_ranking_info'] else None for result in results]
    times = [result['time_used'] if result and result['time_used'] else None for result in results]

    ranking_avg = calculate_averages(rankings)
    time_avg = calculate_averages2(times)

    return rankings + [ranking_avg], times + [time_avg]

def check_all(base_path, ground_truth_file_path, num_runs):
    ground_truth_map = get_ground_truth(ground_truth_file_path)

    columns = [f'ranking-{i}' for i in range(1, num_runs + 1)] + ['ranking-avg'] + \
              [f'time-{i}' for i in range(1, num_runs + 1)] + ['time-avg']
    header = ['testcase'] + columns
    results = []

    for testcase_name in ground_truth_map.keys():
        testcase_results = process_testcase(base_path, testcase_name, ground_truth_file_path, num_runs)
        results.append([testcase_name] + testcase_results[0] + testcase_results[1])

    # Print results in table format
    print("\t".join(header))
    for result in results:
        # result_line = [f"{item:.2f}" if isinstance(item, float) else '-' if item is None else str(item) for item in result]
        result_line = [str(item) if item else '-' for item in result]
        print("\t".join(result_line))

def main():
    parser = argparse.ArgumentParser(description='Check ranking results for single or multiple test case runs.')
    subparsers = parser.add_subparsers(dest='command')

    # Subparser for single test case run
    parser_check = subparsers.add_parser('check', help='Check a single test case run')
    parser_check.add_argument('ground_truth_file_path', type=str, help='Path to the ground truth file.')
    parser_check.add_argument('log_file_path', type=str, help='Path to the log file.')
    parser_check.add_argument('testcase_id', type=str, help='ID of the testcase to use for ground truth.')

    # Subparser for multiple test case runs
    parser_check_all = subparsers.add_parser('check_all', help='Check multiple test case runs')
    parser_check_all.add_argument('ground_truth_file_path', type=str, help='Path to the ground truth file.')
    parser_check_all.add_argument('base_path', type=str, help='Base path where the test case directories are located.')
    parser_check_all.add_argument('num_runs', type=int, default=1, help='Number of runs for each test case.')

    args = parser.parse_args()

    if args.command == 'check':
        result = check_single(args.log_file_path, args.ground_truth_file_path, args.testcase_id)
        print(json.dumps(result, indent=4, default=str))
    elif args.command == 'check_all':
        check_all(args.base_path, args.ground_truth_file_path, args.num_runs)

if __name__ == "__main__":
    main()
