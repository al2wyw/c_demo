//
// Created by 李扬 on 2025/6/5.
//
#include <stdlib.h>

const int LogBytesPerShort   = 1;
const int LogBytesPerInt     = 2;
#ifdef _LP64
const int LogBytesPerWord    = 3;
#else
const int LogBytesPerWord    = 2;
#endif
const int LogBytesPerLong    = 3;

const int BytesPerShort      = 1 << LogBytesPerShort;
const int BytesPerInt        = 1 << LogBytesPerInt;
const int BytesPerWord       = 1 << LogBytesPerWord;
const int BytesPerLong       = 1 << LogBytesPerLong;

const int LogBitsPerByte     = 3;
const int LogBitsPerShort    = LogBitsPerByte + LogBytesPerShort;
const int LogBitsPerInt      = LogBitsPerByte + LogBytesPerInt;
const int LogBitsPerWord     = LogBitsPerByte + LogBytesPerWord;
const int LogBitsPerLong     = LogBitsPerByte + LogBytesPerLong;

const int BitsPerByte        = 1 << LogBitsPerByte;
const int BitsPerShort       = 1 << LogBitsPerShort;
const int BitsPerInt         = 1 << LogBitsPerInt;
const int BitsPerWord        = 1 << LogBitsPerWord;
const int BitsPerLong        = 1 << LogBitsPerLong;

const intptr_t OneBit     =  1;

#define nth_bit(n)        (n >= BitsPerWord ? 0 : OneBit << (n))
#define right_n_bits(n)   (nth_bit(n) - 1)
#define left_n_bits(n)    (right_n_bits(n) << (n >= BitsPerWord ? 0 : (BitsPerWord - n)))

enum {
    // high order bits are the TosState corresponding to field type or method return type
    tos_state_bits             = 4,
    tos_state_mask             = right_n_bits(tos_state_bits),
    tos_state_shift            = BitsPerInt - tos_state_bits,  // see verify_tos_state_shift below
    // misc. option bits; can be any bit position in [16..27]
    is_field_entry_shift       = 26,  // (F) is it a field or a method?
    has_method_type_shift      = 25,  // (M) does the call site have a MethodType?
    has_appendix_shift         = 24,  // (A) does the call site have an appendix argument?
    is_forced_virtual_shift    = 23,  // (I) is the interface reference forced to virtual mode?
    is_final_shift             = 22,  // (f) is the field or method final?
    is_volatile_shift          = 21,  // (v) is the field volatile?
    is_vfinal_shift            = 20,  // (vf) did the call resolve to a final method?
    // low order bits give field index (for FieldInfo) or method parameter size:
    field_index_bits           = 16,
    field_index_mask           = right_n_bits(field_index_bits),
    parameter_size_bits        = 8,  // subset of field_index_mask, range is 0..255
    parameter_size_mask        = right_n_bits(parameter_size_bits),
    option_bits_mask           = ~(((-1) << tos_state_shift) | (field_index_mask | parameter_size_mask))
  };

// specific bit definitions for the indices field:
enum {
    cp_index_bits              = 2*BitsPerByte,
    cp_index_mask              = right_n_bits(cp_index_bits),
    bytecode_1_shift           = cp_index_bits,
    bytecode_1_mask            = right_n_bits(BitsPerByte), // == (u1)0xFF
    bytecode_2_shift           = cp_index_bits + BitsPerByte,
    bytecode_2_mask            = right_n_bits(BitsPerByte)  // == (u1)0xFF
  };

typedef uint8_t  jubyte;
typedef uint16_t jushort;
typedef uint32_t juint;
typedef uint64_t julong;

typedef intptr_t  intx;
typedef uintptr_t uintx;

#define offset_of(klass,field) (size_t)((intx)&(((klass*)16)->field) - 16)

const int size_of_intx = sizeof(intx);
const int size_of_ptr = sizeof(char*);

typedef juint narrowOop; // Offset instead of address for an oop within a java object

// If compressed klass pointers then use narrowKlass.
typedef juint  narrowKlass;