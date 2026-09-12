/*

Q4.3 — #pragma, #ifdef, #ifndef, and #pragma once

#pragma provides a way to send implementation-specific instructions to the compiler. Its behavior depends on the compiler.

#ifdef NAME includes the following code only if NAME is defined.

#ifndef NAME includes the following code only if NAME is not defined.

#pragma once is a widely supported, but non-standard, instruction asking the compiler to include a header only once per translation unit.

A portable include guard looks like this:

#ifndef MY_HEADER_H
#define MY_HEADER_H

Header declarations

#endif

Include guards are standard preprocessor logic and portable across C compilers.
#pragma once is shorter, but is not part of ISO C17.

*/