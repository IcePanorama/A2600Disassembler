//  Copyright (C) 2024 IcePanorama
//
//  This file is a part of A2600Disassembler.
//
//  A2600Disassembler is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by the
//  Free Software Foundation, either version 3 of the License, or (at your
//  option) any later version.
//  This program is distributed in the hope that it will be useful, but WITHOUT
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
//  more details.
//
//  You should have received a copy of the GNU General Public License along
//  with this program.  If not, see <https://www.gnu.org/licenses/>.
#include "disassembler.hpp"
#include "instruction_lookup.hpp"
#include "time.hpp"

#include <cstdint>
#include <format>
#include <iostream>
#include <optional>
#include <stdexcept>

void
Disassembler::initialize (const std::string &input_filename,
                          const std::string &output_filename)
{
  this->input_fptr.open (input_filename, std::ios::binary);
  if (!this->input_fptr.is_open ())
    throw std::runtime_error (
        std::format ("Error opening input file, {}.\n", input_filename));

  this->output_fptr.open (output_filename, std::ios::binary);
  if (!this->output_fptr.is_open ())
    throw std::runtime_error (
        std::format ("Error opening output file, {}.\n", output_filename));

  this->create_file_header (input_filename);
}

void
Disassembler::create_file_header (const std::string &input_filename)
{
  /*
   *  `std::string` can't be a constexpr--causes CC to complain. Using a
   *  cstring as an easy fix.
   */
  constexpr const char *header_format
      = ";  Original filename: {}\n;  Generated {}\n;  Created using "
        "A2600Disassembler <IcePanorama/A2600Disassembler>\n";
  const std::string header
      = std::format (header_format, input_filename, get_current_time ());

  this->output_fptr.write (header.c_str (), header.length ());
}

void
Disassembler::process_file (void)
{
  const auto &table = InstructionLookupTable::get_table ();

  char b;
  while (this->input_fptr.read (&b, sizeof (b)))
    {
      if (table.find (b) == table.end ())
        throw std::runtime_error (std::format (
            "Error: instruction not found for op code, {:02X}\n", b));

      this->process_instruction (
          table.at (b), static_cast<uint16_t> (this->input_fptr.tellg ()) - 1);
    }

  export_program ();
}

void
Disassembler::process_instruction (const Instruction &i, uint16_t location)
{
  std::vector<uint8_t> arguments (i.get_num_arguments ());
  input_fptr.read (reinterpret_cast<char *> (arguments.data ()),
                   arguments.size ());

  arguments.resize (static_cast<size_t> (input_fptr.gcount ()));
  if (input_fptr.eof ())
    {
      leftover_bytes.push_back (i.get_opcode ());
      for (const uint8_t &b : arguments)
        leftover_bytes.push_back (b);
      return;
    }

  lines.push_back (Line (location, i, arguments));
}

void
Disassembler::export_program (void)
{
  for (Line &l : this->lines)
    {
      /*
       *  Line starting addr is in big endian form, whereas the label lookup
       *  table needs it in little endian form, hence this weird
       *  hackjob/conversion going on.
       */
      uint16_t le_start_addr = l.get_starting_addr ();
      le_start_addr = ((le_start_addr & 0xFF) << 0x8)
                      | ((le_start_addr & 0xFF00) >> 0x8);

      /* Add label to the current line if 2+ other lines points here. */
      std::optional<Label> label = Label::find_label (le_start_addr);
      if (label.has_value () && label->get_num_usages () > 1)
        {
          const std::string label_str
              = std::format ("{}:\n", label->to_string ());
          this->output_fptr.write (label_str.c_str (), label_str.length ());
        }

      const std::string curr_output = std::format ("{}\n", l.to_string ());
      this->output_fptr.write (curr_output.c_str (), curr_output.length ());
    }

  this->export_leftover_bytes ();
}

void
Disassembler::export_leftover_bytes (void)
{
  const uint16_t addr = this->lines.back ().get_starting_addr ()
                        + this->lines.back ().get_instruction_length ();

  std::string leftovers = std::format ("  {:04X}  ", addr);
  for (const uint8_t &b : this->leftover_bytes)
    leftovers += std::format ("{:02X} ", b);

  if (!leftovers.empty ())
    leftovers.pop_back ();

  this->output_fptr.write (leftovers.c_str (), leftovers.length ());
}
