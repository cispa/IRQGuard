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
/*
 *  This file contains an ISA-portable PIN tool for tracing memory accesses.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>
#include <ios>

#include "pin.H"

struct __attribute__((__packed__))
{
  uint64_t start;
  uint64_t total_accesses;
  uint64_t instructions;
  uint64_t mem_accesses;
} aux_data;


std::ofstream trace, aux;

uint64_t instruction_count = 0;
uint64_t instruction_rec_count = 0, old_instruction_rec_count = 0;
uint64_t mem_count = 0, old_mem_count = 0;

PIN_MUTEX lock;
int main_thread = 0; // store the thread that we want to record
bool recording = false;

// Print a memory read record
VOID RecordMemRead(VOID *ip, VOID *addr)
{
  PIN_MutexLock(&lock);
  if (!recording || IARG_THREAD_ID != main_thread) {
    PIN_MutexUnlock(&lock);
    return;
  }

  // dumping the stringstream at once makes it thread-safe to write to the file
  std::ostringstream oss;
  oss << "0x" << std::hex << (size_t)addr << std::endl;
  trace << oss.str();
  PIN_MutexUnlock(&lock);
}

// Print a memory write record
VOID RecordMemWrite(VOID *ip, VOID *addr)
{
  PIN_MutexLock(&lock);
  if (!recording || IARG_THREAD_ID != main_thread) {
    PIN_MutexUnlock(&lock);
    return;
  }


  // dumping the stringstream at once makes it thread-safe to write to the file
  std::ostringstream oss;
  oss << "0x" << std::hex << (size_t)addr << std::endl;
  trace << oss.str();
  PIN_MutexUnlock(&lock);
}

VOID printip(VOID *ip)
{
  PIN_MutexLock(&lock);
  instruction_count++;

  if (!recording || IARG_THREAD_ID != main_thread) {
    PIN_MutexUnlock(&lock);
    return;
  }

  // dumping the stringstream at once makes it thread-safe to write to the file
  std::ostringstream oss;
  oss << "0x" << std::hex << (size_t)ip << std::endl;
  trace << oss.str();
  PIN_MutexUnlock(&lock);
}

VOID verr(VOID *ip)
{

  main_thread = IARG_THREAD_ID;
  std::cout << "MemTracer: verr @ 0x" << std::hex << size_t(ip)
            << "(tid: " << main_thread << ")" << std::endl;
  recording = !recording;
  if (!recording)
  {
    std::ostringstream out;
    aux_data.start = old_instruction_rec_count + old_mem_count;
    aux_data.total_accesses = instruction_rec_count + mem_count - old_instruction_rec_count - old_mem_count;
    aux_data.mem_accesses = mem_count - old_mem_count;
    aux_data.instructions = instruction_rec_count - old_instruction_rec_count;
//     aux.write((char *)&aux_data, sizeof(aux_data));

    old_mem_count = mem_count;
    old_instruction_rec_count = instruction_rec_count;
  }
}

// Is called for every instruction and instruments reads and writes
VOID Instruction(INS ins, VOID *v)
{

  //check for verr instructions, start/stop recording when encountered
  if (INS_Disassemble(ins).find("verr") != std::string::npos)
  {
    //cout << INS_Disassemble(ins) << endl;
    INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)verr, IARG_INST_PTR, IARG_END);
  }

  //record instructions
  INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)printip, IARG_INST_PTR, IARG_END);

  // Instruments memory accesses using a predicated call, i.e.
  // the instrumentation is called iff the instruction will actually be executed.
  //
  // On the IA-32 and Intel(R) 64 architectures conditional moves and REP
  // prefixed instructions appear as predicated instructions in Pin.
  UINT32 memOperands = INS_MemoryOperandCount(ins);

  // Iterate over each memory operand of the instruction.
  for (UINT32 memOp = 0; memOp < memOperands; memOp++)
  {
    if (INS_MemoryOperandIsRead(ins, memOp))
    {
      INS_InsertPredicatedCall(
          ins, IPOINT_BEFORE, (AFUNPTR)RecordMemRead,
          IARG_INST_PTR,
          IARG_MEMORYOP_EA, memOp,
          IARG_END);
    }
    // Note that in some architectures a single memory operand can be
    // both read and written (for instance incl (%eax) on IA-32)
    // In that case we instrument it once for read and once for write.
    if (INS_MemoryOperandIsWritten(ins, memOp))
    {
      INS_InsertPredicatedCall(
          ins, IPOINT_BEFORE, (AFUNPTR)RecordMemWrite,
          IARG_INST_PTR,
          IARG_MEMORYOP_EA, memOp,
          IARG_END);
    }
  }
}


VOID Fini(INT32 code, VOID *v)
{
  trace.close();
  aux.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
  PIN_ERROR("This Pintool prints a trace of memory addresses\n" + KNOB_BASE::StringKnobSummary() + "\n");
  return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */

int main(int argc, char *argv[])
{
  if (PIN_Init(argc, argv))
    return Usage();

  PIN_MutexInit(&lock);

  trace.open("prefetch_config.iguard", std::ios_base::out | std::ios_base::trunc);

  PIN_InitSymbols();

  INS_AddInstrumentFunction(Instruction, 0);
  PIN_AddFiniFunction(Fini, 0);

  // Never returns
  PIN_StartProgram();

  return 0;
}
