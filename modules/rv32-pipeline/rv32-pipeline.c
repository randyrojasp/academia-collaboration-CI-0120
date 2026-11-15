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

static int writeback(rv32_t *cpu, rv32_pipeline_slot_t previous_wb);
static void execute_instruction(rv32_t *cpu, rv32_pipeline_slot_t *current_ex,
                                int *flush);
static void pipeline_fetch_instruction(rv32_t *cpu, rv32_pipeline_slot_t *next_if,
                                       rv32_pipeline_slot_t *next_ex, int flush);

void
execute(rv32_t *cpu)
{
        // The processor cycle is incremented to the next cycle: 0 + 1 = cycle 1
        rv32_pipeline_slot_t previous_wb = cpu->wb_stage;
        rv32_pipeline_slot_t current_ex = cpu->ex_stage;

        rv32_pipeline_slot_t next_if = {0};
        rv32_pipeline_slot_t next_ex = cpu->if_stage;

        int flush = 0;

        // The processor cycle is incremented to the next cycle: 0 + 1 = cycle 1
        increment_cycle(cpu);

        if (!writeback(cpu, previous_wb))
                return;
        execute_instruction(cpu, &current_ex, &flush);
        pipeline_fetch_instruction(cpu, &next_if, &next_ex, flush);

        cpu->if_stage = next_if;
        cpu->ex_stage = next_ex;
        cpu->wb_stage = current_ex;
}

static int
writeback(rv32_t *cpu, rv32_pipeline_slot_t previous_wb)
{
        // Only the oldest instruction can change architectural state or halt
        // We check the status of the previous writeback to see if we can execute the current statement
        if (previous_wb.valid != 0) {
                // Control de hazards
                if (previous_wb.fetch_error != 0 || previous_wb.result.status != RV32_EXEC_OK) {
                        // Here we update the CPU's PC in the current cycle with the previous cycle,
                        // because the instruction is pending processing
                        cpu->pc = previous_wb.pc;
                        cpu->if_stage.valid = 0;
                        cpu->ex_stage.valid = cpu->wb_stage.valid = 0;
                        cpu->fetch_stopped = 0;
                        if (previous_wb.fetch_error != 0)
                                rv32_halt(cpu, "Error reading instruction");
                        else if (previous_wb.result.status == RV32_EXEC_EBREAK)
                                ebreak(cpu);
                        else if (previous_wb.result.status == RV32_EXEC_MISALIGNED)
                                rv32_halt(cpu, "Misaligned instruction target");
                        else
                                illegal_instruction(cpu, previous_wb.pc, previous_wb.word);
                        return 0;
                }

                if (previous_wb.instruction.writes_rd)
                        reg_write_x(cpu, previous_wb.instruction.rd, previous_wb.result.value);
                increment_step(cpu);
        }
        return 1;
}

static void
execute_instruction(rv32_t *cpu, rv32_pipeline_slot_t *current_ex, int *flush)
{
        // Execute resolves control flow, younger instructions are discarded
        if (current_ex->valid != 0) {
                if (current_ex->fetch_error) {
                        *flush = 1;
                        cpu->fetch_stopped = 1;
                } else {
                        const rv32_decoded_t *insn = &current_ex->instruction;

                        if (current_ex->result.status != RV32_EXEC_ILLEGAL) {
                                uint32 a = insn->uses_rs1 ? reg_read_x(cpu, insn->rs1) : 0;
                                uint32 b = insn->uses_rs2 ? reg_read_x(cpu, insn->rs2) : 0;
                                current_ex->result = rv32_evaluate(insn, current_ex->pc, a, b);
                        }
                        if (current_ex->result.status != RV32_EXEC_OK) {
                                *flush = 1;
                                cpu->fetch_stopped = 1;
                        } else if (current_ex->result.redirect) {
                                *flush = 1;
                                cpu->pc = current_ex->result.next_pc;
                        }
                }
        }
}

static void
pipeline_fetch_instruction(rv32_t *cpu, rv32_pipeline_slot_t *next_if,
                           rv32_pipeline_slot_t *next_ex, int flush)
{
        if (flush != 0) {
                next_ex->valid = 0;
        } else {
                // ID
                // Here, it decodes the instruction and stores it in next_ex
                if (next_ex->valid != 0 && !next_ex->fetch_error
                    && rv32_decode(next_ex->word, &next_ex->instruction) != 1)
                        next_ex->result.status = RV32_EXEC_ILLEGAL;

                // IF
                if (!cpu->fetch_stopped) {
                        uint8 bytes[4];
                        next_if->valid = 1;
                        next_if->pc = cpu->pc;
                        next_if->fetch_error = !read_simics_memory(cpu, cpu->pc,
                                                                  bytes, sizeof bytes);
                        if (!next_if->fetch_error)
                                next_if->word = (uint32)bytes[0] | ((uint32)bytes[1] << 8)
                                              | ((uint32)bytes[2] << 16) | ((uint32)bytes[3] << 24);
                        // We increment the PC and find ourselves at the next instruction
                        cpu->pc += 4;
                }
        }
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
