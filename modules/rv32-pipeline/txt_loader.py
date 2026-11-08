# © 2026 Intel Corporation
#
# This software and the related documents are Intel copyrighted materials, and
# your use of them is governed by the express license under which they were
# provided to you ("License"). Unless the License provides otherwise, you may
# not use, modify, copy, publish, distribute, disclose or transmit this software
# or the related documents without Intel's prior written permission.
#
# This software and the related documents are provided as is, with no express or
# implied warranties, other than those that are expressly stated in the License.

import simics


INSTRUCTIONS = {
    "ebreak": 0x00100073,
}

R_TYPE = {
    "add": (0b000, 0b0000000),
    "sub": (0b000, 0b0100000),
    "sll": (0b001, 0b0000000),
    "slt": (0b010, 0b0000000),
    "sltu": (0b011, 0b0000000),
    "xor": (0b100, 0b0000000),
    "srl": (0b101, 0b0000000),
    "sra": (0b101, 0b0100000),
    "or": (0b110, 0b0000000),
    "and": (0b111, 0b0000000),
}

I_TYPE = {
    "addi": 0b000,
    "slti": 0b010,
    "sltiu": 0b011,
    "xori": 0b100,
    "ori": 0b110,
    "andi": 0b111,
    "slli": 0b001,
    "srli": 0b101,
    "srai": 0b101,
}

U_TYPE = {
    "lui": 0b0110111,
    "auipc": 0b0010111,
}

def register(text):
    text = text.strip().lower()
    if not text.startswith("x") or not text[1:].isdigit():
        raise ValueError(f"invalid register: {text}")
    number = int(text[1:])
    if not 0 <= number <= 31:
        raise ValueError(f"register out of range: {text}")
    return number

def immediate(text):
    text = text.strip()
    try:
        if text.lower().lstrip("+-").startswith("0x"):
            return int(text, 16)
        return int(text, 10)
    except ValueError:
        raise ValueError(f"invalid immediate: {text}") from None

def immediate_12(text):
    number = immediate(text)
    if not -2048 <= number <= 2047:
        raise ValueError(f"immediate out of range [-2048, 2047]: {number}")
    return number & 0b111111111111

def immediate_20(text):
    number = immediate(text)

    if not -(1 << 19) <= number <= (1 << 20) - 1:
        raise ValueError(
            f"immediate out of range: {number}"
        )

    return number & 0xFFFFF

def encode_r(rd, rs1, rs2, funct3, funct7):
    return ((funct7 << 25) | (rs2 << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | 0b0110011)

def encode_i(rd, rs1, imm, funct3, opcode=0b0010011):
    return ((imm << 20) | (rs1 << 15) | (funct3 << 12) | (rd << 7) | opcode)

def encode_u(rd, imm, opcode):
    return ((imm << 12) | (rd << 7) | opcode)

def assemble(line):
    line = line.split("#", 1)[0].strip().lower()
    if not line:
        return None

    parts = line.split(None, 1)
    name = parts[0]
    args = [item.strip() for item in parts[1].split(",")] if len(parts) == 2 else []
    if name == "ebreak":
        if args:
            raise ValueError("ebreak: expected 0 operands")
        return 0x00100073

    if name in U_TYPE:
        if len(args) != 2:
            raise ValueError(f"{name}: expected 2 operands")

        rd = register(args[0])
        imm = immediate_20(args[1])

        return encode_u(rd, imm, U_TYPE[name])

    if name not in R_TYPE and name not in I_TYPE:
        raise ValueError(f"unsupported instruction: {name}")

    if len(args) != 3:
        raise ValueError(f"{name}: expected 3 operands")

    rd = register(args[0])
    rs1 = register(args[1])

    if name in R_TYPE:
        rs2 = register(args[2])
        funct3, funct7 = R_TYPE[name]
        return encode_r(rd, rs1, rs2, funct3, funct7)

    if name in ("slli", "srli", "srai"):
        shamt = immediate(args[2])

        if not 0 <= shamt <= 31:
            raise ValueError(f"shift amount out of range [0, 31]: {shamt}")

        imm = shamt

        if name == "srai":
            imm |= 0b0100000 << 5

        return encode_i(rd, rs1, imm, I_TYPE[name])

    imm = immediate_12(args[2])
    return encode_i(rd, rs1, imm, I_TYPE[name])

def mem_write8(core, addr, value):
    """Write the byte value (0..255) to address addr of the core's memory."""
    simics.SIM_write_phys_memory(core, addr, value, 1)


def load_program(core, path, address):
    """Read the assembly program in the text file path and write the machine
    code of its instructions into memory, the first one at address."""
    with open(path, "r", encoding="utf-8") as file:
        for line_number, line in enumerate(file, 1):
            try:
                code = assemble(line)
            except ValueError as error:
                raise ValueError(f"{path}:{line_number}: {error}") from error
            if code is None:
                continue
            for byte in code.to_bytes(4, "little"):
                mem_write8(core, address, byte)
                address += 1
