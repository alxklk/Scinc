# Scinc
Scriptable C IN C / scinc or skink

_Skinks are lizards belonging to the family Scincidae and the infraorder Scincomorpha._
_(Wikipedia)_

Small and simple statically strong typed embeddable interpreted programming language.

Syntactical and semantical subset of C++. Can be embedded in another applications or run by interpreter. Skink program is correct C++ program and thus can be compiled into native code without changes.

Current VM realization runs on own virtual processor and virtual address space. Speed is about 10-12 host CPU tacts per VM opcode. On modern 4.5 GHz CPU it is ~420 million vm opcodes per second.

![Scinc picture](skink.png)
