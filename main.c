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

uint32_t numOfSamples = 0;
uint16_t * leftChannel = NULL;
uint16_t * rightChannel = NULL;


void swapEndiannes(uint32_t * value)
{
  uint32_t tmp = *value;
  *value =   ( tmp >> 24) | (tmp << 24) |
             ((tmp >> 8) & 0x0000FF00) | ((tmp<<8) & 0x00FF0000);
}

void printfU32String(uint32_t array)
{
    char text[5];
    text[0] = (array >> 24) & 0xFF;
    text[1] = (array >> 16) & 0xFF;
    text[2] = (array >> 8) & 0xFF;
    text[3] = array & 0xFF;
    text[4] = 0;
    printf(" %s.\n",text);
}

void printfHeader(sWavHeader * header)
{
  printf("WAV HEADER.\n");
  printf("--------------------------------------.\n");
  printf("ChunkID : ");
  swapEndiannes(&header->ChunkID);
  printfU32String(header->ChunkID);
  printf("ChunkSize : %d.\n", header->ChunkSize);
  printf("Format : ");
  swapEndiannes(&header->Format);
  printfU32String(header->Format);
  /// fmt
  printf("Subchunk1ID : ");
  swapEndiannes(&header->Subchunk1ID);
  printfU32String(header->Subchunk1ID);
  printf("Subchunk1Size : %d.\n", header->Subchunk1Size);
  if(header->AudioFormat == 1)
  {
    printf("AudioFormat : PCM.\n");
  }
  else
  {
    printf("AudioFormat : Compression.\n");
  }
  printf("NumChannels : %d.\n", header->NumChannels);
  printf("SampleRate : %d.\n", header->SampleRate);
  printf("ByteRate : %d.\n", header->ByteRate);
  printf("BlockAlign : %d.\n", header->BlockAlign);
  printf("BitsPerSample : %d.\n", header->BitsPerSample);
  /// data
  printf("Subchunk2ID : ");
  swapEndiannes(&header->Subchunk2ID);
  printfU32String(header->Subchunk2ID);
  printf("Subchunk2Size : %d.\n", header->Subchunk2Size);
  printf("--------------------------------------.\n");
}

void fprintfChannel(FILE *pFile , uint16_t * array, uint32_t size)
{
  for (uint32_t i = 0; i<size; ++i)
  {
    fprintf(pFile, "0x%04x,\n",array[i]);
  }
}

void fprintfChannelMatlab(FILE *pFile , uint16_t * array, uint32_t size)
{
  for (uint32_t i = 0; i<size; ++i)
  {
    fprintf(pFile, "0x%04x ",array[i]);
  }
}

void readWavData(int fileId, sWavHeader * header)
{
  numOfSamples = header->Subchunk2Size / (header->NumChannels * header->BitsPerSample/8);

  leftChannel = malloc(sizeof(uint16_t) * numOfSamples);
  if (header->NumChannels > 1)
  {
    rightChannel = malloc(sizeof(uint16_t) * numOfSamples);
  }

  /// read all music data
  for (uint32_t i = 0; i<numOfSamples; ++i)
  {
    read(fileId, &leftChannel[i], sizeof(uint16_t));
    if(header->NumChannels > 1)
    {
      read(fileId, &rightChannel[i], sizeof(uint16_t));
    }
  }
}

void writeAsCFile(uint16_t *leftChannel, uint16_t *rightChannel,  sWavHeader * header)
{
  FILE * pFile;
  pFile = fopen("sound.c","w");

  /// header
  fprintf(pFile, "#include <stdint.h>\n");

  /// printf left Channel
  fprintf(pFile, "uint16_t leftChannel[] = {\n");
  fprintfChannel(pFile, leftChannel, numOfSamples);
  fprintf(pFile, "};\n");

  if(header->NumChannels > 1)
  {
    /// printf right Channel
    fprintf(pFile, "uint16_t rightChannel[] = {\n");
    fprintfChannel(pFile, rightChannel, numOfSamples);
    fprintf(pFile, "};\n");
  }
  fclose(pFile);
}

void writeAsMatlabFile(uint16_t *leftChannel, uint16_t *rightChannel,  sWavHeader * header)
{
  FILE * pFile;
  pFile = fopen("sound.m","w");

  /// printf left Channel
  fprintf(pFile, "leftChannel = [");
  fprintfChannelMatlab(pFile, leftChannel, numOfSamples);
  fprintf(pFile, "];");

  if(header->NumChannels > 1)
  {
    /// printf right Channel
    fprintf(pFile, "rightChannel = [");
    fprintfChannelMatlab(pFile, rightChannel, numOfSamples);
    fprintf(pFile, "];");
    fprintf(pFile, "plot(rightChannel);\n");
  }
  fprintf(pFile, "plot(leftChannel);\n");
  fprintf(pFile, "input(\"Wait for keypress\");\n");
  fclose(pFile);

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
    readWavData(file, &wavHeader);
    close(file);

    writeAsCFile(leftChannel, rightChannel, &wavHeader);
    writeAsMatlabFile(leftChannel, rightChannel, &wavHeader);

    return 0;
}
