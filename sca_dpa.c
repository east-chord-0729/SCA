#include "sca.h"
 
#define GET_MSB(a) ((a) >> 7) & 1

static float get_diff_with_binary_model(u8* middle_value_list, u32 point_idx, const TraceInfo* trace_info)
{
    u32 count[2] = {0};
    float sum_point[2] = {0.0};
    float average_point[2] = {0.0};

    /* 바이너리 모델로 포인트 합의 평균의 차 계산 */
    for(u32 trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
    {
        if (GET_MSB(middle_value_list[trace_idx])) {
            sum_point[1] += trace_info->trace[trace_idx][point_idx];
            count[1] += 1;
        }
        else {
            sum_point[0] += trace_info->trace[trace_idx][point_idx];
            count[0] += 1;
        }
    }

    average_point[0] = sum_point[0] / count[0];
    average_point[1] = sum_point[1] / count[1];

    return (float)fabs(average_point[1] - average_point[0]);
}

static float get_diff_max(u8 guess_key, int key_byte_idx, const TraceInfo* trace_info)
{
    u8* middle_value_list = (u8*)malloc(sizeof(u8) * trace_info->trace_num);
    float max_diff = 0.0;

    /* key_byte_idx 에 위치한 평문 바이트의 중간값 획득. */
    for(int trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
        middle_value_list[trace_idx] = table_sbox[trace_info->plain[trace_idx][key_byte_idx] ^ guess_key];

    /* 바이너리 모델을 통해 차분의 최댓값 구하기. */
    for (u32 point_idx = 0; point_idx < trace_info->point_num; point_idx++)
    {   
        float diff = get_diff_with_binary_model(middle_value_list, point_idx, trace_info);

        if(max_diff < diff)
            max_diff = diff;
    }

    free(middle_value_list);

    return max_diff;
}

static int find_key_byte(Result* result,int key_byte_idx, float* diff_list)
{
    float max_diff = 0.0;
    float second_diff = 0.0;
    u8 key = 0;

    /* 차분의 최댓값, 두번째 최댓값 구하기 */
    for(u32 guess_key = 0; guess_key < BYTE_KEY_BOUND; guess_key++) 
    {
        if(max_diff < diff_list[guess_key]) {
            second_diff = max_diff;
            max_diff = diff_list[guess_key];
            key = guess_key;
        } 
        else if(second_diff < diff_list[guess_key]) {
            second_diff = diff_list[guess_key];
        }
    }

    result->key[key_byte_idx] = key;
    result->diff_or_corr[key_byte_idx] = max_diff;
    result->ratio[key_byte_idx] = max_diff / second_diff;

    return 1;
}

void dpa(Result* result, const TraceInfo* trace_info)
{
    time_t t_start = time(NULL);

    /* 1 바이트씩 키를 찾음 */
    for(int key_byte_idx = 0; key_byte_idx < KEY_SIZE; key_byte_idx++) 
    {
        float diff_list[BYTE_KEY_BOUND] = {0.0};

        /* 추측한 키 마다 포인트 별 차분 계산 후 가장 큰 차분 얻기. 차분은 바이너리 모델을 따름. */
        for(u32 guess_key = 0; guess_key < BYTE_KEY_BOUND; guess_key++)
            diff_list[guess_key] = get_diff_max(guess_key, key_byte_idx, trace_info);

        /* 차분이 가장 높은 위치가 실제 키일 확률이 높음 */
        find_key_byte(result, key_byte_idx, diff_list);
    }

    time_t t_end = time(NULL);
    result->time = (double)(t_end - t_start);
}