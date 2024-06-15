#!/bin/bash

TEST_ROOT=$PWD

# Default number of parallel jobs
num_jobs=1

# Parse -j flag
while getopts "j:" opt; do
    case ${opt} in
        j )
            num_jobs=$OPTARG
            ;;
        \? )
            echo "Usage: cmd [-j number_of_parallel_jobs]"
            exit 1
            ;;
    esac
done

# Initialize counters for success and failure
success_count=0
failure_count=0

# Track completed test cases
completed_cases_file="$PWD/completed_cases.txt"
overall_status_file="$PWD/overall_status.log"

# Create or clear the completed cases file
> "$completed_cases_file"
> "$overall_status_file"

# Function to run a script and check its exit status
run_script() {
    testcase=$1
    script=$2
    logfile="${script}.log"
    start_time=$(date +%s)

    echo "Running ${script} for ${testcase}..."
    ./${script}.sh &> "${logfile}"
    exit_status=$?

    end_time=$(date +%s)
    elapsed_time=$((end_time - start_time))

    echo "${script} for ${testcase} took ${elapsed_time} seconds."

    if [ $exit_status -ne 0 ]; then
        echo "${script} for ${testcase} failed. Check ${logfile} for details."
        return 1
    else
        echo "${script} for ${testcase} succeeded."
        return 0
    fi
}

# Function to process each test case
process_test_case() {
    testcase=$1
    cd $testcase

    # Skip already completed test cases
    if grep -q "${testcase}" "$completed_cases_file"; then
        echo "Skipping already completed test case: ${testcase}"
        return 0
    fi

    start_time=$(date +%s)

    if run_script "${testcase}" "01_build_trace" && \
       run_script "${testcase}" "02_PocExecutionInspector" && \
       run_script "${testcase}" "03_build_fuzz"; then
    #    run_script "${testcase}" "03_build_fuzz" && \
    #    run_script "${testcase}" "04_racing"; then
        end_time=$(date +%s)
        elapsed_time=$((end_time - start_time))
        echo "${testcase} succeeded in ${elapsed_time} seconds"
        echo "${testcase} succeeded in ${elapsed_time} seconds" >> "$overall_status_file"
        echo "${testcase}" >> "$completed_cases_file"
        return 0
    else
        end_time=$(date +%s)
        elapsed_time=$((end_time - start_time))
        echo "${testcase} failed in ${elapsed_time} seconds"
        echo "${testcase} failed in ${elapsed_time} seconds" >> "$overall_status_file"
        echo "${testcase}" >> "$completed_cases_file"
        return 1
    fi
    
    cd $TEST_ROOT
}

# List all directories in the current directory and sort them numerically
test_cases=$(ls -d */ | sort -V | tr -d '/')

# Export functions and variables for GNU Parallel
export -f run_script
export -f process_test_case
export num_jobs
export completed_cases_file
export overall_status_file
export TEST_ROOT

# Run the test cases in parallel
parallel -k -j "${num_jobs}" process_test_case ::: ${test_cases}

# Summarize results
success_count=$(grep -c "succeeded" overall_status.log)
failure_count=$(grep -c "failed" overall_status.log)

# Report overall status
echo "=================================="
echo "Overall Status"
echo "Success: ${success_count}"
echo "Failure: ${failure_count}"
echo "=================================="

# Exit with the number of failures as the exit status
exit ${failure_count}
