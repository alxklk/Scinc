# Scinc interpreter internals and bytecode

### _For curious and adventurous_

## VM architecture

Scinc VM is register-based, with 3 address memory-to-memory instruction set. Each command operand can be immediate integer, immediate float or address. Addresses are based upon 6 virtual segments, 4 of them are based on registers:

Segment | purpose | base register
--------|---------|--------------
`LOCAL` |local variables |`loc` register
`ARG`   |function arguments | `arg` register
`TMP`   |temporary variables, intermediate result of calculations in expressions | `tmp` register
`THIS`  |class data members for usage in methods | `this` register
`GLOBAL`|global variables | none (direct address)
`CONST` |constants | direct address

Some of these segments are considered to be extra and may be refactored out in future releases. This should not affect functionality.

Special bit in memory mode codes indirect access, `mem[addr]` if 0 or `mem[mem[addr]]` otherwise.

VM has these sontrol registers:

 reg | description
-----|------------
IP   | Instruction Pointer
SP   | Stack Pointer
this | `this` pointer for current member function
loc  | Local variables for current function
arg  | Arguments for current function
tmp  | Temporary variables

They are hidden from programmer. Each register `IP`, `this`, `loc`, `arg` and `tmp` has own stack, it is unaccessible by program and used by some commands, for example, IAM pushes current value of `this` into stack and NOTME pops - it is intended for nested method calls.

Code and data memory are separated (Harvard architecture). 
Interpreter takes next bytecode from program memory at `[IP]` and executes corresponding action. In debug mode interpreter returns control to host program after each step. In release mode interpreter returns controls only in non-linear points (JMPs, CALL) and special instructions (INT, FIN)

Return stack is used by CALL and RET commands.

Operands are denoted as `dst`, `src0` (=`src`) and `src1` in following table.

Bytecode|# operands| types         | actions
--------|----------|---------------|------
NOP     | 0        |               | No operation, default filler with code 0
CALL    | 1        | int           | Call subroutine at addr `dst` or native function if `dst`<0
RET     | 0        |               | Return from sub
IAM     | 1        | [mem]         | Load `this` pointer into register and push old value 
THAT    | 1        | [mem]         | Store `this` register into mem (`mem[dst]=this`)
NOTME   | 0        |               | Pop `this`
NEG     | 2        | [mem]         | Int negate, `dst=-src`
FNEG    | 2        | [mem]         | Float negate, `dst=-src`
AND     | 3        | [mem]         | Bitwise and, `dst&=src`
OR      | 3        | [mem]         | Bitwise or
XOR     | 3        | [mem]         | Bitwise xor
NOT     | 2        | [mem]         | Bitwise not
SHL     | 3        | [mem]         | Bitwise shift left, <<
SHR     | 3        | [mem]         | Bitwise shift right, >>
ADD     | 3        | [mem]         | Integer binary +
MUL     | 3        | [mem]         | Integer *
SUB     | 3        | [mem]         | Integer binary -
DIV     | 3        | [mem]         | Integer /
INC     | 1        | [mem]         | Integer ++
DEC     | 1        | [mem]         | Integer --
MOD     | 3        | [mem]         | Integer % (modulo)
FADD    | 3        | [mem]         | Float +
FMUL    | 3        | [mem]         | Float *
FSUB    | 3        | [mem]         | Float -
FDIV    | 3        | [mem]         | Float /
FDIV    | 3        | [mem]         | Float MODulo
CLDC    | 2        | [mem], int    | Char LoaD Constant
ILDC    | 2        | [mem], int    | Integer LoaD Constant
FLDC    | 2        | [mem], float  | Float LoaD Constant
I2F     | 2        | [mem]         | Convert Int -> Float
F2I     | 2        | [mem]         | Convert Float -> Int
I2C     | 2        | [mem]         | Convert Int -> Char
C2I     | 2        | [mem]         | Convert Char -> Int
ADJSP   | 1        | int           | ADJust SP += arg
MKRET   | 1        | int           | MaKe place for RETurn value on stack
RMRET   | 0        |               | ReMove RETurn value
PUSH    | 2        | [mem], int    | PUSH
PUSHIC  | 1        | int           | PUSH Integer Constant
PUSHFC  | 1        | float         | PUSH Float Constant
MOV     | 3        |[mem],[mem],int| MOVe (copy) scr1 bytes from src0 to dst
ENTR    | 2        |int            | ENTeR sub (reserve place for local and temp variables on stack)
EXIT    | 2        |int            | EXIT from subroutine (forget about local and temp vars)
JMPZ    | 2        | int,[mem]     | conditinao JuMP if Zero: `if(mem[src])IP+=dst;`
JMPNZ   | 2        | int,[mem]     | conditinao JuMP if NonZero: `if(!mem[src])IP+=dst;`
JMP     | 1        | int           | JuMP
LS      | 3        | [mem]         | integer LeSs, `mem[dst]=(mem[src0]<mem[src1]);`
GR      | 3        | [mem]         | integer GReater
LE      | 3        | [mem]         | integer Less or Equal
GE      | 3        | [mem]         | integer Greater or Equal
EQ      | 3        | [mem]         | integer Equal
NE      | 3        | [mem]         | integer NonEqual
FLS     | 3        | [mem]         | Float LeSs
FGR     | 3        | [mem]         | Float GReater
FLE     | 3        | [mem]         | Float Less or Equal
FGE     | 3        | [mem]         | Float Greater or Equal
FEQ     | 3        | [mem]         | Float Equal
FNE     | 3        | [mem]         | Float NonEqual
LAND    | 2        | [mem]         | Logical NOT (!)
LAND    | 3        | [mem]         | Logical AND (&&)
LOR     | 3        | [mem]         | Logical OR (||)
LEA     | 2        | [mem]         | Load Effective Address
MADD    | 3        | [mem]         | Multiply-ADD, `dst=dst+src0*src1`
INT     | 1        | int           | software INTerrupt #dst
FIN     | 1        | int           | FINish execution with result code

