#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "aes.h"
#include "my.h"

#define PLAINTEXT_FILEPATH "plaintext.txt"
#define MASTERKEY_FILEPATH "masterKey.txt"
#define TRACE_FILEPATH "simulatedPowerConsumption.txt"
#define PLAIN_SIZE 16
#define KEY_SIZE 16
#define NUMBER_OF_PLAINTEXT 1000

static const u8 table_hamming_weight[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8 };

/**
 * @brief 중간값에 대한 파형을 계산하는 함수, Hamming Weight 모델을 따름.
 * 
 * @param trace 파형을 담을 버퍼
 * @param middle 중간값이 담긴 버퍼
 * @param middleDataSize 중간값의 크기
 */
void get_trace(u8* trace, const u8* middle, u8 size)
{
    // 평문 각 바이트의 Hamming Weight를 계산하여 파형 생성.
    for(size_t i = 0; i < size; i++)
        trace[i] = table_hamming_weight[middle[i]];
}

void generate_trace_about_aes128()
{
    // file open
    FILE* file_plain = fopen(PLAINTEXT_FILEPATH, "r");
    FILE* file_key   = fopen(MASTERKEY_FILEPATH, "r");
    FILE* file_trace = fopen(TRACE_FILEPATH, "w");

    // file open error
    if(file_plain == NULL || file_key == NULL || file_trace == NULL)
        abort();

    u8 key[KEY_SIZE]      = {0};
    u8 plain[PLAIN_SIZE]  = {0};
    u8 middle[PLAIN_SIZE] = {0};
    u8 trace[PLAIN_SIZE]  = {0}; // 전체 파형이 아닌 중간값 크기의 부분 파형. 합치면 전체 파형이 완성된다.

    // 키 16 바이트를 읽어옴. 평문을 읽어오는 read_data 함수를 사용해도 문제 X.
    read_data(key, file_key, KEY_SIZE);

    // plaintext.txt의 1000개의 평문 중 i번째 평문에 대해, 중간값의 부분 파형 계산 후 simulatedPowerConsumption.txt 쓰기.
    for(int plainTextIndex = 0; plainTextIndex < NUMBER_OF_PLAINTEXT; plainTextIndex++)
    {
        // 평문 16 바이트를 읽어옴
        read_data(middle, file_plain, PLAIN_SIZE);

        // AddRoundKey에 해당하는 중간값의 부분 파형 계산 후 파일에 쓰기
        AddRoundKey(middle, key);
        get_trace(trace, middle, PLAIN_SIZE);
        write_data(file_trace, trace, PLAIN_SIZE);

        // SubBytes에 해당하는 중간값의 부분 파형 계산 후 파일에 쓰기
        SubBytes_shuffle(middle);
        get_trace(trace, middle, PLAIN_SIZE);
        write_data(file_trace, trace, PLAIN_SIZE);

        // ShiftRows에 해당하는 중간값의 부분 파형 계산 후 파일에 쓰기
        ShiftRows(middle);
        get_trace(trace, middle, PLAIN_SIZE);
        write_data(file_trace, trace, PLAIN_SIZE);

        // MixColumns에 해당하는 중간값의 부분 파형 계산 후 파일에 쓰기
        MixColumns(middle);
        get_trace(trace, middle, PLAIN_SIZE);
        write_data(file_trace, trace, PLAIN_SIZE);
        
        // 줄 개행
        fputc('\n', file_trace);
    }

    // 파일 닫기
    fclose(file_plain);
    fclose(file_key);
    fclose(file_trace);
}

int main()
{   
    generate_trace_about_aes128();

    return 0;
}