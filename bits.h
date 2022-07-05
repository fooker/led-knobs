#ifndef BITS_H_
#define BITS_H_

#define bit_set(reg, bit) ((reg) |=  (1 << (bit)))
#define bit_clr(reg, bit) ((reg) &= ~(1 << (bit)))
#define bit_tgl(reg, bit) ((reg) ^=  (1 << (bit)))

#define bit_get(reg, bit) ((reg) & (1 << (bit)))
#define bit_put(reg, bit, val) ((reg) = (reg) & ~(1 << (bit)) | ((val) << (bit)))

#endif