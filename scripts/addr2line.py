#!/usr/bin/python3
import json,pdb,sys,os
import pwd
import subprocess
import multiprocessing

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: ./addr2line.py <PATH TO BINARY> <PATH TO TRACE> <OUTPUT FILE>")
        sys.exit(1)
    
    binary=sys.argv[1]
    trace_file=sys.argv[2]    
    out_file=sys.argv[3]

    with open(trace_file,'r') as f:
        ins_addrs = f.read().split('\n')
    
    source_lines = []
    i = 0
    for addr in ins_addrs:
        if addr=='': 
            continue
        
        print(f"Processing inst #{i}")
        i += 1

        cmd = f"addr2line -e {binary} -a {addr} -f -C -s -i -p"
        try:
            p = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, stdin=subprocess.PIPE, shell=True)
            (output, err) = p.communicate()
        except:
            print(cmd,  " error!")
            continue

        output = output.decode('utf-8')
        
        if ' at ' not in output:
            continue
        source_line = output.split(' at ')[1].split(' ')[0].split('\n')[0]
        if source_line not in source_lines:
            source_lines.append(source_line)


    with open(out_file, 'w') as f:
        for line in source_lines:
            f.write(line+'\n')
