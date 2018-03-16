use std::collections::HashMap;
use std::error;
use std::fmt;
use std::fs::File;
use std::io::{self, BufReader, BufRead};
use std::str;

#[derive(Debug)]
enum AsmError {
   InvalidInstruction,
   InvalidAddressing,
   InvalidLabel,
   DuplicateSymbol,
   UnresolvedReference,
   MissingRightBracket,
   ArgumentTooLarge(u16),
   BranchTargetOutOfRange(u16),
   LocationUnreachable(u16),
}

#[derive(Debug,Copy,Clone)]
enum Instruction {
   ADD,  //000
   SUB,  //004
   RSB,  //010
   SHL,  //014
   CMP,  //020
   LDA,  //024
   STA,  //030
   OUT,  //034
   JMP,  //040
   JPC,  //044
   JPN,  //050
   JMS,  //054
   NOP,  //060
   SWP,  //064
   //ILL,//070
   HLT,  //074

   // pseudo-instructions
   ORG,
   WRD
}

#[derive(Debug,Copy,Clone)]
enum Operand {
   Literal(u16),
   Label(usize),
}

#[derive(Debug,Copy,Clone)]
enum Addressing {
   CurrentPage,
   ZeroPage,
   Immediate,
   Indirect,

   // jump instructions only
   IndirectCurrentPage,
   IndirectZeroPage,
   Direct,
   Accumulator,

   // WRD and ORG instructions only
   Assembler,
}

#[derive(Debug,Copy,Clone)]
struct Opcode {
   instruction: Instruction,
   operand: Operand,
   addressing: Addressing,
}

#[derive(Debug)]
struct Line {
   num: usize,
   org: u16,
   op: Opcode,
}

#[derive(Debug)]
struct SymbolTable {
   symbols: HashMap<String, usize>,
   addresses: Vec<u16>,
}

impl fmt::Display for AsmError {
   fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
      match *self {
         AsmError::InvalidInstruction           => write!(f, "Invalid instruction"),
         AsmError::InvalidAddressing            => write!(f, "Invalid addressing mode"),
         AsmError::InvalidLabel                 => write!(f, "Invalid label"),
         AsmError::DuplicateSymbol              => write!(f, "Duplicate symbol"),
         AsmError::UnresolvedReference          => write!(f, "Unresolved reference"),
         AsmError::MissingRightBracket          => write!(f, "Missing ']'"),
         AsmError::ArgumentTooLarge(arg)        => write!(f, "Argument @{:04o} too large", arg),
         AsmError::BranchTargetOutOfRange(arg)  => write!(f, "Branch target @{:04o} out of range", arg),
         AsmError::LocationUnreachable(arg)     => write!(f, "Location @{:04o} unreachable", arg),
      }
   }
}

impl error::Error for AsmError {
   fn description(&self) -> &str {
      match *self {
         AsmError::InvalidInstruction           => "invalid instruction",
         AsmError::InvalidAddressing            => "invalid addressing mode",
         AsmError::InvalidLabel                 => "invalid label",
         AsmError::DuplicateSymbol              => "duplicate symbol",
         AsmError::UnresolvedReference          => "unresolved reference",
         AsmError::MissingRightBracket          => "missing ']'",
         AsmError::ArgumentTooLarge(_)          => "argument too large",
         AsmError::BranchTargetOutOfRange(_)    => "branch target out of range",
         AsmError::LocationUnreachable(_)       => "location unreachable",
      }
   }

   fn cause(&self) -> Option<&error::Error> {
      match *self {
         _ => None
      }
   }
}

impl SymbolTable {
   fn new() -> SymbolTable {
      SymbolTable {
         symbols: HashMap::<String, usize>::new(),
         addresses: Vec::<u16>::new()
      }
   }

   fn add_reference(&mut self, symbol: &str) -> usize {
      if self.symbols.contains_key(symbol) {
         self.symbols[symbol]
      } else {
         let index = self.symbols.len();
         self.symbols.insert(String::from(symbol), index);
         self.addresses.push(u16::max_value());
         index
      }
   }

   fn add_label(&mut self, symbol: &str, address: u16) -> Result<usize, ()> {
      let index = self.add_reference(symbol);
      if self.addresses[index] != u16::max_value() {
         Err(())
      } else {
         self.addresses[index] = address;
         Ok(index)
      }
   }

   fn resolve_reference(&self, label: usize) -> Option<u16> {
      if label < self.addresses.len() && self.addresses[label] < u16::max_value() {
         Some(self.addresses[label])
      } else {
         None
      }
   }
}

fn inst_is_jump(inst: Instruction) -> bool {
   match inst {
      Instruction::JMP => true,
      Instruction::JPC => true,
      Instruction::JPN => true,
      Instruction::JMS => true,
      _ => false
   }
}

fn inst_is_assembler(inst: Instruction) -> bool {
   match inst {
      Instruction::WRD => true,
      Instruction::ORG => true,
      _ => false
   }
}

fn inst_from_string(s: &str) -> Option<Instruction> {
   // Do upper-case conversion on the stack
   let bytes = s.as_bytes();
   if bytes.len() != 3 {
      return None;
   }

   let buf: [u8; 3] = [ bytes[0] & !0x20, bytes[1] & !0x20, bytes[2] & !0x20 ];
   let upper = unsafe {
      str::from_utf8_unchecked(&buf)
   };

   match upper {
      "ADD" => Some(Instruction::ADD),
      "SUB" => Some(Instruction::SUB),
      "RSB" => Some(Instruction::RSB),
      "SHL" => Some(Instruction::SHL),
      "CMP" => Some(Instruction::CMP),
      "LDA" => Some(Instruction::LDA),
      "STA" => Some(Instruction::STA),
      "OUT" => Some(Instruction::OUT),
      "JMP" => Some(Instruction::JMP),
      "JPC" => Some(Instruction::JPC),
      "JPN" => Some(Instruction::JPN),
      "JMS" => Some(Instruction::JMS),
      "NOP" => Some(Instruction::NOP),
      "SWP" => Some(Instruction::SWP),
      "HLT" => Some(Instruction::HLT),
      "ORG" => Some(Instruction::ORG),
      "WRD" => Some(Instruction::WRD),
      _ => None,
   }
}

fn opcode_to_int(op: &Opcode) -> u16 {
   let addr: u16 = match op.addressing {
      Addressing::CurrentPage => 0o000,
      Addressing::ZeroPage    => 0o100,
      Addressing::Immediate   => 0o200,
      Addressing::Indirect    => 0o300,

      Addressing::IndirectCurrentPage  => 0o000,
      Addressing::IndirectZeroPage     => 0o100,
      Addressing::Direct               => 0o200,
      Addressing::Accumulator          => 0o300,

      Addressing::Assembler   => 0o000,
   };

   let inst: u16 = match op.instruction {
      Instruction::ADD => 0o0000 + addr,
      Instruction::SUB => 0o0400 + addr,
      Instruction::RSB => 0o1000 + addr,
      Instruction::SHL => 0o1400 + addr,
      Instruction::CMP => 0o2000 + addr,
      Instruction::LDA => 0o2400 + addr,
      Instruction::STA => 0o3000 + addr,
      Instruction::OUT => 0o3400 + addr,
      Instruction::JMP => 0o4000 + addr,
      Instruction::JPC => 0o4400 + addr,
      Instruction::JPN => 0o5000 + addr,
      Instruction::JMS => 0o5400 + addr,
      Instruction::NOP => 0o6000 + addr,
      Instruction::SWP => 0o6400 + addr,
      Instruction::HLT => 0o7400 + addr,
      Instruction::ORG => 0o0000,
      Instruction::WRD => 0o0000,
   };

   inst
}

fn string_is_identifier(s: &str) -> bool {
   let mut chars = s.chars();

   match chars.next() {
      Some(c) => { if !c.is_alphabetic() && c != '_' { return false; } },
      None => return false,
   }

   for c in chars {
      if !c.is_alphanumeric() && c != '_' {
         return false;
      }
   }

   true
}

fn parse_number(text: &str) -> Option<u16> {
   if text.len() == 0 {
      return None;
   }

   let mut r: usize = 0;
   let mut bytes = text.bytes();
   let base;
   if text.starts_with('@') {
      base = 8;
      bytes.next();
   } else if text.starts_with('$') {
      base = 16;
      bytes.next();
   } else {
      base = 10;
   }

   for b in bytes {
      if b'0' <= b && b <= b'7' {
         r = r * base + (b - b'0') as usize;
      } else if base > 8 && (b'8' <= b && b <= b'9') {
         r = r * base + (b - b'0') as usize;
      } else if base == 16 && (b'a' <= b && b <= b'f') {
         r = r * base + (b - b'a' + 10) as usize;
      } else if base == 16 && (b'A' <= b && b <= b'F') {
         r = r * base + (b - b'A' + 10) as usize;
      } else {
         return None;
      }
   }

   if r > 0xffff { None } else { Some(r as u16) }
}

fn parse_operand(token: &str, symbols: &mut SymbolTable) -> Result<Operand, AsmError> {
   if let Some(val) = parse_number(token) {
      Ok(Operand::Literal(val as u16))
   } else if string_is_identifier(token) {
      Ok(Operand::Label(symbols.add_reference(token)))
   } else {
      Err(AsmError::InvalidLabel)
   }
}

fn parse_line(text: &str, address: u16, symbols: &mut SymbolTable) -> Result<Opcode, AsmError> {
   let mut tokens = text.split_whitespace();

   let mut token = tokens.next().unwrap();

   // label
   if token.ends_with(':') {
      if !symbols.add_label(&token[..token.len()-1], address).is_ok() {
         return Err(AsmError::DuplicateSymbol);
      }
      token = tokens.next().unwrap();
   }

   // instruction
   let instruction;
   match inst_from_string(token) {
      Some(inst) => instruction = inst,
      None => return Err(AsmError::InvalidInstruction),
   };

   let op_token = tokens.next();
   if op_token.is_none() {
      let addressing = if inst_is_assembler(instruction) { Addressing::Assembler } else { Addressing::Immediate };
      return Ok(Opcode{ instruction, operand: Operand::Literal(0), addressing });
   }

   token = op_token.unwrap();
   
   // operand
   let operand;
   let addressing;
   if inst_is_assembler(instruction) {
      // assembler (full-word)
      addressing = Addressing::Assembler;
      operand = parse_operand(token, symbols)?;
   } else if token.starts_with('[') {
      // indirect
      if !token.ends_with(']') {
         return Err(AsmError::MissingRightBracket);
      }
      addressing = if inst_is_jump(instruction) { Addressing::IndirectCurrentPage } else { Addressing::Indirect };
      operand = parse_operand(&token[1..token.len()-1], symbols)?;
    } else if token.starts_with('#') {
      // immediate
      addressing = Addressing::Immediate;
      operand = parse_operand(&token[1..], symbols)?;
   } else if token.starts_with("A+") || token.starts_with("a+") {
      // accumulator
      if !inst_is_jump(instruction) {
         return Err(AsmError::InvalidAddressing);
      }
      addressing = Addressing::Accumulator;
      operand = parse_operand(&token[2..], symbols)?;
   } else {
      // direct/current page, possibly changing to zero-page once references are resolved
      addressing = if inst_is_jump(instruction) { Addressing::Direct } else { Addressing::CurrentPage };
      operand = parse_operand(token, symbols)?;
   }

   Ok(Opcode{ instruction, operand, addressing })
}

fn parse_lines<R>(file_name: &str, reader: R, symbols: &mut SymbolTable) -> Option<Vec<Line>>
   where R: BufRead
{
   let mut line_num = 0usize;
   let mut address = 0u16;
   let mut lines = Vec::<Line>::new();
   let mut success = true;

   for t in reader.lines() {
      line_num += 1;

      let text = t.unwrap();
      let text = text.split(';').next().unwrap().trim();

      if !text.is_empty() {
         let op;
         match parse_line(&text, address, symbols) {
            Ok(o) => op = o,
            Err(e) => {
               eprintln!("{}({}): Error {}.", file_name, line_num, e);
               success = false;
               continue;
            }
         }

         if let Instruction::ORG = op.instruction {
            address = match op.operand {
               Operand::Literal(o) => o,
               _ => address,
            }
         } else {
            lines.push(Line{num: line_num, org: address, op});
            address += 1;
         }
      }
   }

   if success { Some(lines) } else { None }
}

fn assemble_line(l: &Line, symbols: &SymbolTable) -> Result<(), AsmError> {
   let mut op = l.op;
   let arg;
   match op.operand {
      Operand::Literal(lit) => arg = lit,
      Operand::Label(label) => {
         let refr = symbols.resolve_reference(label);
         if refr.is_none() {
            return Err(AsmError::UnresolvedReference);
         }
         arg = refr.unwrap();
      },
   };

   match op.addressing {
      Addressing::Immediate | Addressing::Accumulator => {
         if arg > 63 {
            return Err(AsmError::ArgumentTooLarge(arg));
         }
      },
      Addressing::CurrentPage => {
         if (arg >> 6) == 0 {
            op.addressing = Addressing::ZeroPage;
         } else if (arg >> 6) != (l.org >> 6) {
            return Err(AsmError::LocationUnreachable(arg));
         }
      },
      Addressing::Indirect => {
         if (arg >> 6) != (l.org >> 6) {
            return Err(AsmError::LocationUnreachable(arg));
         }
      },
      Addressing::IndirectCurrentPage => {
         if (arg >> 6) == 0 {
            op.addressing = Addressing::IndirectZeroPage;
         } else if (arg >> 6) != (l.org >> 6) {
            return Err(AsmError::BranchTargetOutOfRange(arg));
         }
      },
      Addressing::Direct => {
         if (arg >> 6) != (l.org >> 6) {
            return Err(AsmError::BranchTargetOutOfRange(arg));
         }
      },
      _ => (),
   }

   let opcode = if inst_is_assembler(op.instruction) { arg } else { opcode_to_int(&op) + (arg & 63) };

   let name = match op.instruction {
      Instruction::ADD => "ADD",
      Instruction::SUB => "SUB",
      Instruction::RSB => "RSB",
      Instruction::SHL => "SHL",
      Instruction::CMP => "CMP",
      Instruction::LDA => "LDA",
      Instruction::STA => "STA",
      Instruction::OUT => "OUT",
      Instruction::JMP => "JMP",
      Instruction::JPC => "JPC",
      Instruction::JPN => "JPN",
      Instruction::JMS => "JMS",
      Instruction::NOP => "NOP",
      Instruction::SWP => "SWP",
      Instruction::HLT => "HLT",
      Instruction::ORG => "ORG",
      Instruction::WRD => "WRD",
   };

   match op.addressing {
      Addressing::Immediate           => println!("{:04o}: {:04o} ; {} #{:02}",    l.org, opcode, name, arg),
      Addressing::Accumulator         => println!("{:04o}: {:04o} ; {} A+{:02o}",  l.org, opcode, name, arg),
      Addressing::ZeroPage            => println!("{:04o}: {:04o} ; {} @{:02o}",   l.org, opcode, name, arg),
      Addressing::Indirect            => println!("{:04o}: {:04o} ; {} [@{:04o}]", l.org, opcode, name, arg),
      Addressing::IndirectCurrentPage => println!("{:04o}: {:04o} ; {} [@{:04o}]", l.org, opcode, name, arg),
      Addressing::IndirectZeroPage    => println!("{:04o}: {:04o} ; {} [@{:02o}]", l.org, opcode, name, arg),
      _                               => println!("{:04o}: {:04o} ; {} @{:04o}",   l.org, opcode, name, arg),
   }

   Ok(())
}

fn assemble_file(file_name: &str, lines: &Vec<Line>, symbols: &SymbolTable) -> bool {
   let mut success = true;

   for l in lines {
      match assemble_line(l, symbols) {
         Ok(_) => (),
         Err(e) => {
            eprintln!("{}({}): Error {}.", file_name, l.num, e);
            success = false;
         }
      }
   }

   success
}

fn parse_file(name: Option<String>) -> bool {
   let mut symbols = SymbolTable::new();
   let parsed;
   let file_name: &str;
   if let Some(ref s) = name {
      file_name = s;
      let f = File::open(s).unwrap();
      parsed = parse_lines(file_name, BufReader::new(f), &mut symbols);
   } else {
      file_name = "<stdin>";
      let stdin = io::stdin();
      parsed = parse_lines(file_name, stdin.lock(), &mut symbols);
   }

   match parsed {
      Some(lines) => assemble_file(file_name, &lines, &symbols),
      None => false,
   }
}

fn main() {
   let mut args = std::env::args().skip(1);
   let exit_code = if parse_file(args.next()) { 0 } else { 1 };
   std::process::exit(exit_code);
}
