#ifndef RV32_DECODE_H
#define RV32_DECODE_H
#include <stdint.h>

typedef enum {
    RV32_OP_INVALID = 0,
    RV32_OP_ADDI,
    RV32_OP_BNE,
    RV32_OP_JAL,
    RV32_OP_JALR,
    RV32_OP_EBREAK,
    RV32_OP_ADD,
    RV32_OP_SUB,
    RV32_OP_SLL,
    RV32_OP_SLT,
    RV32_OP_SLTU,
    RV32_OP_XOR,
    RV32_OP_SRL,
    RV32_OP_SRA,
    RV32_OP_OR,
    RV32_OP_AND,
    RV32_OP_SLLI,
    RV32_OP_SLTI,
    RV32_OP_SLTIU,
    RV32_OP_XORI,
    RV32_OP_SRLI,
    RV32_OP_SRAI,
    RV32_OP_ORI,
    RV32_OP_ANDI,
    RV32_OP_BEQ,
    RV32_OP_BLT,
    RV32_OP_BGE,
    RV32_OP_BLTU,
    RV32_OP_BGEU,
    RV32_OP_LUI,
    RV32_OP_AUIPC
} rv32_operation_t;

typedef struct {
    rv32_operation_t operation;
    unsigned rd;
    unsigned rs1;
    unsigned rs2;
    uint32_t immediate;
    int uses_rs1;
    int uses_rs2;
    int writes_rd;
} rv32_decoded_t;

// Decode an instruction into out; return 1 on success or 0 on failure.
int rv32_decode(uint32_t word, rv32_decoded_t *out);
#endif
