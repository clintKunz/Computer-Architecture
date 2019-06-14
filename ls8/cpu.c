#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cpu.h"

//#define DATA_LEN 6

/**
 * Load the binary bytes from a .ls8 source file into a RAM array
 */
void cpu_load(struct cpu *cpu, char *filename)
{
  // char data[DATA_LEN] = {
  //   // From print8.ls8
  //   0b10000010, // LDI R0,8
  //   0b00000000,
  //   0b00001000,
  //   0b01000111, // PRN R0
  //   0b00000000,
  //   0b00000001  // HLT
  // };

  // int address = 0;

  // for (int i = 0; i < DATA_LEN; i++) {
  //   cpu->ram[address++] = data[i];
  // }

  // TODO: Replace this with something less hard-coded
  int address = 0;
  FILE *fp;
  char line[1024];
  printf("%s\n", filename);
  fp = fopen(filename, "r");

  if (fp == NULL) {
    fprintf(stderr, "file not found\n");
    exit(1);
  }

  while (fgets(line, sizeof line, fp) != NULL) {
    char *endptr; 

    unsigned char v = strtoul(line, &endptr, 2);

    if (endptr == line) {
      //printf("Ignoring line: %s", line);
      continue;
    }

    //printf("%u\n", v); 
    cpu->ram[address] = v;
    address++; 
  }

  fclose(fp); 
}

/**
 * ALU
 */
void alu(struct cpu *cpu, enum alu_op op, unsigned char regA, unsigned char regB)
{
  switch (op) {
    case ALU_MUL:
      cpu->registers[regA] = cpu->registers[regA] * cpu->registers[regB];
      break;
    case ALU_AND:
      cpu->registers[regA] = cpu->registers[regA] & cpu->registers[regB];
      break;
    case ALU_OR:
      cpu->registers[regA] = cpu->registers[regA] | cpu->registers[regB];
      break;
    case ALU_XOR:
      cpu->registers[regA] = cpu->registers[regA] ^ cpu->registers[regB];
      break;
    case ALU_NOT:
      cpu->registers[regA] = ~cpu->registers[regA];
      break;
  }
}

/**
 * Run the CPU
 */
void cpu_run(struct cpu *cpu)
{
  int running = 1; // True until we get a HLT instruction
  unsigned int operA;
  unsigned int operB;
  int v, reg, retaddr;
  int equal = 0;
  int lessThan = 0;
  int greaterThan = 0;

  unsigned char ir;

  while (running) {
    // TODO
    // 1. Get the value of the current instruction (in address PC).
    ir = cpu->ram[cpu->pc];
    // 2. Figure out how many operands this next instruction requires
    int numOperands = (ir >> 6) + 1;
    //printf("numOper: %d\n", numOperands);
    // 3. Get the appropriate value(s) of the operands following this instruction
    if (numOperands > 1) {
      operA = cpu->ram[cpu->pc+1];
    }
    if (numOperands > 2) {
      operB = cpu->ram[cpu->pc+2];
    }
    // 4. switch() over it to decide on a course of action.
    switch(ir) {
    // 5. Do whatever the instruction should do according to the spec.
    // 6. Move the PC to the next instruction.
      case JEQ:
        //printf("JEQ\n");
        if (equal) {
          //printf("JumpJEQ\n");
          reg = cpu->ram[cpu->pc + 1];
          cpu->pc = cpu->registers[reg];
        } else {
          cpu->pc+=numOperands; 
        }
        break;
      case JNE:
        //printf("JNE\n");
        if (!equal) {
          //printf("JumpJNE\n");
          reg = cpu->ram[cpu->pc + 1];
          cpu->pc = cpu->registers[reg];
        } else {
          cpu->pc+=numOperands; 
        }
        break;
      case JMP:
        //printf("JMP\n");
        reg = cpu->ram[cpu->pc + 1];
        cpu->pc = cpu->registers[reg]; 
        break;
      case CMP:
        //printf("CMP\n");
        if (cpu->registers[operA] == cpu->registers[operB]) {
          equal = 1;
          lessThan = 0;
          greaterThan = 0;
          //printf("Equal\n");
        } else if (cpu->registers[operA] < cpu->registers[operB]) {
          lessThan = 1;
          equal = 0;
          greaterThan = 0;
          //printf("Less Than\n");
        } else if (cpu->registers[operA] > cpu->registers[operB]) {
          greaterThan = 1;
          lessThan = 0;
          equal = 0;
          //printf("Greater Than\n");
        }
        cpu->pc+=numOperands; 
        break;
      case CALL:
        retaddr = cpu->pc + numOperands; 
        cpu->registers[SP]--;
        cpu->ram[cpu->registers[SP]] = retaddr; 
        reg = cpu->ram[cpu->pc + 1];
        cpu->pc = cpu->registers[reg];
        break;
      case RET:
        retaddr = cpu->ram[cpu->registers[SP]];
        cpu->registers[SP]++;
        cpu->pc = retaddr;
        break;
      case ADD:
        //printf("ADD\n");
        cpu->registers[operA] = cpu->registers[operA] + cpu->registers[operB];
        cpu->pc+=numOperands;
        break;
      case PUSH:
        //printf("PUSH\n");
        cpu->registers[SP]--;
        reg = operA; 
        v = cpu->registers[reg];
        cpu->ram[cpu->registers[SP]] = v;
        cpu->pc+=numOperands; 
        break;
      case POP:
        //printf("POP\n");
        reg = operA;
        v = cpu->ram[cpu->registers[SP]];
        cpu->registers[reg] = v;
        cpu->registers[SP]++;
        cpu->pc+=numOperands;
        break;
      case MUL: 
        //printf("MUL\n");
        alu(cpu, ALU_MUL, operA, operB);
        //cpu->registers[operA] = cpu->registers[operA] * cpu->registers[operB];
        cpu->pc+=numOperands;
        break;
      case LDI:
        //printf("LDI\n");
        cpu->registers[operA] = operB; 
        cpu->pc+=numOperands;
        break;
      case PRN:
        //printf("PRN\n");
        v = cpu->registers[operA];
        printf("%d\n", v);
        cpu->pc+=numOperands;
        break;
      case HLT:
        //printf("HLT\n");
        running = 0; 
        break; 
      default: 
        printf("Unknown instruction %02x at address %02x\n", ir, cpu->pc);
        cpu->pc++;
        break;  
    }
 
  }
}

/**
 * Initialize a CPU struct
 */
void cpu_init(struct cpu *cpu)
{
  // TODO: Initialize the PC and other special registers
  cpu->pc = 0;
  cpu->registers[SP] = 248; 
}
