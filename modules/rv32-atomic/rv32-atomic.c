/*
  © 2026 Intel Corporation

  This software and the related documents are Intel copyrighted materials, and
  your use of them is governed by the express license under which they were
  provided to you ("License"). Unless the License provides otherwise, you may
  not use, modify, copy, publish, distribute, disclose or transmit this software
  or the related documents without Intel's prior written permission.

  This software and the related documents are provided as is, with no express or
  implied warranties, other than those that are expressly stated in the License.
*/

#include <simics/util/strbuf.h>


#include "cosim.h"
#include "rv32-core.h"
#include "rv32-helpers.h"

#include "../rv32-common/rv32-decoder.h"
#include "../rv32-common/rv32-evaluate.h"

void 
execute(rv32_t *cpu)
{
    uint8 bytes[4];
    uint32_t instruction_pc = cpu->pc;
    if (!read_simics_memory(cpu, instruction_pc, bytes, sizeof bytes)){
        rv32_halt(cpu, "Error to read the instruccion");
        return;
    }

    uint32 word = (uint32)bytes[0]
                | ((uint32)bytes[1] << 8)
                | ((uint32)bytes[2] << 16)
                | ((uint32)bytes[3] << 24);

    rv32_decoded_t instruction;

    if (!rv32_decode(word, &instruction)){
        illegal_instruction(cpu, instruction_pc, word);
        return;
    }

    uint32_t operand_a = 0;
    uint32_t operand_b = 0;
    if (instruction.uses_rs1) {
        operand_a = reg_read_x(cpu, instruction.rs1);
    }

    if (instruction.uses_rs2) {
        operand_b = reg_read_x(cpu, instruction.rs2);
    }

    rv32_result_t result = rv32_evaluate(
        &instruction,
        instruction_pc,
        operand_a,
        operand_b
    );


    switch (result.status) {
    case RV32_EXEC_OK:
        break;

    case RV32_EXEC_EBREAK:
        ebreak(cpu);
        return;

    case RV32_EXEC_MISALIGNED:
        rv32_halt(cpu, "Misaligned instruction target");
        return;

    case RV32_EXEC_ILLEGAL:
    default:
        illegal_instruction(cpu, instruction_pc, word);
        return;
    }


    if (instruction.writes_rd) {
        reg_write_x(cpu, instruction.rd, result.value);
    }

    cpu->pc = result.next_pc;
    increment_step(cpu);
    increment_cycle(cpu);
}

/* Text for Simics' disassemble CLI command (e.g. "disassemble 0x0 10")
   for the instruction word instr. Return a string allocated with Simics'
   allocators, e.g. MM_STRDUP() or a strbuf as below; Simics frees it. */
char *
rv32_disassemble(uint32 instr)
{
        strbuf_t b = SB_INIT;
        sb_addfmt(&b, ".word 0x%08x", instr);
        return sb_detach(&b);
}
