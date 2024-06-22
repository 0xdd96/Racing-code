/*BEGIN_LEGAL 
Intel Open Source License 

Copyright (c) 2002-2018 Intel Corporation. All rights reserved.
 
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

Redistributions of source code must retain the above copyright notice,
this list of conditions and the following disclaimer.  Redistributions
in binary form must reproduce the above copyright notice, this list of
conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.  Neither the name of
the Intel Corporation nor the names of its contributors may be used to
endorse or promote products derived from this software without
specific prior written permission.
 
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL OR
ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
END_LEGAL */
#include <stdio.h>
#include <fstream>
#include <set>
#include <vector>
#include <iterator>
#include "pin.H"


static FILE * g_trace_file;
static std::set<ADDRINT> g_instruction_map;
static PIN_LOCK g_lock;
static ADDRINT g_load_offset;

/**
 *  Extract image base from main executable. 
 */
VOID parse_image(IMG img, VOID *v) {
    LOG("[+] Called parse_image on " + IMG_Name(img) + "\n");
    if (IMG_IsMainExecutable(img)) {
        g_load_offset  = IMG_LoadOffset(img);
        LOG("[*] Load offset: " + StringFromAddrint(g_load_offset) + "\n");
    }
}


VOID log_ins_addr(ADDRINT ins_addr) {
    PIN_GetLock(&g_lock, ins_addr);
    g_instruction_map.insert(ins_addr);
    PIN_ReleaseLock(&g_lock);
}


// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v) {
    // Skip instructions outside main exec
    PIN_LockClient();
    const IMG image = IMG_FindByAddress(INS_Address(ins));
    PIN_UnlockClient();
    if (IMG_Valid(image) && IMG_IsMainExecutable(image)) {
        if (INS_IsHalt(ins)) {
            LOG("[W] Skipping instruction: " + StringFromAddrint(INS_Address(ins)) + " : "
                + INS_Disassemble(ins) + "\n");
            return;
        }
        // Insert a call to log_ins_addr before every instruction, and pass it the IP
        INS_InsertCall(ins,
            IPOINT_BEFORE, (AFUNPTR)log_ins_addr,
            IARG_ADDRINT, INS_Address(ins),
            IARG_END
        );
    }
}

/**
 *  Write data to output file upon application exit
 */
VOID Fini(INT32 code, VOID *v) {
    if (g_trace_file == nullptr) {
        PIN_ERROR("Error opening file.\n");
        return;
    }

    for (auto it = g_instruction_map.begin(); it != g_instruction_map.end(); ++it) {
        fprintf(g_trace_file, "%#lx\n", (*it)-g_load_offset);
    }

    fclose(g_trace_file);
    LOG("[=] Completed trace.\n");
}


// Allow renaming output file via -o switch
KNOB<std::string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "itrace.out", "specify output file name");


/* ===================================================================== */
/* Print Help Messages                                                   */
/* ===================================================================== */

INT32 Usage() {
    PIN_ERROR("This Pintool traces each instruction, dumping their addresses and additional state.\n"
              + KNOB_BASE::StringKnobSummary() + "\n");
    return -1;
}


INT32 Aslr() {
    PIN_ERROR("Disable ASLR before running this tool: echo 0 | sudo tee /proc/sys/kernel/randomize_va_space");
    return -1;
}


/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char * argv[]) {
    // Check if ASLR is disabled
    std::ifstream infile("/proc/sys/kernel/randomize_va_space");
    int aslr;
    if (!infile) {
        PIN_ERROR("Unable to check whether ASLR is enabled or not. Failed to open /proc/sys/kernel/randomize_va_space");
        return -1;
    }
    infile >> aslr;
    infile.close();
    if (aslr != 0) return Aslr();

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    g_trace_file = fopen(KnobOutputFile.Value().c_str(), "w");

    // get image base address
    IMG_AddInstrumentFunction(parse_image, 0);

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);
    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    LOG("[*] Pintool: " + std::string(PIN_ToolFullPath()) + "\n");
    LOG("[*] Target:  " + std::string(PIN_VmFullPath()) + "\n");

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}

