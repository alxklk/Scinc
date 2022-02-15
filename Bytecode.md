# Scinc interpreter internals and bytecode

### _For curious and adventurous_

## VM architecture

Scinc VM is register-based, with 3 address memory-to-memory instruction set. Each command operand can be immediate integer, immediate float or address. Addresses are based upon 6 virtual segments, 4 of them are based on registers:

Address<br>Mode| Purpose                                                                                                                | Base<br>Register
---------------|------------------------------------------------------------------------------------------------------------------------|--------------
`Local`        | function local variables and arguments                                                                                 |`sp` 
`This`         | class data members accessed from methods                                                                               | `this`
`Global`       | constants, global variables, variables accessed by reference or by pointer dereferencing, and dynamicly allocated data | none

Special bit in memory mode codes indirect access, `mem[addr]` if 0 or `mem[mem[addr]]` otherwise.

VM has these сontrol registers:

Register | description
-------|------------
IP     | Instruction Pointer
SP     |  Stack Pointer
this   | `this` pointer for current member function

They are hidden from programmer. Each register `IP`, `this`, `loc`, `arg` and `tmp` has own stack, it is unaccessible by program and used by some commands, for example, IAM pushes current value of `this` into stack and NOTME pops - it is intended for nested method calls.

Code and data memory are separated (Harvard architecture). 
Interpreter takes next bytecode from program memory at `[IP]` and executes corresponding action. In debug mode interpreter returns control to host program after each step. In release mode interpreter returns controls only in non-linear points (JMPs, CALL) and special instructions (INT, FIN)

Return stack is used by CALL and RET commands.

Operands are denoted as `dst`, `src0` (=`src`) and `src1` in following table.
Indirect memory locations are enclosed in additional [].

 Bytecode | # operands | types           | actions                                                         | rough C++ equivalent                 
----------|------------|-----------------|-----------------------------------------------------------------|--------------------------------------
 NOP      | 0          |                 | No operation, default filler with code 0                        | `;` (empty expression)               
 CALL     | 1          | int             | Call subroutine at addr `dst`                                   | `func();`                            
 NCALL    | 1          | int             | Call native                                                     | `func();`                            
 RET      | 0          |                 | Return from sub                                                 | `return;`                            
 IAM      | 1          | [mem]           | Load `this` pointer into register and push old value            | `someObject.someMethod();`           
 THAT     | 1          | [mem]           | Store `this` register into mem                                  | `someVar=this;` or `return this;`    
 NOTME    | 0          |                 | Pop `this`                                                      | `return;` from member function       
 NEG      | 2          | [mem]           | Int negate                                                      | `mem[dst]=-mem[src];`                
 FNEG     | 2          | [mem]           | Float negate                                                    | `mem[dst]=-mem[src];`                
 AND      | 3          | [mem]           | Bitwise and                                                     | `mem[dst]=mem[src0]&mem[src1];`      
 OR       | 3          | [mem]           | Bitwise or                                                      | `mem[dst]=mem[src0]\                 |mem[src1];`
 XOR      | 3          | [mem]           | Bitwise xor                                                     | `mem[dst]=mem[src0]^mem[src1];`      
 NOT      | 2          | [mem]           | Bitwise not                                                     | `mem[dst]=~mem[src0];`               
 SHL      | 3          | [mem]           | Bitwise shift left, <<                                          | `mem[dst]=mem[src0]<<mem[src1];`     
 SHR      | 3          | [mem]           | Bitwise shift right, >>                                         | `mem[dst]=mem[src0]>>mem[src1];`     
 ADD      | 3          | [mem]           | Integer binary +                                                | `mem[dst]=mem[src0]+mem[src1];`      
 MUL      | 3          | [mem]           | Integer *                                                       | `mem[dst]=mem[src0]*mem[src1];`      
 SUB      | 3          | [mem]           | Integer binary -                                                | `mem[dst]=mem[src0]-mem[src1];`      
 DIV      | 3          | [mem]           | Integer /                                                       | `mem[dst]=mem[src0]/mem[src1];`      
 INC      | 1          | [mem]           | Integer ++                                                      | `mem[dst]++;`                        
 DEC      | 1          | [mem]           | Integer --                                                      | `mem[dst]--;`                        
 MOD      | 3          | [mem]           | Integer % (modulo)                                              | `mem[dst]=mem[src0]%mem[src1];`      
 FADD     | 3          | [mem]           | Float +                                                         | `mem[dst]=mem[src0]+mem[src1];`      
 FMUL     | 3          | [mem]           | Float *                                                         | `mem[dst]=mem[src0]*mem[src1];`      
 FSUB     | 3          | [mem]           | Float -                                                         | `mem[dst]=mem[src0]-mem[src1];`      
 FDIV     | 3          | [mem]           | Float /                                                         | `mem[dst]=mem[src0]/mem[src1];`      
 FDIV     | 3          | [mem]           | Float MODulo                                                    | `mem[dst]=fmod(mem[src0],mem[src1]);` 
 CLDC     | 2          | [mem], int      | Char LoaD Constant                                              | `mem[dst]=(unsigned char)src;`       
 ILDC     | 2          | [mem], int      | Integer LoaD Constant                                           | `mem[dst]=src;`                      
 FLDC     | 2          | [mem], float    | Float LoaD Constant                                             | `mem[dst]=src;`                      
 I2F      | 2          | [mem]           | Convert Int -> Float                                            | `mem[dst]=mem[src];`                 
 F2I      | 2          | [mem]           | Convert Float -> Int                                            | `mem[dst]=(int)mem[src];`            
 I2C      | 2          | [mem]           | Convert Int -> Char                                             | `mem[dst]=(unsigned char)mem[src];`  
 C2I      | 2          | [mem]           | Convert Char -> Int                                             | `mem[dst]=mem[src];`                 
 ADDSP    | 1          | int             | ADD constant to SP (create a stack frame inside function)       | no equivalent, `SP+=dst`             
 MOV      | 3          | [mem],[mem],int | MOVe (copy) scr1 bytes from src0 to dst                         | `var1=var0` or `memcpy(dst,src0,src1)` 
 JMPZ     | 2          | int,[mem]       | conditinal JuMP if Zero                                         | `if(mem[src])IP+=dst;`               
 JMPNZ    | 2          | int,[mem]       | conditinal JuMP if NonZero                                      | `if(!mem[src])IP+=dst;`              
 JMP      | 1          | int             | JuMP                                                            | `goto label;`                        
 LS       | 3          | [mem]           | integer LeSs                                                    | `i<j`                                
 GR       | 3          | [mem]           | integer GReater                                                 | `i>j`                                
 LE       | 3          | [mem]           | integer Less or Equal                                           | `i<=j`                               
 GE       | 3          | [mem]           | integer Greater or Equal                                        | `i>=j`                               
 EQ       | 3          | [mem]           | integer Equal                                                   | `i==j`                               
 NE       | 3          | [mem]           | integer NonEqual                                                | `i!=j`                               
 FLS      | 3          | [mem]           | Float LeSs                                                      | `x<y`                                
 FGR      | 3          | [mem]           | Float GReater                                                   | `x>y`                                
 FLE      | 3          | [mem]           | Float Less or Equal                                             | `x<=y`                               
 FGE      | 3          | [mem]           | Float Greater or Equal                                          | `x>=y`                               
 FEQ      | 3          | [mem]           | Float Equal                                                     | `x==y`                               
 FNE      | 3          | [mem]           | Float NonEqual                                                  | `x!=y`                               
 LNOT     | 2          | [mem]           | Logical NOT (!)                                                 | `!a`                                 
 LAND     | 3          | [mem]           | Logical AND (&&)                                                | `a&&b`                               
 LOR      | 3          | [mem]           | Logical OR (                                                    | <code>a&#124;&#124;b</code>         ||b`
 LEA      | 2          | [mem]           | Load Effective Address                                          | `void* p=(void*)&...`                
 IDX      | 3          | [mem]           | Index array, calculate address by array index, dst=dst+src0*arg | `void* p=(void*)&a[i];`              
 OFFS     | 3          | [mem],[mem],int | Offset structure field dst=dst+arg                              | `p.x`                                
 INT      | 1          | int             | software INTerrupt                                              | `__asm("int ...");`                  
 FIN      | 1          | int             | FINish execution with result code                               | `exit(n)`                            

There is no (and probably will not be) assembler tool, that uses the language, described in this document. However, knowing the syntax can be useful for debugging and overall understanding what is going on.
