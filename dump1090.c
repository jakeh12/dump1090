#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define PEAK_THRESHOLD 40000
#define PREAMBLE_LENGTH 8
#define DATA_LENGTH 112

static uint16_t magnitude_lut[256][256];

void make_magnitude_table()
{
    int i, q;
    double d_i, d_q;
    double max = sqrt(2.0);
    
    for (i = 0; i < 256; i++)
    {
        for (q = 0; q < 256; q++)
        {
            d_i = (i - 127.5) / 127.5;
            d_q = (q - 127.5) / 127.5;
            magnitude_lut[i][q] = (uint16_t)(round(65535.0*(sqrt(d_i*d_i+d_q*d_q)/max)));
        }
    }
}

uint16_t lookup_magnitude(uint8_t i, uint8_t q)
{
    return magnitude_lut[i][q];
}

void process_file(const char* path)
{
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "fopen %s failed: %d %s\n", path, errno, strerror(errno));
        exit(1);
    }
    fseek(file, 0, SEEK_END);
    int length = (int)(ftell(file));
    fseek(file, 0, SEEK_SET);
    uint8_t* data = calloc(length, sizeof(uint8_t));
    fread(data, 1, length, file);
    fclose(file);
    
    int total = 0;
    uint16_t m[length/2];
    int i;
    
    for (i = 0; i < length; i+=2)
    {
        m[i/2] = lookup_magnitude(data[i], data[i+1]);
    }
    
    uint16_t peak_avg;
    for (i = 0; i < length/2-16; i++)
    {
        // look for preamble pulse pattern
        if (!(m[i+0] > m[i+1] &&
              m[i+1] < m[i+2] &&
              m[i+2] > m[i+3] &&
              m[i+3] < m[i+0] &&
              m[i+4] < m[i+0] &&
              m[i+5] < m[i+0] &&
              m[i+6] < m[i+0] &&
              m[i+7] > m[i+8] &&
              m[i+8] < m[i+9] &&
              m[i+9] > m[i+6]))
        {
            continue;
        }
        
        // calculate average amplitude of the peaks
        peak_avg = (uint16_t)((m[i] + m[i+2] + m[i+7] + m[i+9])/4);
        
        // check if peak average is higher than threshold
        if (!(peak_avg >= PEAK_THRESHOLD))
        {
            continue;
        }
        
        // check if low areas are less than peak average
        if (!(m[i+4 ] < peak_avg &&
              m[i+5 ] < peak_avg &&
              m[i+11] < peak_avg &&
              m[i+12] < peak_avg &&
              m[i+13] < peak_avg &&
              m[i+14] < peak_avg))
        {
             continue;
        }
        
        uint16_t first, second;
        int errors = 0;
        int j;
        int bit = 0;
        uint8_t bytes[14] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0};
        
        for (j = 0; j < DATA_LENGTH*2; j+=2)
        {
            first = m[i+j+PREAMBLE_LENGTH*2];
            second = m[i+j+PREAMBLE_LENGTH*2+1];
            
            if (first < second)
            {
                bit = 0;
            }
            else if (first > second)
            {
                bit = 1;
            }
            else
            {
                errors++;
                // use previous bit if the amplitudes are equal
            }
            
            // place bit into the byte array
            bytes[j/2/8] = bytes[j/2/8] | (bit << (8 - (j/2 % 8) - 1));
            
        }
        
        // print out the decoded message
        int c;
        for (c = 0; c < 14; c++)
        {
            printf("%02x", bytes[c]);
        }
        total++;
        printf(" peak_avg: %d errors: %d", peak_avg, errors);
        printf("\n");
        
    }
    
    printf("total: %d\n", total);
    free(data);
}

int main()
{
    make_magnitude_table();
    process_file("test.bin");
    return 0;
}
