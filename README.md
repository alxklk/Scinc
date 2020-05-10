# Scinc
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

So far, these base data types are supported:

Name|example|meaning
----|-------|----
`char`|`'a'`, `"12345"`|1 byte character. Single char literal implicitly converted ti int, string literal "123456" is stored in constant segment, implicitly terminated with `'\0'` and converted to `char*` pointer to 0th element
`int`|`3`, `0xff`|signed integer
`float`|`3.14`, `.5`, `-1.5e+3`|IEEE float. Underlying native type is `double`, so, `sizeof(float)` equals `8` in Scinc



![Scinc picture](scinc.png)
