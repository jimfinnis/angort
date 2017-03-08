/**
 * @file
 * brief description, full stop.
 *
 * long description, many sentences.
 * 
 */

#ifndef __OPCODES_H
#define __OPCODES_H

#if DEFOPCODENAMES

namespace angort {

const char *opcodenames[]=
{ "?","end","litint","litfloat",
    "func","globaldo","mod","localget",
    "localset","decleaveneg","add","mul",
    "div","sub","if","jump",
    "leave","dup","litstring","call",
    "litcode","globget","globset","propget",
    "propset","not","equals","nequals",
    "swap","drop","and","or","gt","lt",
    "ifleave","SPARE","SPARE","newhash",
    "litsymb","over","closureget","closureset","dot",
    "iterlvifdone","iterstart","cmp","library",
    "newlist","appendlist", "SPARE","loopstart","stop",
    "hashgetsymb","hashsetsymb","import","def","recurse",
    "self","dummycase","le","ge","constexpr",
    "yield","try","endtry","throw","litdouble",
    "litlong"
    
};

}
#endif
    
#define OP_INVALID 0
#define OP_END 1
#define OP_LITERALINT 2
#define OP_LITERALFLOAT 3

#define OP_FUNC 4
#define OP_GLOBALDO 5
#define OP_MOD 6
#define OP_LOCALGET 7

#define OP_LOCALSET 8
#define OP_DECLEAVENEG 9
#define OP_ADD 10
#define OP_MUL 11

#define OP_DIV 12
#define OP_SUB 13
#define OP_IF	14
#define OP_JUMP 15

#define OP_LEAVE 16
#define OP_DUP 17
#define OP_LITERALSTRING 18
#define OP_CALL 19

#define OP_LITERALCODE 20
#define OP_GLOBALGET 21
#define OP_GLOBALSET 22
#define OP_PROPGET 23

#define OP_PROPSET 24
#define OP_NOT 25
#define OP_EQUALS 26
#define OP_NEQUALS 27

#define OP_SWAP 28
#define OP_DROP 29
#define OP_AND	30
#define OP_OR	31
#define OP_GT	32

#define OP_LT   33
#define OP_IFLEAVE  34
#define OP_spareFOR	35
#define OP_spareFORJUMP	36
#define OP_NEWHASH	37

#define OP_LITERALSYMB	38
#define OP_OVER	39
#define OP_CLOSUREGET 40
#define OP_CLOSURESET 41
#define OP_DOT 42

#define OP_ITERLEAVEIFDONE	43
#define OP_ITERSTART 44
#define OP_CMP 45
#define OP_LIBRARY 46

#define OP_NEWLIST 47
#define OP_APPENDLIST 48
#define OP_sparexxxxxxxx 49
#define OP_LOOPSTART 50
#define OP_STOP 51

#define OP_HASHGETSYMB 52
#define OP_HASHSETSYMB 53
#define OP_IMPORT 54
#define OP_DEF 55
#define OP_RECURSE 56

#define OP_SELF 57
#define OP_DUMMYCASE 58
#define OP_LE 59
#define OP_GE 60
#define OP_CONSTEXPR 61

#define OP_YIELD 62
#define OP_TRY 63
#define OP_ENDTRY 64
#define OP_THROW 65
#define OP_LITDOUBLE 66

#define OP_LITLONG 67
#endif /* __OPCODES_H */
