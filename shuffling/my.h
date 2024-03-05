#ifndef MY_H_
#define MY_H_

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define u8 uint8_t

void read_data(u8* data, FILE* file, size_t size);
void write_data(FILE* file, const u8* data, size_t size);

#endif