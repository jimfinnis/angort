/**
 * @file exceptsymbs.h
 * @brief  Defines of exception symbols - use or add to these,
 * don't just create new exceptions.
 * This file is created by genexcepts from excepts,
 *
 * DO NOT EDIT MANUALLY
 *
 * (although this file must be created using genexcepts manually,
 * it's not part of the build process.)
 */

#ifndef __EXCEPTSYMBS_H
#define __EXCEPTSYMBS_H
// not a collection
#define EX_NOTCOLL                     "ex$notcoll"
// item not found
#define EX_NOTFOUND                    "ex$notfound"
// internal error
#define EX_WTF                         "ex$wtf"
// bad conversion
#define EX_BADCONV                     "ex$badconv"
// division by zero
#define EX_DIVZERO                     "ex$divzero"
// syntax error (generic)
#define EX_SYNTAX                      "ex$syntax"
// already defined ident
#define EX_DEFINED                     "ex$defined"
// assertion failed
#define EX_ASSERT                      "ex$assert"
// bad parameter type
#define EX_BADPARAM                    "ex$badparam"
// too many local variables
#define EX_TOOMANYLOCALS               "ex$toomanylocals"
// not in an iterator loop
#define EX_NOITERLOOP                  "ex$noiterloop"
// reference count error
#define EX_REFS                        "ex$refs"
// attempt to modify iterated hash
#define EX_HASHMOD                     "ex$hashmod"
// attempt to store in a non-reference val
#define EX_NOTREF                      "ex$notref"
// type is not hashable
#define EX_NOHASH                      "ex$nohash"
// type is not iterable
#define EX_NOITER                      "ex$noiter"
// not a callable (function)
#define EX_NOTFUNC                     "ex$notfunc"
// calling a deferred func
#define EX_DEFCALL                     "ex$defcall"
// string error thrown by native func
#define EX_NATIVE                      "ex$native"
// throw should throw symbol
#define EX_BADTHROW                    "ex$badthrow"
// unhandled exception
#define EX_UNHANDLED                   "ex$unhandled"
// unknown opcode
#define EX_BADOP                       "ex$badop"
// not supported
#define EX_NOTSUP                      "ex$notsup"
// unable to CD in include
#define EX_BADINC                      "ex$badinc"
// attempt to set constant
#define EX_SETCONST                    "ex$setconst"
// type invalid in this operation
#define EX_TYPE                        "ex$type"
// name too long
#define EX_LONGNAME                    "ex$longname"
// shared library load error
#define EX_LIBRARY                     "ex$library"
// corrupt data
#define EX_CORRUPT                     "ex$corrupt"
// value out of range
#define EX_OUTOFRANGE                  "ex$outofrange"
// system not ready for operation
#define EX_NOTREADY                    "ex$notready"
// operation failed
#define EX_FAILED                      "ex$failed"
// out of memory
#define EX_NOMEM                       "ex$nomem"
// cannot modify iterated list
#define EX_MODITER                     "ex$moditer"
// not a numeric value
#define EX_NOTNUMBER                   "ex$notnumber"
#endif /* __EXCEPTSYMBS_H */
