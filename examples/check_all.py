import os
import re
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

def parse_log_file(file_path, ground_truth_loc):
    with open(file_path, 'r') as f:
        log_content = f.read()
    
    blocks = log_content.split('rank update at: ')
    if not blocks:
        return [], None

    last_block = blocks[-1]
    ranking_entries = last_block.split('\n')
    ranking_info = []
    for entry in ranking_entries:
        if ground_truth_loc in entry:
            ranking_info.append(entry)

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


def process_testcase(base_path, testcase_name, ground_truth_file_path):
    log_file_path = os.path.join(base_path, testcase_name, f"afl-workdir-batch0", "ranked_file")
    if not os.path.exists(log_file_path):
        return None

    
    ground_truth_loc = get_ground_truth(ground_truth_file_path).get(testcase_name)
    ranking_info_string = ''
    time_used = 0
    if ground_truth_loc:
        ranking_info, time_used = parse_log_file(log_file_path, ground_truth_loc)
        ranking_info_string = "\n".join(ranking_info)

    return ranking_info_string, time_used

def print_table(data):
    header = ["Testcase", "Ranking Info", "Time"]

    col_widths = [max(len(str(item[0])) for item in data), 0, max(len(str(item[2])) for item in data)]
    col_widths[0] = max(len(header[0]), col_widths[0])
    col_widths[2] = max(len(header[2]), col_widths[2])
    
    for item in data:
        lines = item[1].split('\n')
        max_line_length = max(len(line) for line in lines)
        if max_line_length > col_widths[1]:
            col_widths[1] = max_line_length
    col_widths[1] = max(len(header[1]), col_widths[1])

    header_row = " | ".join(header[i].ljust(col_widths[i]) for i in range(len(header)))
    print(header_row)
    print("-" * len(header_row))

    for row in data:
        ranking_info_lines = row[1].split('\n')
        max_lines = len(ranking_info_lines)
        
        testcase = [row[0]] + [''] * (max_lines - 1)
        time_str = [str(row[2])] + [''] * (max_lines - 1)
        
        for i in range(max_lines):
            row_str = f"{testcase[i].ljust(col_widths[0])} | {ranking_info_lines[i].ljust(col_widths[1])} | {time_str[i].ljust(col_widths[2])}"
            print(row_str)
        
        if row != data[-1]:
            print("-" * len(header_row))
    print("-" * len(header_row))

def check_all(base_path, ground_truth_file_path):
    ground_truth_map = get_ground_truth(ground_truth_file_path)

    results = []
    for testcase_name in ground_truth_map.keys():
        testcase_results = process_testcase(base_path, testcase_name, ground_truth_file_path)
        if testcase_results:
            results.append([testcase_name] + [testcase_results[0]] + [testcase_results[1]])
        else:
            results.append([testcase_name] + ["test case failed, please check 04_racing.log!"] + ["-"])
    print_table(results)
    return

def main():
    parser = argparse.ArgumentParser(description='Check ranking results for single or multiple test case runs.')
    subparsers = parser.add_subparsers(dest='command')

    parser_check_all = subparsers.add_parser('check_all', help='Check multiple test case runs')
    parser_check_all.add_argument('ground_truth_file_path', type=str, help='Path to the ground truth file.')
    parser_check_all.add_argument('base_path', type=str, help='Base path where the test case directories are located.')

    args = parser.parse_args()

    if args.command == 'check_all':
        check_all(args.base_path, args.ground_truth_file_path)

if __name__ == "__main__":
    main()
