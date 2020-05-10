# Scinc

![Scinc picture](scinc.png)

Scriptable C IN C / scinc (skink)

(pronounced as \[stsɪnk\])

_Skinks are lizards belonging to the family Scincidae and the infraorder Scincomorpha._
_(Wikipedia)_

Small and simple statically strong typed embeddable interpreted programming language.

Syntactical and semantical subset of C++. Can be embedded in another applications or run by interpreter. Scinc program is correct C++ program and thus can be compiled into native code without changes.

Current VM realization runs on own virtual processor and virtual address space. Speed is about 10-12 host CPU tacts per VM opcode. On modern 4.5 GHz desktop CPU it is ~420 million vm opcodes per second.

## Motivation

Language itself is intended to be reasonably limited subset of C++, moving towards better compatibility with standard compilers. Constantly growing set of minimal feature tests is used to check it after all changes in lexer, parser, backend or VM. There are thereasons, why C++ was choosen:

* author knows C++ well enough
* standard language with competing compilers allows easy and reliable tests: just compile test program with G++/Clang/MSVC and compare output stringwise
* no version compatibility quiestions: new versions of Scinc will be more and more compatible with C++ _(hopefully)_

## Language features and description

### So far, these base data types are supported:

Name|example|meaning
----|-------|----
`char`|`'a'`, `"12345"`|1 byte character. Single char literal implicitly converted to int, string literal "123456" is stored in constant segment, implicitly terminated with `'\0'` and converted to `char*` pointer to 0th element
`int`|`3`, `0xff`|signed integer, underlying type is default 32 bit signed int
`float`|`3.14`, `.5f`, `-1.5e+3`|IEEE float. Underlying native type is `double`, so, `sizeof(float)` equals `8` in Scinc

Arrays with compile time known sizes, structs, classes, pointers and references can be used to extend types. Struct can contain arrays or other structs, array can contain structs.

Major type conversions between base types are supported, however, they can cause extra operations and sometimes buggy or missing. It is known issue and author works on fixes and improvements

`struct`, `class` correspond to their C++ analogs. For now, there are some limitations:
* no bit fields
* no constructors and destructors
* members access (`public:`, `private:`, `protected:`) ignored, `const` is ignored too
* user defined overloaded operators don't support freestanding implementation yet
* `operator[]` and `operator->` cannot be overloaded for now

Function member can be declared as `static` and works reasonably well.

Variable declarations support only one variable at a time: `int a; int b;` is OK, `int a,b;` will cause an error.

Global variables can not be initialized.

Local variable deslarations support initialization, arrays can be initialized with initialization lists: `int x[3]={1,2,3};`, missing items are implicitly 0. Be aware of the fact, that list is stored in constant segment and event empty list will occupy space, for example, `int x[4]={};` will be compiled into 4*sizeof(int) zero bytes.


Pointer and references are supported, but their usage in variable declarations is limited to one at a time, i.e. `int * ptrx;` is OK, `int ** ptrptrx;` is not.

## Preprocessor

Preprocessor supports object-style and funstion-style macros, `__SCINC__` macro is defined, can be useful if someone needs conditional compilation

`#include <...>` is ignored, `#include "..."` works, but only in current folder.

## Error messages

Error messages are sometimes confusing or missing, sometimes extra. No file/line/position included in error text, but someone can run Scinc with `-gui` command line switch, and find erroneous token highlighted if he is lucky enough

![Error picture](error.png)
