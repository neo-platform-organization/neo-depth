#include "machine.h"
#include <limits.h>
#include <string.h>
#if !defined(__x86_64__)
#error This CPU backend requires x86-64.
#endif

static bool neo_bytes(neo_machine_code *code, const unsigned char *bytes, size_t count) {
    if (!code || (!bytes && count) || (!code->bytes && code->capacity) ||
        code->length>code->capacity || count>code->capacity-code->length) { return false; }
    if (!count) { return true; }
    memcpy(code->bytes+code->length,bytes,count); code->length+=count; return true;
}
#define EMIT(...) neo_bytes(code,(const unsigned char[]){__VA_ARGS__},sizeof((const unsigned char[]){__VA_ARGS__}))
static bool neo_word(neo_machine_code *code, uint32_t value) {
    return EMIT((unsigned char)value,(unsigned char)(value>>8),(unsigned char)(value>>16),(unsigned char)(value>>24));
}
static bool neo_jump(neo_machine_code *code, unsigned char condition) {
    bool ok=condition ? EMIT(0x0f,condition,0,0,0,0) : EMIT(0xe9,0,0,0,0);
    if (ok) { code->branch=code->length-4; } return ok;
}
bool neo_machine_patch(neo_machine_code *code, size_t relocation, size_t target) {
    if (!code || !code->bytes || code->length>code->capacity ||
        relocation>code->length || code->length-relocation<4 || target>code->length) { return false; }
    size_t origin=relocation+4;
    if ((target>=origin && target-origin>INT32_MAX) ||
        (target<origin && origin-target>(size_t)INT32_MAX+1u)) { return false; }
    uint32_t distance=(uint32_t)target-(uint32_t)origin;
    for (unsigned byte=0; byte<4; ++byte) { code->bytes[relocation+byte]=(unsigned char)(distance>>(byte*8)); }
    return true;
}
static bool neo_fail(neo_machine_code *code, uint32_t failure, uint32_t stack) {
    return EMIT(0x48,0xb8) && neo_word(code,failure) && neo_word(code,0x80000000u) &&
        EMIT(0x48,0x81,0xc4) && neo_word(code,stack) && EMIT(0xc3);
}
static bool neo_copy(neo_machine_code *code, uint32_t source, uint32_t destination, uint32_t size, bool indexed) {
    if (size%16 || source>UINT32_MAX-size || destination>UINT32_MAX-size) { return false; }
    for (uint32_t byte=0; byte<size; byte+=16) {
        if (!EMIT(0xf3,0x41,0x0f,0x6f,0x83) || !neo_word(code,source+byte) ||
            !(indexed ? EMIT(0xf3,0x41,0x0f,0x7f,0x82) : EMIT(0xf3,0x41,0x0f,0x7f,0x83)) ||
            !neo_word(code,destination+byte)) { return false; }
    }
    return true;
}
static bool neo_table(neo_machine_code *code, neo_machine_operation operation, uint32_t count, uint32_t stack) {
    if (!(count<=127 ? EMIT(0x83,0xf8,(unsigned char)count) : EMIT(0x3d) && neo_word(code,count)) ||
        !neo_jump(code,0x83)) { return false; }
    size_t bounds=code->branch;
    if (!EMIT(0x48,0x8d,0x0d,0,0,0,0)) { return false; }
    code->address=code->length-4;
    bool ok=operation==NEO_MACHINE_TABLE_F64 ? EMIT(0xf2,0x0f,0x10,0x04,0xc1) :
        operation==NEO_MACHINE_TABLE_I32 ? EMIT(0x8b,0x04,0x81) : EMIT(0x0f,0xb6,0x04,0x01);
    if (!ok || !neo_jump(code,0)) { return false; }
    size_t valid=code->branch;
    if (!neo_machine_patch(code,bounds,code->length) ||
        !(operation==NEO_MACHINE_TABLE_F64 ? neo_fail(code,6,stack) : EMIT(0xb8,0xff,0xff,0xff,0xff)) ||
        !neo_jump(code,0)) { return false; }
    size_t invalid=code->branch;
    /* The valid path joins the invalid path's final jump. Only that jump and
       the table address need relocation when the compiler appends its data. */
    if (!neo_machine_patch(code,valid,invalid-1)) { return false; }
    code->branch=invalid;
    return true;
}

static bool neo_instruction(neo_machine_code *code, neo_machine_operation operation, const uint64_t *args, size_t count) {
    for (size_t i=0; i<count; ++i) {
        if (operation!=NEO_MACHINE_CONSTANT_F64 && args[i]>UINT32_MAX) { return false; }
    }
    uint32_t a=count>0 ? (uint32_t)args[0] : 0, b=count>1 ? (uint32_t)args[1] : 0;
    uint32_t c=count>2 ? (uint32_t)args[2] : 0, d=count>3 ? (uint32_t)args[3] : 0;
    switch (operation) {
    case NEO_MACHINE_ENTER: return count==1 && EMIT(0x49,0x89,0xcb,0x48,0x81,0xec) && neo_word(code,a);
    case NEO_MACHINE_RETURN: return count==1 && EMIT(0x48,0x98,0x48,0x81,0xc4) && neo_word(code,a) && EMIT(0xc3);
    case NEO_MACHINE_FAIL: return count==2 && neo_fail(code,a,b);
    case NEO_MACHINE_CONSTANT: return count==1 && EMIT(0xb8) && neo_word(code,a);
    case NEO_MACHINE_ZERO: return count==0 && EMIT(0x31,0xc0);
    case NEO_MACHINE_LOAD_LOCAL: return count==1 && EMIT(0x8b,0x84,0x24) && neo_word(code,a);
    case NEO_MACHINE_STORE_LOCAL: return count==1 && EMIT(0x89,0x84,0x24) && neo_word(code,a);
    case NEO_MACHINE_BINARY_LEFT: return count==1 && EMIT(0x89,0xc1,0x8b,0x84,0x24) && neo_word(code,a);
    case NEO_MACHINE_LOAD_WORD: return count==1 && EMIT(0x41,0x8b,0x83) && neo_word(code,a);
    case NEO_MACHINE_STORE_VALUE: return count==1 && EMIT(0x41,0x89,0x83) && neo_word(code,a);
    case NEO_MACHINE_STORE_WORD: return count==2 && EMIT(0x41,0xc7,0x83) && neo_word(code,a) && neo_word(code,b);
    case NEO_MACHINE_STORE_BYTE: return count==2 && b<=255 && EMIT(0x41,0xc6,0x83) && neo_word(code,a) && EMIT((unsigned char)b);
    case NEO_MACHINE_INCREMENT_WORD: return count==1 && EMIT(0x41,0xff,0x83) && neo_word(code,a);
    case NEO_MACHINE_COMPARE:
        return count==1 && a>=NEO_COMPARE_EQUAL && a<=NEO_COMPARE_GREATER &&
            EMIT(0x39,0xc8,0x0f,a==NEO_COMPARE_EQUAL ? 0x94 : a==NEO_COMPARE_LESS ? 0x9c : 0x9f,0xc0,0x0f,0xb6,0xc0);
    case NEO_MACHINE_ADD: return count==0 && EMIT(0x01,0xc8);
    case NEO_MACHINE_SUBTRACT: return count==0 && EMIT(0x29,0xc8);
    case NEO_MACHINE_MULTIPLY: return count==0 && EMIT(0x0f,0xaf,0xc1);
    case NEO_MACHINE_EQUAL_LOCAL: return count==1 && EMIT(0x3b,0x84,0x24) && neo_word(code,a) && EMIT(0x0f,0x94,0xc0,0x0f,0xb6,0xc0);
    case NEO_MACHINE_JUMP: return count==0 && neo_jump(code,0);
    case NEO_MACHINE_BRANCH_ZERO: return count==0 && EMIT(0x85,0xc0) && neo_jump(code,0x84);
    case NEO_MACHINE_BRANCH_OVERFLOW: return count==0 && neo_jump(code,0x80);
    case NEO_MACHINE_BRANCH_BELOW: return count==0 && neo_jump(code,0x82);
    case NEO_MACHINE_BRANCH_NOT_EQUAL: return count==0 && neo_jump(code,0x85);
    case NEO_MACHINE_COMPARE_WORD: return count==1 && EMIT(0x41,0x3b,0x83) && neo_word(code,a);
    case NEO_MACHINE_COMPARE_WORD_CONSTANT: return count==2 && EMIT(0x41,0x81,0xbb) && neo_word(code,a) && neo_word(code,b);
    case NEO_MACHINE_GUARD_BELOW:
    case NEO_MACHINE_GUARD_EQUAL: {
        if (count!=4 || b>127 || !EMIT(0x41,0x83,0xbb) || !neo_word(code,a) || !EMIT((unsigned char)b) ||
            !neo_jump(code,operation==NEO_MACHINE_GUARD_BELOW ? 0x82 : 0x84)) { return false; }
        size_t valid=code->branch;
        return neo_fail(code,c,d) && neo_machine_patch(code,valid,code->length);
    }
    case NEO_MACHINE_COPY: return count==3 && neo_copy(code,a,b,c,false);
    case NEO_MACHINE_COPY_INDEXED:
        return count==5 && EMIT(0x45,0x8b,0x93) && neo_word(code,a) && EMIT(0x45,0x69,0xd2) && neo_word(code,b) &&
            EMIT(0x4d,0x01,0xda) && neo_copy(code,c,d,(uint32_t)args[4],true);
    case NEO_MACHINE_INDEX_LOAD: return count==1 && EMIT(0x45,0x8b,0x83) && neo_word(code,a);
    case NEO_MACHINE_INDEX_LIMIT: return count==1 && EMIT(0x41,0x81,0xf8) && neo_word(code,a);
    case NEO_MACHINE_BRANCH_AT_LEAST: return count==0 && neo_jump(code,0x83);
    case NEO_MACHINE_INDEX_STORE_WORD: return count==2 && EMIT(0x43,0xc7,0x84,0xc3) && neo_word(code,a) && neo_word(code,b);
    case NEO_MACHINE_INDEX_STORE_VALUE: return count==1 && EMIT(0x43,0x89,0x84,0xc3) && neo_word(code,a);
    case NEO_MACHINE_INDEX_INCREMENT_STORE: return count==1 && EMIT(0x41,0xff,0xc0,0x45,0x89,0x83) && neo_word(code,a);
    case NEO_MACHINE_REQUEST_STORE:
        return count==2 && EMIT(0x41,0x8b,0x8b) && neo_word(code,a) && EMIT(0xc1,0xe1,0x05,0x41,0x89,0x84,0x0b) && neo_word(code,b);
    case NEO_MACHINE_LOAD_F64: return count==1 && EMIT(0xf2,0x41,0x0f,0x10,0x83) && neo_word(code,a);
    case NEO_MACHINE_STORE_F64: return count==1 && EMIT(0xf2,0x41,0x0f,0x11,0x83) && neo_word(code,a);
    case NEO_MACHINE_CONSTANT_F64:
        return count==1 && EMIT(0x48,0xb8) && neo_word(code,(uint32_t)args[0]) &&
            neo_word(code,(uint32_t)(args[0]>>32)) && EMIT(0x66,0x48,0x0f,0x6e,0xc0);
    case NEO_MACHINE_INT_TO_F64: return count==0 && EMIT(0xf2,0x0f,0x2a,0xc0);
    case NEO_MACHINE_F64_TO_INT: return count==0 && EMIT(0xf2,0x48,0x0f,0x2c,0xc0,0x48,0x63,0xc8,0x48,0x39,0xc8) && neo_jump(code,0x85);
    case NEO_MACHINE_STORE_F64_LOCAL: return count==1 && EMIT(0xf2,0x0f,0x11,0x84,0x24) && neo_word(code,a);
    case NEO_MACHINE_F64_LEFT: return count==1 && EMIT(0xf2,0x0f,0x10,0xc8,0xf2,0x0f,0x10,0x84,0x24) && neo_word(code,a);
    case NEO_MACHINE_F64_BINARY:
        return count==1 && a>=NEO_FLOAT_ADD && a<=NEO_FLOAT_DIVIDE &&
            EMIT(0xf2,0x0f,a==NEO_FLOAT_ADD ? 0x58 : a==NEO_FLOAT_SUBTRACT ? 0x5c : a==NEO_FLOAT_MULTIPLY ? 0x59 : 0x5e,0xc1);
    case NEO_MACHINE_INDEXED_F64: return count==1 && EMIT(0xf2,0x41,0x0f,0x10,0x84,0xc3) && neo_word(code,a);
    case NEO_MACHINE_SEQUENCE_AT:
        return count==2 && b<=1 && EMIT(0x41,0x3b,0x83) && neo_word(code,a+4u) && EMIT(0x73,0x0b) &&
            (b ? EMIT(0x90,0x41,0x8b,0x84,0x83) : EMIT(0x41,0x0f,0xb6,0x84,0x03)) && neo_word(code,a+8u) &&
            EMIT(0xeb,0x05,0xb8,0xff,0xff,0xff,0xff);
    case NEO_MACHINE_TABLE_BYTE:
    case NEO_MACHINE_TABLE_I32: return count==1 && neo_table(code,operation,a,0);
    case NEO_MACHINE_TABLE_F64: return count==2 && neo_table(code,operation,a,b);
    case NEO_MACHINE_LOAD_BYTE_INDEXED:
        return count==2 && EMIT(0x44,0x8b,0x84,0x24) && neo_word(code,b) &&
            EMIT(0x43,0x0f,0xb6,0x84,0x03) && neo_word(code,a);
    case NEO_MACHINE_STORE_BYTE_INDEXED:
        return count==2 && EMIT(0x44,0x8b,0x84,0x24) && neo_word(code,b) &&
            EMIT(0x43,0x88,0x84,0x03) && neo_word(code,a);
    /* The local holds an element index. Scale by four for an int32 value;
       language bounds are emitted by the surface compiler before this access. */
    case NEO_MACHINE_LOAD_WORD_INDEXED:
        return count==2 && EMIT(0x44,0x8b,0x84,0x24) && neo_word(code,b) &&
            EMIT(0x43,0x8b,0x84,0x83) && neo_word(code,a);
    case NEO_MACHINE_STORE_WORD_INDEXED:
        return count==2 && EMIT(0x44,0x8b,0x84,0x24) && neo_word(code,b) &&
            EMIT(0x43,0x89,0x84,0x83) && neo_word(code,a);
    case NEO_MACHINE_AND_CONSTANT: return count==1 && EMIT(0x25) && neo_word(code,a);
    case NEO_MACHINE_SHIFT_LEFT: return count==1 && a<32 && EMIT(0xc1,0xe0,(unsigned char)a);
    case NEO_MACHINE_BRANCH_NONZERO: return count==0 && EMIT(0x85,0xc0) && neo_jump(code,0x85);
    }
    return false;
}
bool neo_machine_emit(neo_machine_code *code, neo_machine_operation operation, const uint64_t *args, size_t count) {
    if (!code || count>5 || (!args && count) || code->length>code->capacity || (!code->bytes && code->capacity)) { return false; }
    neo_machine_code staged=*code;
    if (!neo_instruction(&staged,operation,args,count)) { return false; }
    *code=staged; return true;
}
bool neo_machine_data(neo_machine_code *code, const unsigned char *bytes, size_t count) { return neo_bytes(code,bytes,count); }
bool neo_machine_word(neo_machine_code *code, uint32_t value) { return neo_word(code,value); }
