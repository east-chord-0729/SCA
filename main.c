#include "sca.h"

int main()
{
    /* 파형 분석에 필요한 파일 읽기/쓰기 */
    FILE* file_trace = fopen("data_cpa/powerConsumption.trace", "rb");
    FILE* file_plain = fopen("data_cpa/plaintext.txt", "r");
    FILE* file_cpa_result = fopen("data_cpa/CpaResult.txt", "w");

    /* 에러 체크 */
    if(file_trace == NULL || file_plain == NULL || file_cpa_result == NULL) {
        perror("file open error");
        abort();
    }

    /******************** CPA 시작! ********************/

    /* 메모리 할당 */
    TraceInfo* trace_info = trace_info_create();
    Result* result = result_create();

    /*  [파형 구간 수집 전략]
        sbox 연산구간 1600 ~ 2600 을 수집해도 되지만, 주어진 암호화 코드에서 102번 103을 보면,
        sbox 연산 결과를 시프트한 값들을 그대로 다시 입력하는 것을 볼 수 있다, 즉, sbox 연산과 같은 해밍무게를 가진다.
        이 코드에 해당하는 구간을 지정해서 cpa를 하면, 똑같은 결과를 얻을 것을 기대할 수 있다. */

    trace_info_load_advanced(trace_info, file_trace, file_plain, 2950, 3400); // 파형 구간 지정하여 수집
    trace_info_print(trace_info); // 파형 정보 출력

    cpa(result, trace_info); // CPA 시작
    result_print(result); // CPA 결과 출력

    /* 메모리 해제 */
    trace_info_destroy(trace_info);
    result_destroy(result);

    /******************** CPA 종료! ********************/

    /* 파일 닫기 */
    fclose(file_trace);
    fclose(file_cpa_result);

    return 0;
}