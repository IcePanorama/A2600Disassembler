#include "instruction_lookup.hpp"
#include "addressing_mode.hpp"
#include "instruction.hpp"

/* clang-format off */
const std::unordered_map<uint8_t, Instruction> &
InstructionLookupTable::get_table (void)
{
  /** @see: http://www.6502.org/tutorials/6502opcodes.html */
  static const std::unordered_map<uint8_t, Instruction> table = {
    { 0xAD, Instruction ("LDA $", 0xAD, AM_ABSOLUTE, 2, 4) },
    { 0x4C, Instruction ("JMP $", 0x4C, AM_ABSOLUTE, 2, 4) }
  };
  return table;
}
/* clang-format on */
