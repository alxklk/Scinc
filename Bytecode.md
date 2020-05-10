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

VM has these сontrol registers:

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
Indirect memory locations are enclosed in additional [].

Bytecode|# operands| types         | actions                  | rough C++ equivalent
--------|----------|---------------|--------------------------|-----------------------
NOP     | 0        |               | No operation, default filler with code 0 | `;` (empty expression)
CALL    | 1        | int           | Call subroutine at addr `dst` or native function if `dst`<0 | `func();`
RET     | 0        |               | Return from sub | `return;`
IAM     | 1        | [mem]         | Load `this` pointer into register and push old value | `someObject.someMethod();`
THAT    | 1        | [mem]         | Store `this` register into mem | `someVar=this;` or `return this;`
NOTME   | 0        |               | Pop `this` | `return;` from member function
NEG     | 2        | [mem]         | Int negate | `mem[dst]=-mem[src];`
FNEG    | 2        | [mem]         | Float negate | `mem[dst]=-mem[src];`
AND     | 3        | [mem]         | Bitwise and | `mem[dst]=mem[src0]&mem[src1];`
OR      | 3        | [mem]         | Bitwise or | `mem[dst]=mem[src0]\|mem[src1];`
XOR     | 3        | [mem]         | Bitwise xor | `mem[dst]=mem[src0]^mem[src1];`
NOT     | 2        | [mem]         | Bitwise not | `mem[dst]=~mem[src0];`
SHL     | 3        | [mem]         | Bitwise shift left, << | `mem[dst]=mem[src0]<<mem[src1];`
SHR     | 3        | [mem]         | Bitwise shift right, >> | `mem[dst]=mem[src0]>>mem[src1];`
ADD     | 3        | [mem]         | Integer binary + | `mem[dst]=mem[src0]+mem[src1];`
MUL     | 3        | [mem]         | Integer * | `mem[dst]=mem[src0]*mem[src1];`
SUB     | 3        | [mem]         | Integer binary - | `mem[dst]=mem[src0]-mem[src1];`
DIV     | 3        | [mem]         | Integer / | `mem[dst]=mem[src0]/mem[src1];`
INC     | 1        | [mem]         | Integer ++ | `mem[dst]++;`
DEC     | 1        | [mem]         | Integer -- | `mem[dst]--;`
MOD     | 3        | [mem]         | Integer % (modulo) | `mem[dst]=mem[src0]%mem[src1];`
FADD    | 3        | [mem]         | Float + | `mem[dst]=mem[src0]+mem[src1];`
FMUL    | 3        | [mem]         | Float * | `mem[dst]=mem[src0]*mem[src1];`
FSUB    | 3        | [mem]         | Float - | `mem[dst]=mem[src0]-mem[src1];`
FDIV    | 3        | [mem]         | Float / | `mem[dst]=mem[src0]/mem[src1];`
FDIV    | 3        | [mem]         | Float MODulo | `mem[dst]=fmod(mem[src0],mem[src1]);`
CLDC    | 2        | [mem], int    | Char LoaD Constant | `mem[dst]=(unsigned char)src;`
ILDC    | 2        | [mem], int    | Integer LoaD Constant | `mem[dst]=src;`
FLDC    | 2        | [mem], float  | Float LoaD Constant | `mem[dst]=src;`
I2F     | 2        | [mem]         | Convert Int -> Float | `mem[dst]=mem[src];`
F2I     | 2        | [mem]         | Convert Float -> Int | `mem[dst]=(int)mem[src];`
I2C     | 2        | [mem]         | Convert Int -> Char | `mem[dst]=(unsigned char)mem[src];`
C2I     | 2        | [mem]         | Convert Char -> Int | `mem[dst]=mem[src];`
ADJSP   | 1        | int           | ADJust SP | no equivalent, `SP+=dst`
MKRET   | 1        | int           | MaKe place for RETurn value on stack | no direct equivalent, `stack[SP++]=arg;arg=SP;`
RMRET   | 0        |               | ReMove RETurn value | no direct equivalent, `arg=stack[SP--];`
PUSH    | 2        | [mem], int    | PUSH | function argument, `memcpy(SP,src0,src1);SP+=src1;`
PUSHIC  | 1        | int           | PUSH Integer Constant | function argument
PUSHFC  | 1        | float         | PUSH Float Constant | function argument
MOV     | 3        |[mem],[mem],int| MOVe (copy) scr1 bytes from src0 to dst | `var1=var0` or `memcpy(dst,src0,src1)`
ENTR    | 2        |int            | ENTeR sub (reserve place for local and temp variables on stack)
EXIT    | 2        |int            | EXIT from subroutine (forget about local and temp vars)
JMPZ    | 2        | int,[mem]     | conditinao JuMP if Zero | `if(mem[src])IP+=dst;`
JMPNZ   | 2        | int,[mem]     | conditinao JuMP if NonZero | `if(!mem[src])IP+=dst;`
JMP     | 1        | int           | JuMP | `goto label;`
LS      | 3        | [mem]         | integer LeSs | `mem[dst]=(mem[src0]<mem[src1]);`
GR      | 3        | [mem]         | integer GReater | `mem[dst]=(mem[src0]>mem[src1]);`
LE      | 3        | [mem]         | integer Less or Equal | `mem[dst]=(mem[src0]<=mem[src1]);`
GE      | 3        | [mem]         | integer Greater or Equal | `mem[dst]=(mem[src0]>=mem[src1]);`
EQ      | 3        | [mem]         | integer Equal | `mem[dst]=(mem[src0]==mem[src1]);`
NE      | 3        | [mem]         | integer NonEqual | `mem[dst]=(mem[src0]!=mem[src1]);`
FLS     | 3        | [mem]         | Float LeSs | `mem[dst]=(mem[src0]<mem[src1]);`
FGR     | 3        | [mem]         | Float GReater | `mem[dst]=(mem[src0]>mem[src1]);`
FLE     | 3        | [mem]         | Float Less or Equal | `mem[dst]=(mem[src0]<=mem[src1]);`
FGE     | 3        | [mem]         | Float Greater or Equal | `mem[dst]=(mem[src0]>=mem[src1]);`
FEQ     | 3        | [mem]         | Float Equal | `mem[dst]=(mem[src0]==mem[src1]);`
FNE     | 3        | [mem]         | Float NonEqual | `mem[dst]=(mem[src0]!=mem[src1]);`
LNOT    | 2        | [mem]         | Logical NOT | `mem[dst]=!mem[src];`
LAND    | 3        | [mem]         | Logical AND | `mem[dst]=(mem[src0]&&mem[src1]);`
LOR     | 3        | [mem]         | Logical OR | `mem[dst]=(mem[src0]\|\|mem[src1]);`
LEA     | 2        | [mem]         | Load Effective Address | `mem[dst]=&mem[src0];`
MADD    | 3        | [mem]         | Multiply-ADD | `dst=dst+src0*src1`
INT     | 1        | int           | software INTerrupt | `__asm(int dst)`
FIN     | 1        | int           | FINish execution with result code | `exit(code);` or `return code;` from main

There is no (and probably will not be) assembler tool, that uses the language, described in this document. However, knowing the syntax can be useful for debugging and overall understanding what is going on.
