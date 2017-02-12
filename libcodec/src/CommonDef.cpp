
#include "CommonDef.h"
#include <stdio.h>
#include <iostream>
using namespace std;

void       WriteUnsigned(int  v, int n_of_bytes, BYTE *&stream)
{
    while(n_of_bytes--)
            *stream++ = (v>>(n_of_bytes*8))&0xff;
}
int        ReadUnsigned(int n_of_bytes, BYTE *&stream)
{
    int v = 0;
    while(n_of_bytes--)
    {
        v |= *stream++;
        v <<= (n_of_bytes)?8:0;
    }
    return v;
}
void       WriteInt(int  v, int n_of_bytes, BYTE *&stream)
{
    v = (v >= 0)?v*2:v*(-2)-1; //Convert to unsigned integer
    while(n_of_bytes--)
            *stream++ = (v>>(n_of_bytes*8))&0xff;
}
int 	   ReadInt(int n_of_bytes, BYTE *&stream)
{
    int v = 0;
    while(n_of_bytes--)
    {
        v |= *stream++;
        v <<= (n_of_bytes)?8:0;
    }
    return (v % 2)?v/(-2)-1:v/2;
}
void       ForwardDPCM(short *x, int len)
{
    short  pl1 = *x++;
    short  p   = *x;
    while(--len)
    {
        pl1 = p;
        p = *x;
        *x++ -= pl1;
    }
}
void       InverseDPCM(short *x, int vLen)
{
    ++x;
    while(--vLen)
        *x++ += *(x-1);
}
double     calculate_aml(short **seq, int seq_len)
{
    register short **end = seq + seq_len - 1,
    *s1, *s2;
    register unsigned count = 0;

    while (seq < end)
    {
        s1 = seq[0]; s2 = seq[1];
        while(*s1++ == *s2++)
            ++count;
        ++seq;
    }
    return (double)count/(seq_len - 1);
}
double     lpc0(short *x, unsigned xLen)
{
    __int64 r0_int = x[0]*x[0];
    __int64 r1_int = 0; 

    for(register unsigned i = 1; i < xLen; i++)
    {
        r0_int += x[i]*x[i];
        r1_int += x[i]*x[i-1];
    }
    return ((double)r1_int)/((double)r0_int);
}
int        get_file_size(const char *filename)
{
    FILE *fptr = nullptr;
    int filesize = 0;
    if (fptr = fopen(filename, "rb"))
    {
        fseek(fptr, 0, SEEK_END);
        filesize = ftell(fptr);
        fseek(fptr, 0, SEEK_SET);
        fclose(fptr);
        return filesize;
    }
    else
        return -1;
}
bool       compare_files(const char *f1, const char *f2)
{
    FILE *fptr1, *fptr2; unsigned file_len, file_len2; 
    file_len  = get_file_size(f1);
    file_len2 = get_file_size(f2);
    
    //Swap lengths to get smaller size of file length
    if(file_len > file_len2)
    {
        unsigned t = file_len2;
        file_len2  = file_len;
        file_len   = t;
    }

    if(file_len != file_len2)
        cout<<"      The length of files are differents. Comparing content..."<<endl;

    fptr1 = fopen(f1,"rb");  fptr2 = fopen(f2,"rb");
    int size_to_read = 0, already_read = 0, buffer_len = 1024;
    BYTE b1[1024], b2[1024];

    while(already_read < file_len)
    {	
        size_to_read = (file_len - already_read > buffer_len) ?
                        buffer_len : file_len - already_read;
        fread(b1,1,size_to_read,fptr1);
        fread(b2,1,size_to_read,fptr2);
        if(memcmp(b1, b2, size_to_read))
            break;
        already_read += size_to_read;
    }
    fclose(fptr1); fclose(fptr2);
    if(already_read < file_len)
    {
        int i=0;
        while(b1[i] == b2[i++])
            already_read++;
        cout<<"      The files are different at position "<<already_read<<"."<<endl;
        return false;
    }
    else
    {
        cout<<"      The files are equals!!!"<<endl;
        return true;
    }
}