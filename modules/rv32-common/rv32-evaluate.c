#include "rv32-evaluate.h"

static const uint32_t INSTRUCTION_SIZE_BYTES = 4u;

rv32_result_t
rv32_evaluate(
    const rv32_decoded_t *instruction,
    uint32_t instruction_pc,
    uint32_t operand_a,
    uint32_t operand_b
)
{
    rv32_result_t result = {
        .value = 0,
        .next_pc = instruction_pc + INSTRUCTION_SIZE_BYTES,
        .redirect = 0,
        .status = RV32_EXEC_OK
    };

    if (!instruction) {
        result.status = RV32_EXEC_ILLEGAL;
        return result;
    }

    uint32_t immediate = instruction->immediate;

    // If the instruction uses a second register, it takes operand_b.
    uint32_t rhs;
    if (instruction->uses_rs2 != 0) {
        rhs = operand_b;
    } else {
        rhs = immediate;
    }

    int take_branch = 0;

    switch (instruction->operation) {
    case RV32_OP_ADD:
    case RV32_OP_ADDI:
        result.value = operand_a + rhs;
        break;

    case RV32_OP_SUB:
        result.value = operand_a - rhs;
        break;

    case RV32_OP_SLL:
    case RV32_OP_SLLI:
        result.value = operand_a << (rhs & 0x1Fu);
        break;

    case RV32_OP_SLT:
    case RV32_OP_SLTI:
        result.value = (int32_t)operand_a < (int32_t)rhs;
        break;

    case RV32_OP_SLTU:
    case RV32_OP_SLTIU:
        result.value = operand_a < rhs;
        break;

    case RV32_OP_XOR:
    case RV32_OP_XORI:
        result.value = operand_a ^ rhs;
        break;

    case RV32_OP_SRL:
    case RV32_OP_SRLI:
        result.value = operand_a >> (rhs & 0x1Fu);
        break;

    case RV32_OP_SRA:
    case RV32_OP_SRAI:
        result.value = (uint32_t)((int32_t)operand_a >> (rhs & 0x1Fu));
        break;

    case RV32_OP_OR:
    case RV32_OP_ORI:
        result.value = operand_a | rhs;
        break;

    case RV32_OP_AND:
    case RV32_OP_ANDI:
        result.value = operand_a & rhs;
        break;

    case RV32_OP_LUI:
        result.value = immediate;
        break;

    case RV32_OP_AUIPC:
        result.value = instruction_pc + immediate;
        break;

    /* Las ramas comparan los dos registros. */
    case RV32_OP_BEQ:
        take_branch = operand_a == operand_b;
        break;

    case RV32_OP_BNE:
        take_branch = operand_a != operand_b;
        break;

    case RV32_OP_BLT:
        take_branch = (int32_t)operand_a < (int32_t)operand_b;
        break;

    case RV32_OP_BGE:
        take_branch = (int32_t)operand_a >= (int32_t)operand_b;
        break;

    case RV32_OP_BLTU:
        take_branch = operand_a < operand_b;
        break;

    case RV32_OP_BGEU:
        take_branch = operand_a >= operand_b;
        break;

    case RV32_OP_JAL:
        result.value = result.next_pc;
        result.next_pc = instruction_pc + immediate;
        result.redirect = 1;
        break;

    case RV32_OP_JALR:
        result.value = result.next_pc;
        result.next_pc = (operand_a + immediate) & ~1u;
        result.redirect = 1;
        break;

    case RV32_OP_EBREAK:
        result.status = RV32_EXEC_EBREAK;
        break;

    default:
        result.status = RV32_EXEC_ILLEGAL;
        break;
    }

    if (take_branch) {
        result.next_pc = instruction_pc + immediate;
        result.redirect = 1;
    }

    if (result.redirect && (result.next_pc & 3u) != 0u)
        result.status = RV32_EXEC_MISALIGNED;

    return result;
}
