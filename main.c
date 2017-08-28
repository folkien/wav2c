#include <stdio.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

typedef struct {
    uint32_t ChunkID;
    uint32_t ChunkSize;
    uint32_t Format;
    /// fmt
    uint32_t Subchunk1ID;
    uint32_t Subchunk1Size;
    uint16_t AudioFormat;
    uint16_t NumChannels;
    uint32_t SampleRate;
    uint32_t ByteRate;
    uint16_t BlockAlign;
    uint16_t BitsPerSample;
    /// data
    uint32_t Subchunk2ID;
    uint32_t Subchunk2Size;
} __attribute__((packed)) sWavHeader;


void printfHeader(sWavHeader * header)
{
    printf("WAV HEADER.\n");
    printf("--------------------------------------.\n");
    printf("ChunkID : %d.\n",header->ChunkID);
    printf("ChunkSize : %d.\n",header->ChunkSize);
    printf("Format : %d.\n",header->Format);
    /// fmt
    printf("Subchunk1ID : %d.\n",header->Subchunk1ID);
    printf("Subchunk1Size : %d.\n",header->Subchunk1Size);
    printf("AudioFormat : %d.\n",header->AudioFormat);
    printf("NumChannels : %d.\n",header->NumChannels);
    printf("SampleRate : %d.\n",header->SampleRate);
    printf("ByteRate : %d.\n",header->ByteRate);
    printf("BlockAlign : %d.\n",header->BlockAlign);
    printf("BitsPerSample : %d.\n",header->BitsPerSample);
    /// data
    printf("Subchunk2ID : %d.\n",header->Subchunk2ID);
    printf("Subchunk2Size : %d.\n",header->Subchunk2Size);
    printf("--------------------------------------.\n");
}

int main(int argc, char * argv[])
{
    sWavHeader wavHeader;
    /// check arguments
    if (argc<2)
    {
        printf("No arguments!.\n");
        return -1;
    }

    /// read header
    printf("Opening file %s.\n", argv[1]);
    int file = open(argv[1],O_RDONLY);
    read(file, &wavHeader, sizeof(sWavHeader));
    printfHeader(&wavHeader);
    close(file);

    return 0;
}
