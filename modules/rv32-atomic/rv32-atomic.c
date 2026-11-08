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

#include <stdbool.h>
#include <simics/util/strbuf.h>

#include "cosim.h"
#include "rv32-core.h"
#include "rv32-helpers.h"

static bool 
execute_r_type(rv32_t *cpu, uint32 instr)
{
        uint32 rd = (instr >> 7) & 0x1F;
        uint32 funct3 = (instr >> 12) & 0x07;
        uint32 rs1 = (instr >> 15) & 0x1F;
        uint32 rs2 = (instr >> 20) & 0x1F;
        uint32 funct7 = (instr >> 25) & 0x7F;

        uint32 a = reg_read_x(cpu, rs1);
        uint32 b = reg_read_x(cpu, rs2);
        uint32 result = 0;

        switch (funct3) {

        case 0x0:
                if (funct7 == 0x00)
                        result = a + b;
                else if (funct7 == 0x20)
                        result = a - b;
                else
                        return false;
                break;

        case 0x1:
                if (funct7 != 0x00)
                        return false;
                result = a << (b & 0x1F);
                break;

        case 0x2:
                if (funct7 != 0x00)
                        return false;
                result = (int32)a < (int32)b;
                break;

        case 0x3:
                if (funct7 != 0x00)
                        return false;
                result = a < b;
                break;

        case 0x4:
                if (funct7 != 0x00)
                        return false;
                result = a ^ b;
                break;

        case 0x5:
                if (funct7 == 0x00)
                        result = a >> (b & 0x1F);
                else if (funct7 == 0x20)
                        result = (uint32)((int32)a >> (b & 0x1F));
                else
                        return false;
                break;

        case 0x6:
                if (funct7 != 0x00)
                        return false;
                result = a | b;
                break;

        case 0x7:
                if (funct7 != 0x00)
                        return false;
                result = a & b;
                break;

        default:
                return false;
        }

        reg_write_x(cpu, rd, result);
        return true;
}


static bool 
execute_i_type(rv32_t *cpu, uint32 instr)
{
        uint32 rd = (instr >> 7) & 0x1F;
        uint32 funct3 = (instr >> 12) & 0x07;
        uint32 rs1 = (instr >> 15) & 0x1F;

        int32 imm = (int32)instr >> 20;

        uint32 shamt = (instr >> 20) & 0x1F;
        uint32 funct7 = (instr >> 25) & 0x7F;


        uint32 a = reg_read_x(cpu, rs1);
        uint32 result = 0;

        switch (funct3) {

        case 0x0:
                result = a + (uint32)imm;
                break;

        case 0x1:
                if (funct7 != 0x00)
                        return false;
                result = a << shamt;
                break;

        case 0x2:
                result = (int32)a < imm;
                break;

        case 0x3:
                result = a < (uint32)imm;
                break;

        case 0x4:
                result = a ^ (uint32)imm;
                break;

        case 0x5:
                if (funct7 == 0x00)
                        result = a >> shamt;
                else if (funct7 == 0x20)
                        result = (uint32)((int32)a >> shamt);
                else
                        return false;
                break;

        case 0x6:
                result = a | (uint32)imm;
                break;

        case 0x7:
                result = a & (uint32)imm;
                break;

        default:
                return false;
        }

        reg_write_x(cpu, rd, result);
        return true;
}

static bool
execute_u_type(rv32_t *cpu, uint32 instr)
{
        uint32 rd = (instr >> 7) & 0x1F;
        uint32 opcode = instr & 0x7F;

        uint32 imm = instr & 0xFFFFF000;
        uint32 result = 0;

        switch (opcode) {

        case 0x37:
                result = imm;
                break;

        case 0x17:
                result = cpu->pc + imm;
                break;

        default:
                return false;
        }

        reg_write_x(cpu, rd, result);
        return true;
}

void 
execute(rv32_t *cpu)
{
        uint8 bytes[4];
        if (!read_simics_memory(cpu, cpu->pc, bytes, sizeof bytes)){
                rv32_halt(cpu, "Error to read the instruccion");
                return;
        }
        uint32 instruccion = (uint32)bytes[0]
                     | ((uint32)bytes[1] << 8)
                     | ((uint32)bytes[2] << 16)
                     | ((uint32)bytes[3] << 24);

        if (instruccion == 0x00100073) {
                ebreak(cpu);
                return;
        }
        uint32 opcode = instruccion & 0x7F;
        bool valida = false;

        switch (opcode) {

        case 0x33:
                valida = execute_r_type(cpu, instruccion);
                break;

        case 0x13:
                valida = execute_i_type(cpu, instruccion);
                break;

        case 0x37:
                valida = execute_u_type(cpu, instruccion);
                break;

        case 0x17:
                valida = execute_u_type(cpu, instruccion);
                break;

        default:
                break;
        }

        if (!valida) {
                illegal_instruction(cpu, cpu->pc, instruccion);
                return;
        }
        cpu->pc += 4;
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
