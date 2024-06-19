#!/bin/bash

TEST_ROOT=$PWD

# ANSI color codes
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# Function to log messages with timestamp and severity
log_message() {
    local severity=$1
    local message=$2
    local color=$3
    local timestamp=$(date "+%Y-%m-%d %H:%M:%S")
    echo -e "${timestamp} [${severity}] ${color}${message}${NC}"
}

# Default number of parallel jobs
num_jobs=1

# Parse -j flag and command
while getopts "j:" opt; do
    case ${opt} in
        j )
            num_jobs=$OPTARG
            ;;
        \? )
            echo "Usage: cmd [-j number_of_parallel_jobs] <build|run|clean-run>"
            exit 1
            ;;
    esac
done
shift $((OPTIND -1))

# Check for command argument
if [ "$#" -ne 1 ]; then
    echo "Usage: $0 [-j number_of_parallel_jobs] <build|run|clean-run>"
    exit 1
fi

command=$1

# Initialize counters for success and failure
success_count=0
failure_count=0

# Define status files based on the command
if [ "$command" == "build" ]; then
    completed_cases_file="$TEST_ROOT/completed_cases.build.txt"
    overall_status_file="$TEST_ROOT/overall_status.build.log"
elif [ "$command" == "run" ]; then
    completed_cases_file="$TEST_ROOT/completed_cases.run.txt"
    overall_status_file="$TEST_ROOT/overall_status.run.log"
elif [ "$command" == "clean-run" ]; then
    rm -f $TEST_ROOT/completed_cases.run.txt
    rm -f $TEST_ROOT/overall_status.run.log
    # Clean up run directories
    for testcase in $(ls -d */); do
        cd $testcase
        rm -rf afl-workdir-batch* 04_racing.log run_temp
        log_message "INFO" "Cleaned afl-workdir-batch* directories in ${testcase}" "$NC"
        cd $TEST_ROOT
    done
    exit 0
else
    echo "Invalid command: $command"
    echo "Usage: $0 [-j number_of_parallel_jobs] <build|run|clean-run>"
    exit 1
fi

# Create or clear the completed cases file
>> "$completed_cases_file"
>> "$overall_status_file"

# Function to run a script and check its exit status
run_script() {
    testcase=$1
    script=$2
    logfile="${script}.log"
    start_time=$(date +%s)

    log_message "INFO" "Running ${script} for ${testcase}..." "$NC"
    ./${script}.sh &> "${logfile}"
    exit_status=$?

    end_time=$(date +%s)
    elapsed_time=$((end_time - start_time))

    log_message "INFO" "${script} for ${testcase} took ${elapsed_time} seconds." "$NC"

    if [ $exit_status -ne 0 ]; then
        log_message "ERROR" "${script} for ${testcase} failed. Check ${logfile} for details." "$RED"
        return 1
    else
        log_message "INFO" "${script} for ${testcase} succeeded." "$GREEN"
        return 0
    fi
}

# Function to process each test case
process_test_case() {
    testcase=$1
    cd $testcase

    # Skip already completed test cases
    if grep -q "${testcase}" "$completed_cases_file"; then
        log_message "INFO" "Skipping already completed test case: ${testcase}" "$NC"
        return 0
    fi

    if [ "$command" == "build" ]; then
        scripts=("01_build_trace" "02_PocExecutionInspector" "03_build_fuzz")
    elif [ "$command" == "run" ]; then
        scripts=("04_racing")
    else
        return -1
    fi

    start_time=$(date +%s)
    local success=true

    for script in "${scripts[@]}"; do
        if ! run_script "${testcase}" "${script}"; then
            success=false
            break
        fi
    done

    end_time=$(date +%s)
    elapsed_time=$((end_time - start_time))

    if $success; then
        log_message "INFO" "${testcase} succeeded in ${elapsed_time} seconds" "$GREEN"
        echo "${testcase} succeeded in ${elapsed_time} seconds" >> "$overall_status_file"
        echo "${testcase}" >> "$completed_cases_file"
        return 0
    else
        log_message "ERROR" "${testcase} failed in ${elapsed_time} seconds" "$RED"
        echo "${testcase} failed in ${elapsed_time} seconds" >> "$overall_status_file"
        # echo "${testcase}" >> "$completed_cases_file"
        return 1
    fi
    
    cd $TEST_ROOT
}

# List all directories in the current directory and sort them numerically
test_cases=$(ls -d */ | sort -V | tr -d '/')

# Export functions and variables for GNU Parallel
export -f run_script
export -f process_test_case
export -f log_message
export num_jobs
export completed_cases_file
export overall_status_file
export TEST_ROOT
export GREEN
export RED
export NC
export command

# Run the test cases in parallel
parallel --lb -j "${num_jobs}" process_test_case ::: ${test_cases}

# Summarize results
success_count=$(grep -c "succeeded" "$overall_status_file")
failure_count=$(grep -c "failed" "$overall_status_file")

# Report overall status
log_message "INFO" "==================================" "$NC"
log_message "INFO" "Overall Status" "$NC"
log_message "INFO" "Success: ${success_count}" "$GREEN"
log_message "INFO" "Failure: ${failure_count}" "$RED"
log_message "INFO" "==================================" "$NC"

# Exit with the number of failures as the exit status
exit ${failure_count}
