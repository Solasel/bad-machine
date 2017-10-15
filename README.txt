$$$$$$$$$$$$$$$$$$$$$$
$$ bad-machine-code $$
$$$$$$$$$$$$$$$$$$$$$$

#########
= SPEC: =
#########

16 bit regs/instructions
8 regs (3 bit specifications)
word-aligned addressing

Instruction Format:

OPC - Opcode
rs0 - Register to write to
rs1 - Register argument
rs2 - Register argument
S1  - Primary Function Selector
S2  - Secondary Function Selector
Imm - Immediate, scrambled around
	as necessary

	- abbreviated as "i" below

########################
======= Formats: =======
########################

R Format:	OPC = 00

S2 rs2 rs1  S1 rs0 OPC
 |  |   |   |   |  |
FE|DCB|A98|765|432|10

~~~~~~~~~~~~~~~~~~~~~~~~

I Format:	OPC = 01

i[4-0]
  |   rs1  S1 rs0 OPC
  |    |   |   |  |
FEDCB|A98|765|432|10

~~~~~~~~~~~~~~~~~~~~~~~~

S Format:	OPC = 10

i[4-3]        i[2-0]
 | rs2 rs1  S1  |  OPC
 |  |   |   |   |  |
FE|DCB|A98|765|432|10

~~~~~~~~~~~~~~~~~~~~~~~~

U Format:	OPC = 11

  i[15-5]   rs0 OPC
     |       |  |
FEDCBA98765|432|10

~~~~~~~~~~~~~~~~~~~~~~~~

#################
= Instructions: =
#################

R : 00
======

000 | 00 : add	# rs0 = rs1 + rs2
000 | 01 : sub	# rs0 = rs1 - rs2

001 | 00 : slt	# rs0 = rs1 < rs2
001 | 01 : sltu # rs0 = rs1 < rs2 (rs2 unsigned)

010 | 00 : sll	# rs0 = rs1 << rs2

011 | 00 : sra	# rs0 = rs1 >> rs2 "sign bit in leftmost
011 | 01 : srl	# rs0 = rs1 >> rs2 "0's in leftmost

100 | 00 : or	# rs0 = rs1 OR rs2
100 | 01 : nor	# rs0 = ~(rs1 OR rs2)

101 | 00 : and	# rs0 = rs1 AND rs2
101 | 01 : nand	# rs0 = ~(rs1 AND rs2)

110 | 00 : xor	# rs0 = rs1 XOR rs2
110 | 01 : xnor	# rs0 = ~(rs1 XOR rs2)

111 | 00 : mul  # rs0 = [rs1 * rs2](31-0) 
111 | 01 : mulh # rs0 = [rs1 * rs2](63-32)
111 | 10 : div	# rs0 = rs1 / rs2
111 | 11 : rem	# rs0 = rem(rs1 / rs2)

I : 01
======
000 : addi	# rs0 = rs1 + imm
001 : slti	# rs0 = rs1 < imm
010 : slli	# rs0 = rsi << imm
011 : srai	# rs0 = rsi >> imm "sign bit in leftmost
100 : ori 	# rs0 = rsi OR imm
101 : andi	# rs0 = rsi AND imm
110 : xori	# rs0 = rsi XOR imm
111 : lw	# rs0 = RAM(rs1 + imm);


S : 10
======
000 : sw	# RAM(rs1 + imm) = rs2


U : 11
======
lui		# rs0 = imm << 5

$$$$$$$$$$$$$$$$$$$$$$$$$$$
$$ bad-assembly-language $$
$$$$$$$$$$$$$$$$$$$$$$$$$$$

#########
= USAGE =
#########

The commands are exactly those above, there is no shorthand.

R-type:		cmd rs0, rs1, rs2
I-type:		cmd rs0, rs1, imm
S-type: 	cmd rs2, rs1, imm
U-type:		lui rs0, imm

where rs0, rs1, and rs2 are x0-x7, and imm is a decimal number.

Commands are separated by newlines, and parts of commands
	are separated by commas and whiespace (including tabs).

Note that if the imm field is larger than can be fit in the given instruction,
	its upper bits are truncated.

