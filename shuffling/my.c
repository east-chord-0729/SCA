#include "my.h"

void read_data(u8* data, FILE* file, size_t size)
{
    /* file is null */
    if(file == NULL) 
        abort();

    /* read byte data */
    for(size_t i = 0; i < size; i++)
        fscanf(file, "%02hhx", &(data[i]));
}

void write_data(FILE* file, const u8* data, size_t size)
{
    /* file is null */
    if(file == NULL) 
        abort();

    /* write byte data */
    for(size_t i = 0; i < size; i++)
        fprintf(file, "%01hhx ", data[i]);
}