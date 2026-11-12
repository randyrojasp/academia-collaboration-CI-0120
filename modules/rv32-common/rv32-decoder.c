#include "rv32-decoder.h"

static const uint32_t REGISTER_INDEX_MASK = 0x1Fu;


static uint32_t
immediate_i(uint32_t word)
{
    uint32_t immediate = word >> 20;
    // Extend the sign 32 bits
    if (immediate & 0x800u)
         immediate |= 0xFFFFF000u;
    return immediate;
}

static uint32_t 
immediate_b(uint32_t instruction) {
    uint32_t immediate = ((instruction >> 31) & 0x1)  << 12 |
                   ((instruction >> 7)  & 0x1)  << 11 |
                   ((instruction >> 25) & 0x3F) << 5  |
                   ((instruction >> 8)  & 0xF)  << 1;

    // Extend the sign 13 bits
    if (immediate & 0x1000u)
        immediate |= 0xFFFFE000u;
    
    return immediate;
}

static uint32_t
immediate_j(uint32_t word)
{
    uint32_t immediate = (((word >> 31) & 0x001u) << 20)
        | (((word >> 12) & 0x0FFu) << 12)
        | (((word >> 20) & 0x001u) << 11)
        | (((word >> 21) & 0x3FFu) << 1);
    // Extend the sign 21 bits
    if (immediate & 0x100000u) {
        immediate |= 0xFFE00000u;
    }

    return immediate;
}

static int
decode_r(uint32_t word, rv32_decoded_t *decoded)
{
    uint32_t function_3b = (word >> 12) & 0x07u;
    uint32_t function_7b = (word >> 25) & 0x7Fu;

    uint32_t key = (function_7b << 3) | function_3b;

    switch (key) {
    case (0x00u << 3) | 0x0u:
        decoded->operation = RV32_OP_ADD;
        break;
    case (0x20u << 3) | 0x0u:
        decoded->operation = RV32_OP_SUB;
        break;
    case (0x00u << 3) | 0x1u:
        decoded->operation = RV32_OP_SLL;
        break;
    case (0x00u << 3) | 0x2u:
        decoded->operation = RV32_OP_SLT;
        break;
    case (0x00u << 3) | 0x3u:
        decoded->operation = RV32_OP_SLTU;
        break;
    case (0x00u << 3) | 0x4u:
        decoded->operation = RV32_OP_XOR;
        break;
    case (0x00u << 3) | 0x5u:
        decoded->operation = RV32_OP_SRL;
        break;
    case (0x20u << 3) | 0x5u:
        decoded->operation = RV32_OP_SRA;
        break;
    case (0x00u << 3) | 0x6u:
        decoded->operation = RV32_OP_OR;
        break;
    case (0x00u << 3) | 0x7u:
        decoded->operation = RV32_OP_AND;
        break;
    default:
        return 0;
    }

    decoded->rd  = (word >> 7)  & REGISTER_INDEX_MASK;
    decoded->rs1 = (word >> 15) & REGISTER_INDEX_MASK;
    decoded->rs2 = (word >> 20) & REGISTER_INDEX_MASK;

    decoded->uses_rs1 = 1;
    decoded->uses_rs2 = 1;
    decoded->writes_rd = 1;

    return 1;
}

static int
decode_i(uint32_t word, rv32_decoded_t *decoded)
{
    uint32_t function_3b = (word >> 12) & 0x07u;
    uint32_t function_7b = (word >> 25) & 0x7Fu;

    switch (function_3b) {
    case 0x0u:
        decoded->operation = RV32_OP_ADDI;
        break;

    case 0x1u:
        if (function_7b != 0x00u)
            return 0;

        decoded->operation = RV32_OP_SLLI;
        break;

    case 0x2u:
        decoded->operation = RV32_OP_SLTI;
        break;

    case 0x3u:
        decoded->operation = RV32_OP_SLTIU;
        break;

    case 0x4u:
        decoded->operation = RV32_OP_XORI;
        break;

    case 0x5u:
        if (function_7b == 0x00u)
            decoded->operation = RV32_OP_SRLI;
        else if (function_7b == 0x20u)
            decoded->operation = RV32_OP_SRAI;
        else
            return 0;
        break;

    case 0x6u:
        decoded->operation = RV32_OP_ORI;
        break;

    case 0x7u:
        decoded->operation = RV32_OP_ANDI;
        break;

    default:
        return 0;
    }

    decoded->rd  = (word >> 7)  & REGISTER_INDEX_MASK;
    decoded->rs1 = (word >> 15) & REGISTER_INDEX_MASK;

    decoded->uses_rs1 = 1;
    decoded->writes_rd = 1;

    if (function_3b == 0x1u || function_3b == 0x5u)
        decoded->immediate = (word >> 20) & 0x1Fu;
    else
        decoded->immediate = immediate_i(word);

    return 1;
}

static int
decode_branch(uint32_t word, rv32_decoded_t *decoded)
{
    uint32_t function_3b = (word >> 12) & 0x07u;

    switch (function_3b) {
    case 0x0u:
        decoded->operation = RV32_OP_BEQ;
        break;
    case 0x1u:
        decoded->operation = RV32_OP_BNE;
        break;
    case 0x4u:
        decoded->operation = RV32_OP_BLT;
        break;
    case 0x5u:
        decoded->operation = RV32_OP_BGE;
        break;
    case 0x6u:
        decoded->operation = RV32_OP_BLTU;
        break;
    case 0x7u:
        decoded->operation = RV32_OP_BGEU;
        break;
    default:
        return 0;
    }

    decoded->rs1 = (word >> 15) & REGISTER_INDEX_MASK;
    decoded->rs2 = (word >> 20) & REGISTER_INDEX_MASK;
    decoded->immediate = immediate_b(word);

    decoded->uses_rs1 = 1;
    decoded->uses_rs2 = 1;

    return 1;
}
int
rv32_decode(uint32_t word, rv32_decoded_t *out)
{
    rv32_decoded_t decoded = {0};

    if (out == 0)
        return 0;
    *out = decoded;

    uint32_t opcode = word & 0x7Fu;

    switch (opcode) {
    case 0x33u:
        if (!decode_r(word, &decoded))
            return 0;
        break;

    case 0x13u:
        if (!decode_i(word, &decoded))
            return 0;
        break;

    case 0x63u:
        if (!decode_branch(word, &decoded))
            return 0;
        break;

    case 0x37u:
        decoded.operation = RV32_OP_LUI;
        decoded.rd = (word >> 7) & REGISTER_INDEX_MASK;
        decoded.immediate = word & 0xFFFFF000u;
        decoded.writes_rd = 1;
        break;

    case 0x17u:
        decoded.operation = RV32_OP_AUIPC;
        decoded.rd = (word >> 7) & REGISTER_INDEX_MASK;
        decoded.immediate = word & 0xFFFFF000u;
        decoded.writes_rd = 1;
        break;

    case 0x6Fu:
        decoded.operation = RV32_OP_JAL;
        decoded.rd = (word >> 7) & REGISTER_INDEX_MASK;
        decoded.immediate = immediate_j(word);
        decoded.writes_rd = 1;
        break;

    case 0x67u:
        if (((word >> 12) & 0x07u) != 0x0u)
            return 0;

        decoded.operation = RV32_OP_JALR;
        decoded.rd  = (word >> 7)  & REGISTER_INDEX_MASK;
        decoded.rs1 = (word >> 15) & REGISTER_INDEX_MASK;
        decoded.immediate = immediate_i(word);
        decoded.uses_rs1 = 1;
        decoded.writes_rd = 1;
        break;

    case 0x73u:
        if (word != 0x00100073u)
            return 0;

        decoded.operation = RV32_OP_EBREAK;
        break;

    default:
        return 0;
    }

    *out = decoded;
    return 1;
}
