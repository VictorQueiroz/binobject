#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdint.h>

#ifndef IEE754_H_
#define IEE754_H_

int write_ieee754_float(float x, uint8_t *buffer, int bigendian);
int write_ieee754(double x, uint8_t *buffer, int bigendian);
double read_ieee754(uint8_t *buffer, int bigendian);
float read_ieee754_float(uint8_t *input_buffer, int bigendian);

#endif

#ifdef __cplusplus
}
#endif