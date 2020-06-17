#ifndef DCTQUANT_H
#define DCTQUANT_H

#include <stdint.h>
#include <math.h>

//The default quantization matrix:
extern int default_quant_matrix[64];

//The ZigZag indices:
extern int zig_zag_index_matrix[64];

//The default quantization factor:
#define DEFAULT_QUANT_FACTOR 50

//Minimum and maximum:
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

//Calculate the alpha parameters for the (I)DCT:
float alpha(int pq);

#endif
