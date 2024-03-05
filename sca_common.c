#include "sca.h"

TraceInfo* trace_info_create()
{
    TraceInfo* trace_info = (TraceInfo*)malloc(sizeof(TraceInfo));

    trace_info->file_description = (char*)malloc(FILE_DESCRIPTION_SIZE + 1);
    trace_info->end_string = (char*)malloc(strlen(END_STRING) + 1);

    return trace_info;
}

void trace_info_destroy(TraceInfo* trace_info)
{
    if(strcmp(trace_info->end_string, END_STRING) == 0) 
    {
        for(u32 i = 0; i < trace_info->trace_num; i++) {
            free(trace_info->trace[i]);
            free(trace_info->plain[i]);
        }

        free(trace_info->trace);
        free(trace_info->plain);
    }

    free(trace_info->file_description);
    free(trace_info->end_string);
    free(trace_info);
}

static void load_header_info(TraceInfo* trace_info, FILE* trace_file)
{
    /* read header information */
    fread(trace_info->file_description, sizeof(char), FILE_DESCRIPTION_SIZE, trace_file);
    trace_info->file_description[FILE_DESCRIPTION_SIZE] = '\0';
    fread(&(trace_info->trace_num), sizeof(u32), 1, trace_file);
    fread(&(trace_info->point_num), sizeof(u32), 1, trace_file);
    fread(trace_info->end_string, sizeof(char), strlen(END_STRING), trace_file);
    trace_info->end_string[strlen(END_STRING)] = '\0';

    /* check end of header */
    if(strcmp(trace_info->end_string, END_STRING) != 0) {
        perror("failed to load trace information.");
        abort();
    }
}

void trace_info_load(TraceInfo* trace_info, FILE* trace_file, FILE* plain_file)
{
    /* read header information */
    load_header_info(trace_info, trace_file);

    trace_info->trace = (float**)malloc(sizeof(float*) * trace_info->trace_num);
    trace_info->plain = (u8**)malloc(sizeof(u8*) * trace_info->trace_num);

    /* read trace and plain data */
    for(u32 trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
    {
        trace_info->trace[trace_idx] = (float*)malloc(sizeof(float) * trace_info->point_num);
        trace_info->plain[trace_idx] = (u8*)malloc(sizeof(u8) * PLAIN_SIZE);

        fread(trace_info->trace[trace_idx], sizeof(float), trace_info->point_num, trace_file);
        
        for(u32 byte_idx = 0; byte_idx < PLAIN_SIZE; byte_idx++)
            fscanf(plain_file, "%02hhx", &(trace_info->plain[trace_idx][byte_idx]));
    }
}

void trace_info_load_advanced(TraceInfo* trace_info, 
                                   FILE* trace_file, 
                                   FILE* plain_file, 
                                     u32 start_point, 
                                     u32 end_point)
{
    /* read header information */
    load_header_info(trace_info, trace_file);

    trace_info->trace = (float**)malloc(sizeof(float*) * trace_info->trace_num);
    trace_info->plain = (u8**)malloc(sizeof(u8*) * trace_info->trace_num);

    /* 분석학 부분 파형의 포인트 수 계산 */
    /* |---(surp_s)--- start_point ---(point_num)--- end_point ---(surp_e)---| */
    u32 surp_s = start_point;
    u32 surp_e = trace_info->point_num - end_point;
    trace_info->point_num =  end_point - start_point;

    /* read trace and plain data */
    for(u32 trace_idx = 0; trace_idx < trace_info->trace_num; trace_idx++)
    {
        trace_info->trace[trace_idx] = (float*)malloc(sizeof(float) * trace_info->point_num);
        trace_info->plain[trace_idx] = (u8*)malloc(sizeof(u8) * PLAIN_SIZE);

        fseek(trace_file, surp_s * 4, SEEK_CUR);
        fread(trace_info->trace[trace_idx], sizeof(float), trace_info->point_num, trace_file);
        fseek(trace_file, surp_e * 4, SEEK_CUR);
        
        for(u32 byte_idx = 0; byte_idx < PLAIN_SIZE; byte_idx++)
            fscanf(plain_file, "%02hhx", &(trace_info->plain[trace_idx][byte_idx]));
    }
}

void trace_info_print(const TraceInfo* trace_info)
{
    printf("File Description: %s\n", trace_info->file_description);
    printf("    Trace Number: %d\n", trace_info->trace_num);
    printf("    Point Number: %d\n\n", trace_info->point_num);
}

Result* result_create()
{
    Result* result = (Result*)malloc(sizeof(Result));

    result->key = (u8*)malloc(sizeof(char) * KEY_SIZE);
    result->diff_or_corr = (float*)malloc(sizeof(float) * KEY_SIZE);
    result->ratio = (float*)malloc(sizeof(float*) * KEY_SIZE);

    return result;
}

void result_destroy(Result* result)
{
    free(result->key); 
    free(result->diff_or_corr);
    free(result->ratio);
    free(result);
}

void result_save(const Result* result, FILE* result_file)
{
    fprintf(result_file, "%s", "Byte_Index\tKey\tDifference\tRatio \n");

    for(int byte_idx = 0; byte_idx < KEY_SIZE; byte_idx++) {
        fprintf(result_file, "[%02d byte]\t%02hhx\t%.6f\t%.6f \n", 
            byte_idx, 
            result->key[byte_idx], 
            result->diff_or_corr[byte_idx], 
            result->ratio[byte_idx]);
    }

    fprintf(result_file, "\n time: %lf", result->time);
    fprintf(result_file, "\n key flag: ");

    for(int key_byte_idx = 0; key_byte_idx < KEY_SIZE; key_byte_idx++)
        fprintf(result_file, "%c", (char)result->key[key_byte_idx]);
    
    fprintf(result_file, "\n\n");
}   

void result_print(const Result* result)
{
    printf("Byte_Index\tKey\tDiff(or Corr)\tRatio \n");

    for(int key_byte_idx = 0; key_byte_idx < KEY_SIZE; key_byte_idx++) {
        printf("[%02d byte]\t%02hhx\t%.6f\t%.6f \n", 
            key_byte_idx, 
            result->key[key_byte_idx], 
            result->diff_or_corr[key_byte_idx], 
            result->ratio[key_byte_idx]);
    }

    printf("\ntime: %lf", result->time);
    printf("\nkey flag: ");

    for(int key_byte_idx = 0; key_byte_idx < KEY_SIZE; key_byte_idx++)
        printf("%c", (char)result->key[key_byte_idx]);

    printf("\n\n");
}