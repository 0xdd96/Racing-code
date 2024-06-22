#!/usr/bin/python3
#pylint: disable = missing-module-docstring, no-member, redefined-outer-name

from functools import partial
from typing import List
import os
import sys
import time
import random
import logging
import zipfile
import subprocess
import multiprocessing

###############################################################################
PIN_EXE = os.environ['PIN_ROOT'] + "/pin"
PIN_TOOL = "../InstTracer/obj-intel64/inst_tracer.so"

ASAN_OPTIONS = "ASAN_OPTIONS=detect_leaks=0"
PIN_TIMEOUT = 5 * 60
###############################################################################

def trace_input(src_path: str, target_exe: str, save_path: str) -> None:
    if "@@" in target_exe:
        target_exe = target_exe.replace("@@", src_path)
    else:
        target_exe += f" < {src_path}"

    cmd = f"{ASAN_OPTIONS} {PIN_EXE} -t {PIN_TOOL} -o {save_path} -- {target_exe}"
    print(f"CMD: {cmd}")
    try:
        subprocess.run(cmd, shell=True, check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=PIN_TIMEOUT)
    except subprocess.TimeoutExpired:
        print(f"Timeout for {src_file}")
    except subprocess.CalledProcessError as err:
        pass


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: ./tracing.py <PATH TO BINARY> <INPUT FILE> <OUTPUT FILE>")
        sys.exit(1)

    target_exe=sys.argv[1]
    src_path=sys.argv[2]
    save_path=sys.argv[3]

    """Checks to avoid creation of bad traces."""
    # Check if ASLR is still enabled and abort
    with open("/proc/sys/kernel/randomize_va_space", 'r') as f:
        if not "0" in f.read().strip():
            print("[!] Disable ASLR: echo 0 | sudo tee /proc/sys/kernel/randomize_va_space")
            sys.exit(1)
    
    # Check if output file still exits and abort
    if os.path.exists(save_path):
        os.remove(save_path)
    #    print(f"[!] Output file {save_path} already exists. Backup its contents and delete it to proceed.")
        #sys.exit(1)
    
    trace_input(src_path, target_exe, save_path)



