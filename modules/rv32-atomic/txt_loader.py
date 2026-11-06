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

def mem_write8(core, addr, value):
    """Write the byte value (0..255) to address addr of the core's memory."""
    simics.SIM_write_phys_memory(core, addr, value, 1)

def load_program(core, path, address):
    """function to load instruccion in format little-endian"""
    with open(path, "r", encoding="utf-8") as archivo:
        for num_linea, linea in enumerate(archivo, start=1):
            # remove the comments
            instruction = linea.split("#", 1)[0].strip()

            # skip whitespace
            if not instruction:
                continue

            try:
                codigo = INSTRUCTIONS[instruction]
            except KeyError:
                raise ValueError(
                    f"{path}:{num_linea}: instrucción no soportada: {instruction!r}"
                ) from None

            # write the bytes in little-endian order
            for i, byte in enumerate(codigo.to_bytes(4, "little")):
                mem_write8(core, address + i, byte)

            address += 4