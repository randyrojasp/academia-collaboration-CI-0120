#ifndef RV32_EVALUATE_H
#define RV32_EVALUATE_H

#include "rv32-decoder.h"

typedef enum {
    RV32_EXEC_OK = 0,
    RV32_EXEC_EBREAK,
    RV32_EXEC_ILLEGAL,
    RV32_EXEC_MISALIGNED
} rv32_exec_status_t;

typedef struct {
    uint32_t value;
    uint32_t next_pc;
    int redirect;
    rv32_exec_status_t status;
} rv32_result_t;

rv32_result_t rv32_evaluate(
    const rv32_decoded_t *instruction,
    uint32_t instruction_pc,
    uint32_t operand_a,
    uint32_t operand_b
);

#endif
