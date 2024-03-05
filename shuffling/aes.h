#ifndef AES_H_
#define AES_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void AddRoundKey(unsigned char input[16], const unsigned char key[16]);
void SubBytes(unsigned char input[16]);
void SubBytes_shuffle(unsigned char input[16]);
void ShiftRows(unsigned char input[16]);
void MixColumns(unsigned char input[16]);
void KeyExpansion(unsigned char roundKey[11][16], const unsigned char masterKey[16]);
void Encrypt(unsigned char* PT, unsigned char* KEY, unsigned char* out);

#endif