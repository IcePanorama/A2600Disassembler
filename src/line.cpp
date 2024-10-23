#include "line.hpp"
#include "zero_page_lookup.hpp"

#include <format>

Line::Line (const uint16_t starting_addr, const Instruction &instruction,
            const std::vector<uint8_t> arguments)
    : starting_addr_ (starting_addr), instruction_ (instruction),
      arguments_ (arguments)
{
  this->assembly_instruction_ = std::format (
      "{} {}", this->instruction_.get_asm_instruction (),
      this->format_arguments (this->instruction_.get_addressing_mode (),
                              arguments_));
  this->comment_ = this->create_comments (
      this->instruction_.get_addressing_mode (), arguments_);
}

std::ostream &
operator<< (std::ostream &os, const Line &l)
{
  os << std::format ("{:04X} {} [ ", l.starting_addr_,
                     l.instruction_.to_string ());
  for (const auto &arg : l.arguments_)
    os << std::format ("{:02X} ", arg);
  os << std::format ("] {} {}", l.assembly_instruction_, l.comment_);

  return os;
}

std::string
Line::format_arguments (const AddressingMode_e &am,
                        const std::vector<uint8_t> &args)
{
  std::string output;
  switch (am)
    {
    case AM_ACCUMULATOR:
      return "A";
    case AM_ABSOLUTE:
      return this->format_absolute_addr_arguments (args);
    case AM_ABSOLUTE_X_INDEXED:
      return this->format_absolute_addr_arguments (args) + ",X";
    case AM_ABSOLUTE_Y_INDEXED:
      return this->format_absolute_addr_arguments (args) + ",Y";
    case AM_IMMEDIATE:
      return std::format ("#${:02X}", args.at (0));
    case AM_INDIRECT:
      return std::format ("(${:02X}{:02X})", args.at (0), args.at (1));
    case AM_INDIRECT_X_INDEXED:
      return std::format ("(${:02X},X)", args.at (0));
    case AM_INDIRECT_Y_INDEXED:
      return std::format ("(${:02X}),Y", args.at (0));
    case AM_RELATIVE:
      // unsure how this should be formatted
      return std::format ("{:02X}", args.at (0));
    case AM_ZERO_PAGE:
      return this->format_zero_page_addr_arguments (args.at (0));
    case AM_ZERO_PAGE_Y_INDEXED:
      return this->format_zero_page_addr_arguments (args.at (0)) + ",Y";
    case AM_ZERO_PAGE_X_INDEXED:
      return this->format_zero_page_addr_arguments (args.at (0)) + ",X";
    case AM_IMPLIED:
    default:
      return "";
    }
}

std::string
Line::format_absolute_addr_arguments (const std::vector<uint8_t> &args)
{
  std::string output = "$";
  for (const auto &arg : args)
    output += std::format ("{:02X}", arg);
  return output;
}

std::string
Line::format_zero_page_addr_arguments (const uint8_t &arg)
{
  return std::format ("${:02X}", arg);
}

std::string
Line::create_comments (const AddressingMode_e &am,
                       const std::vector<uint8_t> &args)
{
  // FIXME: this is a mess, clean up later.
  std::string output = "";
  switch (am)
    {
    case AM_ABSOLUTE:
    case AM_ABSOLUTE_X_INDEXED:
    case AM_ABSOLUTE_Y_INDEXED:
      // output.append (format_absolute_addr_arguments (args));
      break;
    case AM_ZERO_PAGE:
    case AM_ZERO_PAGE_X_INDEXED:
    case AM_ZERO_PAGE_Y_INDEXED:
      output.append (create_comments_for_zero_page_addressing (args.at (0)));
      break;
    default:
      break;
    }

  return output;
}

std::string
Line::format_comments_for_mirrored_ROM_addresses (
    const std::vector<uint8_t> &args, uint8_t mirror_start_hi)
{
  return std::format ("ROM address ${:02X}{:02X} via mirror", args.at (0),
                      args.at (1) - mirror_start_hi);
}

std::string
Line::create_comments_for_absolute_addressing (
    const std::vector<uint8_t> &args)
{
  if (0xB0 <= args.at (1) && args.at (1) <= 0xBF)
    {
      return format_comments_for_mirrored_ROM_addresses (args, 0xB0);
    }
  if (0x70 <= args.at (1) && args.at (1) <= 0x7F) //
    {
      return format_comments_for_mirrored_ROM_addresses (args, 0x70);
    }
  else if (args.at (1) >= 0xF0) // 0xF0xx .. 0xFFFF
    {
      return format_comments_for_mirrored_ROM_addresses (args, 0xF0);
    }

  return "";
}

std::string
Line::create_comments_for_zero_page_addressing (const uint8_t &arg)
{
  static const auto &zp_table = ZeroPageLookupTable::get_table ();
  if (zp_table.find (arg) != zp_table.end ())
    return std::format ("{}", zp_table.at (arg));
  else if (arg <= 0x3F)
    return "in TIA Addresses";
  else if (0x80 <= arg)
    return "in RIOT RAM";
  return "";
}