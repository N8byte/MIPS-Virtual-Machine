// All code by Nathan Waskiewicz
// Nov 23, 2021
// Computer Organization
#include "spimcore.h"


// ALU
void ALU(unsigned A,unsigned B,char ALUControl,unsigned *ALUresult,char *Zero)
{
    switch (ALUControl)
    {
        case 0:
            *ALUresult = A + B;
            break;
        case 1:
            *ALUresult = A - B;
            break;
        case 2:
            if (A << 31 == B << 31)
                *ALUresult = (A & 0x7fffffff) < (B & 0x7fffffff);
            else
                *ALUresult = (A << 31 < B << 31);
            break;
        case 3:
            *ALUresult = A < B;
            break;
        case 4:
            *ALUresult = A * B > 0;
            break;
        case 5:
            *ALUresult = A + B > 0;
            break;
        case 6:
            *ALUresult = B << 16;
            break;
        case 7:
            *ALUresult = ~A;

    }
    *Zero = *ALUresult == 0;
}

// instruction fetch
int instruction_fetch(unsigned PC,unsigned *Mem,unsigned *instruction)
{
    if (PC % 4)
        return 1;
    *instruction = MEM(PC);
    return 0;
}


// instruction partition
void instruction_partition(unsigned instruction, unsigned *op, unsigned *r1,
    unsigned *r2, unsigned *r3, unsigned *funct, unsigned *offset, unsigned *jsec)
{
    *op = instruction >> 26;
    *r1 = (instruction >> 21) & 0x1F;
	*r2 = (instruction >> 16) & 0x1F;
	*r3 = (instruction >> 11) & 0x1F;
    *funct = instruction & 0x3f;
    *offset = instruction & 0xffff;
    *jsec = instruction & 0x1ffffff;
    return;
}



// instruction decode
int instruction_decode(unsigned op,struct_controls *controls)
{
    controls->RegDst = (op >> 3) == 0b101 || (op >> 3) == 0b000000;

    controls->Jump = op == 0b000010;

    controls->Branch = op == 0b000100;

    controls->MemRead = (op >> 3) == 0b100;

    controls->MemtoReg = op == 0b100011;

    switch (op)
    {
        case 0b000000: //r-type
            controls->ALUOp = 0b111;
            break;
        case 0b001000: //addi
            controls->ALUOp = 0b000;
            break;
        case 0b000100: //beq
            controls->ALUOp = 0b001;
            break;
        case 0b001010: //slti
            controls->ALUOp = 0b010;
            break;
        case 0b001011: //sltiu
            controls->ALUOp = 0b011;
            break;
        case 0b001100: //andi
            controls->ALUOp = 0b100;
            break;
        case 0b001101: //ori
            controls->ALUOp = 0b101;
            break;
        case 0b001111: //lui
            controls->ALUOp = 0b110;
            break;
        case 0b100011: //lw
        case 0b101011: //sw
        case 0b000010: //jump
            controls->ALUOp = 0b111;
            break;
        default:
            return 1;
    }

	controls->MemWrite = (op >> 3) == 0b101;

	controls->ALUSrc = (op >> 3) == 0b001 || (op >> 3) == 0b100 || (op >> 3) == 0b101;

	controls->RegWrite = (op >> 3 == 0b001 || op >> 3 == 0b100 || op == 0b000000);

    return 0;
}

// Read Register
void read_register(unsigned r1,unsigned r2,unsigned *Reg,unsigned *data1,unsigned *data2)
{
    *data1 = Reg[r1];
    *data2 = Reg[r2];
}


// Sign Extend
void sign_extend(unsigned offset,unsigned *extended_value)
{
    if (offset >> 15)
        *extended_value = 0xffff0000 + offset;
    else
        *extended_value = 0x00000000 + offset;
}

// ALU operations
int ALU_operations(unsigned data1,unsigned data2,unsigned extended_value,
    unsigned funct,char ALUOp,char ALUSrc,unsigned *ALUresult,char *Zero)
{
    char ALUControl;
    if (ALUOp == 0b111) //r-type, now check funct
    {
        switch (funct)
        {
            case 0b100000: //add
                ALUControl = 0b000;
                break;
            case 0b100010: //subtract
                ALUControl = 0b001;
                break;
            case 0b100100: //and
                ALUControl = 0b100;
                break;
            case 0b100101: //or
                ALUControl = 0b101;
                break;
            case 0b101010: //slt
                ALUControl = 0b010;
                break;
            case 0b101011: //sltu
                ALUControl = 0b011;
                break;
        }
    }
    else
        ALUControl = ALUOp;

    if (ALUSrc)
    {
        ALU(data1, extended_value, ALUControl, ALUresult, Zero);
    }
    else
    {
        ALU(data1, data2, ALUControl, ALUresult, Zero);
    }
    return 0;
}

// Read / Write Memory
int rw_memory(unsigned ALUresult,unsigned data2,char MemWrite,char MemRead,unsigned *memdata,unsigned *Mem)
{
    //check word alignment
    if (((unsigned long)memdata % 4) || ((unsigned long)Mem % 4))
        return 1;

    if (MemWrite)
    {
        MEM(ALUresult) = data2;
    }
    if (MemRead)
    {
        if (ALUresult % 4)
            return 1;
        *memdata = MEM(ALUresult);
    }
    return 0;
}


// Write Register
void write_register(unsigned r2,unsigned r3,unsigned memdata,
    unsigned ALUresult,char RegWrite,char RegDst,char MemtoReg,unsigned *Reg)
{
    if (RegWrite)
    {
        if (MemtoReg)
        {
            if (!RegDst)
                Reg[r2] = memdata;
            else
                Reg[r3] = memdata;
        }
        else
        {
            if (!RegDst)
                Reg[r2] = ALUresult;
            else
                Reg[r3] = ALUresult;
        }
    }
    return;
}

// PC update
void PC_update(unsigned jsec,unsigned extended_value,char Branch,char Jump,char Zero,unsigned *PC)
{
    *PC += 4;

    if (Zero && Branch)
    {
        if (*PC << 31)
            *PC += (extended_value - 1 << 2);
        else
            *PC += extended_value << 2;
    }
    if (Jump)
    {
        *PC = jsec << 2;
    }
}
