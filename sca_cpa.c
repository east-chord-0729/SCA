#include "sca.h"


/* 해당함수들은 메모리 관리나 데이터 입력에 관한 부분이므로 주석 생략. 주석이 있는 함수에서 충분히 설명해주고 있음.
   list_hamming_get에 중간값의 해밍무게를 계산하는 부분이 있으므로 이부분만 참고. */
static float **list_point_create(const TraceInfo *trace_info);
static void    list_point_get(float** list_point, const TraceInfo* trace_info);
static void    list_point_destroy(float **list_point, const TraceInfo *trace_info);
static u8     *list_hamming_create(const TraceInfo *trace_info);
static void    list_hamming_get(u8* list_hamming, u32 key_byte_idx, u32 guess_key, const TraceInfo* trace_info);
static void    list_hamming_destroy(u8 *list_hamming);


/**
 * @brief 두 집합 x, y 상관계수를 반환
 * 
 * 공식은 다음 사이트에서 확인: https://ko.wikipedia.org/wiki/피어슨_상관_계수
 *
 * @param set_x 집합 x
 * @param set_y 집합 y
 * @param set_size 집합의 크기 n
 * @return float 상관계수
 */
static float compute_correlation_coefficient(const u8 *list_hw, const float *list_point, u32 list_size)
{
    float sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0, sum_yy = 0.0;

    /* 공식에서 시그마에 해당 하는 부분 계산 */
    for (u32 i = 0; i < list_size; i++)
    {
        sum_x += list_hw[i];                        // sum(x)
        sum_y += list_point[i];                     // sum(y)
        sum_xy += list_hw[i] * list_point[i];       // sum(xy)
        sum_xx += list_hw[i] * list_hw[i];          // sum(x^2)
        sum_yy += list_point[i] * list_point[i];    // sum(y^2)
    }

    /* 상관계수 계산 */
    float numerator = (list_size * sum_xy - sum_x * sum_y);                                          // 상관계수 공식의 분자 계산
    float denomirator = (list_size * sum_xx - sum_x * sum_x) * (list_size * sum_yy - sum_y * sum_y); // 상관계수 공식의 분모 계산 (제곱근 제외)
    float correlation_coefficient = (float)fabs(numerator / sqrt(denomirator));                      // 상관계수 계산

    return correlation_coefficient;
}

/**
 * @brief 추측한 키에 해당하는 중간값의 해밍무게 집합(X)과, 
 * 0번째 부터 포인트 개수(point_num) 번째 까지 각 위치에서의 포인트 위치(Y0, ..., Ypoint_num-1)간의 상관계수를 계산.
 * X, Y0, .., Ypoint_num-1 집합의 크기는 전부 파형의 개수와 같다(trace_num).
 * 그러면 총 point_num 개의 상관계수를 얻게 되는데, 이 중 최댓값을 반환.
 * 
 *                  . -->  추측한 키가 실제 키와 같을 때, 이 키가 실제로 sbox에서 연산하는 부분에서 상관계수가 높게 측정.
 *                         만약 실제 키가 아니라면, 모든 상관계수가 0에 가까울 것임.
 * 
 * ................. ..................... --> point_num 개의 상관계수를 계산하면, 이렇게 0에 가까운 값을 얻게됨.
 * |----------- sbox 연산 구간 ------------|
 *              (실제 분석 구간은 아니지만 설명을 위해)
 * 
 * 헷갈린 점(여기는 제가 공부하면서 헷갈린 부분을 작성했습니다.)
 *      포인트들의 집합(Y)이 한 파형에서 어느 구간의 포인트들의 모임이 아님.
 *      어느 한 포인트 위치를 잡고, 그 위치에 대하여 각 파형에서 똑같은 위치의 포인트들을 모은 집합임.
 *      즉 포인트들의 집합은 그 크기가 파형의 개수가 되고, 집합의 개수는 포인트의 개수와 같음. 
 * 
 * @param list_point 포인트 집합
 * @param list_hamming 해밍무게 집합
 * @param trace_info 파형 정보
 * @return float 계산한 상관계수 중 최댓값
 */
static float get_max_corr_of_guess_key(float **list_point, const u8* list_hamming, const TraceInfo *trace_info)
{
    float corr_max = 0.0; // 계산한 상관계수 중 최댓값

    /* 포인트 위치에 해당하는 상관계수 중 가장 큰 상관계수 구하기 */
    for (u32 point_idx = 0; point_idx < trace_info->point_num; point_idx++)
    {
        float current_corr = compute_correlation_coefficient(list_hamming, list_point[point_idx], trace_info->trace_num); // 중간값의 헤밍무게 집합(X)와 i번째 포인트에 해당하는 포인트 집합(Y_i)의 상관계수 계산.
        if (corr_max < current_corr) corr_max = current_corr; // 계산한 상관계수가 최댓값인지 확인
    }

    return corr_max;
}

/**
 * @brief 256개의 상관계수 값 중 크게 튀어있는 상관계수를 찾고, 해당하는 키와 비율 구하기
 * 
 * @param result 키, 상관계수, 비율의 결과
 * @param list_corr 추측한 키에 대한 상관계수 집합
 * @param key_byte_idx 찾으려는 키의 바이트 위치
 */
static void find_key_corr_and_ratio(Result *result, const float *list_corr, u32 key_byte_idx)
{
    u8 real_key = 0;            // 실제 키
    float corr_max = 0.0;       // 실제 키에 해당하는 상관계수 (상관계수의 최댓값)
    float corr_second = 0.0;    // 실제 키에 해당하지 않는 상관계수 중 최댓값 (상관계수의 두번째 최댓값), 실제 키에 해당하는 상관계수와의 비율을 계산하여 실제 키일 가능성을 확인하는데 사용.

    /* 위의 세 개의 값 계산 */
    for (u32 guess_key = 0; guess_key < BYTE_KEY_BOUND; guess_key++)
    {
        float corr_current = list_corr[guess_key]; // 추측한 키에 해당하는 상관계수 확인

        if (corr_max < corr_current)    // 확인한 상관계수가 지금까지 계산한 최댓값보다 크다면
        {
            corr_second = corr_max;     // 기존 최댓값은 두번째로 내려감.
            corr_max    = corr_current; // 확인한 상관계수는 최댓값이 됨.
            real_key    = guess_key;    // 최댓값에 해당하는 키는 실제 키임.
        }
        else if (corr_second < corr_current)
            corr_second = corr_current; // 확인한 상관계수가 최댓값은 아니지만 두번째 최댓값보다는 클 수 있으므로 확인.
    }

    /* 결과 입력 */
    result->key[key_byte_idx] = real_key;
    result->diff_or_corr[key_byte_idx] = corr_max; // diff 아님.
    result->ratio[key_byte_idx] = corr_max / corr_second;
}

/**
 * @brief 상관관계 전력 분석
 * 
 * @param result [out] 전력 분석 결과
 * @param trace_info [in] 파형 정보
 */
void cpa(Result *result, const TraceInfo *trace_info)
{   
    /* 시간 측정 시작 */
    time_t t_start = time(NULL);

    /* 상관관계를 구하는데 사용하는 입력 */
    float **list_point = list_point_create(trace_info); // 파형의 i번째 포인트들의 집합. (0 <= i < point_num)
    u8 *list_hamming = list_hamming_create(trace_info); // 평문 바이트에 매칭되는 중간 값의 해밍 무게 집합.

    list_point_get(list_point, trace_info); // 포인트 집합 구하기 : 파형 개수와 포인트 개수에 의존하므로 사전 계산 가능

    /* 0번째 바이트부터 15번째 바이트의 실제 키 찾기 */
    for (u32 key_byte_idx = 0; key_byte_idx < KEY_SIZE; key_byte_idx++)
    {
        float list_correlation[BYTE_KEY_BOUND]; // 최대 상관계수 집합, 256개의 값 중 한 값은 다른 값과 다르게 크게 튀어있음.

        /* 0x00부터 0xFF까지 키를 추측하여, 추측한 키에 해당하는 최대 상관계수 값 구하기 */
        for(u32 guess_key = 0; guess_key < BYTE_KEY_BOUND; guess_key++) 
        {
            list_hamming_get(list_hamming, key_byte_idx, guess_key, trace_info); // 중간 값의 해밍무게 집합 구하기.
            list_correlation[guess_key] = get_max_corr_of_guess_key(list_point, list_hamming, trace_info); // 헤밍무게 집합과 포인트 집합을 이용하여 0번째 ~ point_num번째의 상관계수 중 가장 큰 상관계수 구하기.
        }

        find_key_corr_and_ratio(result, list_correlation, key_byte_idx); // 위에서 계산한 상관계수 집합에서 크게 튀어있는 상관계수를 찾고, 이에 해당되는 키 값과 비율 구하기.
    }

    /* 메모리 해제 */
    list_point_destroy(list_point, trace_info);
    list_hamming_destroy(list_hamming);

    /* 시간 측정 종료 */
    time_t t_end = time(NULL);
    result->time = (double)(t_end - t_start);
}





static float **list_point_create(const TraceInfo *trace_info)
{
    float **list_point = (float **)malloc(sizeof(float *) * trace_info->point_num);

    for (u32 point_idx = 0; point_idx < trace_info->point_num; point_idx++)
        list_point[point_idx] = (float *)malloc(sizeof(float) * trace_info->trace_num);

    return list_point;
}

static void list_point_get(float** list_point, const TraceInfo* trace_info)
{
    for (u32 point_idx = 0; point_idx < trace_info->point_num; point_idx++)
    {
        for(u32 trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
            list_point[point_idx][trace_idx] = trace_info->trace[trace_idx][point_idx]; // trace_idx 와 point_idx 위치가 서로 다름에 주의. 설명은 get_max_corr_of_guess_key 함수 주석 참고.
    }
}

static void list_point_destroy(float **list_point, const TraceInfo *trace_info)
{
    for (int point_idx = 0; point_idx < trace_info->point_num; point_idx++)
        free(list_point[point_idx]);

    free(list_point);
}

static u8 *list_hamming_create(const TraceInfo *trace_info)
{
    u8* list_hamming = malloc(sizeof(u8) * trace_info->trace_num);

    return list_hamming;
}

static void list_hamming_get(u8* list_hamming, u32 key_byte_idx, u32 guess_key, const TraceInfo* trace_info)
{
    for(u32 trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
    {
        u8 plain_byte = trace_info->plain[trace_idx][key_byte_idx];   // 파형에 해당하는 평문
        u8 middle_value = table_sbox[plain_byte ^ guess_key];         // 평문의 중간값
        list_hamming[trace_idx] = table_hamming_weight[middle_value]; // 중간값의 해밍무게
    }
}

static void list_hamming_destroy(u8 *list_hamming)
{
    free(list_hamming);
}
