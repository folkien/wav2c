#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
int32_t lowLimit;
int32_t highLimit;
int16_t * leftChannel = NULL;
int16_t * rightChannel = NULL;

uint16_t DAC_max = 4096;
uint16_t DAC_shift = 2048;

_Bool convertToMono = 0;

/// DAC resolution set for C format
uint16_t ConvertToDACValue(int16_t value)
{
  uint32_t tmp = ((value*DAC_max)/highLimit) + DAC_shift;
  return tmp;
}

void swapEndiannes(uint32_t * value)
{
  uint32_t tmp = *value;
  *value =   ( tmp >> 24) | (tmp << 24) |
             ((tmp >> 8) & 0x0000FF00) | ((tmp<<8) & 0x00FF0000);
}

void swapEndiannesS16(int16_t * value)
{
  uint32_t tmp = *value;
  *value =   ( tmp >> 8) | (tmp << 8);
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

void fprintfChannel(FILE * pFile, int16_t * array, uint32_t size)
{
  for (uint32_t i = 0; i<size; ++i)
  {
    fprintf(pFile, "%d, // %d\n",ConvertToDACValue(array[i]), array[i]);
  }
}
void fprintfChannelCombine(FILE * pFile, int16_t * array_left, int16_t * array_right, uint32_t size)
{
  for (uint32_t i = 0; i<size; ++i)
  {
    int32_t combined = (array_left[i] + array_right[i]) / 2;
    fprintf(pFile, "%d, // %d %d\n",ConvertToDACValue(combined), array_left[i], array_right[i]);
  }
}

void fprintfChannelMatlab(FILE * pFile , int16_t * array, uint32_t size)
{
  for (uint32_t i = 0; i<size; ++i)
  {
    fprintf(pFile, "%d ",array[i]);
  }
}

void readWavData(int fileId, sWavHeader * header)
{
  numOfSamples = header->Subchunk2Size / (header->NumChannels * header->BitsPerSample/8);
  switch (header->BitsPerSample) {
    case 8:
      lowLimit = -128;
      highLimit = 127;
      break;
    case 16:
      lowLimit = -32768;
      highLimit = 32767;
      break;
    case 32:
      lowLimit = -2147483648;
      highLimit = 2147483647;
      break;
  }

  leftChannel = malloc(sizeof(uint16_t) * numOfSamples);
  if (header->NumChannels > 1)
  {
    rightChannel = malloc(sizeof(uint16_t) * numOfSamples);
  }

  /// read all music data
  for (uint32_t i = 0; i<numOfSamples; ++i)
  {
    read(fileId, &leftChannel[i], sizeof(uint16_t));
//    swapEndiannesS16(&leftChannel[i]);
    // check if value was in range
    if ((leftChannel[i] < lowLimit) || (leftChannel[i] > highLimit))
    {
      printf("**value out of range\n");
    }

    if(header->NumChannels > 1)
    {
      read(fileId, &rightChannel[i], sizeof(uint16_t));
//      swapEndiannesS16(&rightChannel[i]);
      // check if value was in range
      if ((rightChannel[i] < lowLimit) || (rightChannel[i] > highLimit))
      {
        printf("**value out of range\n");
      }
    }
  }
}

void writeAsCFile(int16_t * leftChannel, int16_t * rightChannel,  sWavHeader * header)
{
  FILE * pFile;
  pFile = fopen("sound.c","w");

  /// header
  fprintf(pFile, "#include <stdint.h>\n");
  fprintf(pFile, "#include <stdio.h>\n");
  fprintf(pFile, "const uint32_t soundAudioSamplingFrequency = %d;\n", header->SampleRate);
  fprintf(pFile, "const size_t soundChannelSize = %d;\n", numOfSamples);
  fprintf(pFile, "const uint16_t soundLeftChannel[];\n");


  if (convertToMono && (header->NumChannels == 2))
  {
    fprintf(pFile, "\n\n\n");
    /// printf combined Channel
    fprintf(pFile, "const uint16_t soundLeftChannel[] = {\n");
    fprintfChannelCombine(pFile, leftChannel, rightChannel, numOfSamples);
    fprintf(pFile, "};\n");
  }
  else
  {
    if(header->NumChannels > 1)
    {
      fprintf(pFile, "const uint16_t soundRightChannel[];\n");
      fprintf(pFile, "\n\n\n");
    }
    /// printf left Channel
    fprintf(pFile, "const uint16_t soundLeftChannel[] = {\n");
    fprintfChannel(pFile, leftChannel, numOfSamples);
    fprintf(pFile, "};\n");

    if(header->NumChannels > 1)
    {
      /// printf right Channel
      fprintf(pFile, "\n\n\n");
      fprintf(pFile, "const uint16_t soundRightChannel[] = {\n");
      fprintfChannel(pFile, rightChannel, numOfSamples);
      fprintf(pFile, "};\n");
    }
  }
  fclose(pFile);
  printf("generated sound.c\n");
}

void writeAsMatlabFile(int16_t * leftChannel, int16_t * rightChannel,  sWavHeader * header)
{
  if (convertToMono)
  {
    printf("Converting to mono isn't supported on Matlab files, skipping sound.m generation.\n");
  }
  else
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
    fprintf(pFile, "fs = %d;\n",header->SampleRate);
    fprintf(pFile, "scaleFactor = %d;\n",highLimit);
    fprintf(pFile, "player = audioplayer(leftChannel/scaleFactor, fs);\n");
    fprintf(pFile, "play(player);\n");
    fprintf(pFile, "input(\"Wait for keypress\");\n");
    fclose(pFile);
    printf("generated sound.m\n");
  }
}

int usageText()
{
    printf("\n");
    printf("Converts a WAV file into C code.\n");
    printf("\n");
    printf("Usage: wav2c [-mono] filename\n");
    printf("Arguments:\n");
    printf("filename    WAV file to read\n");
    printf("mono        Combines left & right channels into one\n");
    printf("\n");
    return -1;
}

int noFilenameGiven()
{
    printf("No filename given.\n");
    return usageText();
}

int fileNotFound()
{
    printf("File not found.\n");
    return -2;
}

int main(int argc, char * argv[])
{
    sWavHeader wavHeader;
    char * inputFileName;

    /// check arguments
    if (argc<2) return noFilenameGiven();
    if(strcmp(argv[1], "--help") == 0) return usageText();
    if(strcmp(argv[1], "--version") == 0) return usageText();
    if(strcmp(argv[1], "/?") == 0) return usageText();
    if(strcmp(argv[1], "-help") == 0) return usageText();
    if(strcmp(argv[1], "-mono") == 0)
    {
        printf("mono conversion requested.\n");
        convertToMono = 1;
        if (argc<3) return noFilenameGiven();
        inputFileName = argv[2];
    }
    else
    {
        inputFileName = argv[1];
    }

    printf("Opening file \"%s\".\n", inputFileName);
    int inputFileDescriptor = open(inputFileName,O_RDONLY);
    if(inputFileDescriptor == -1) return fileNotFound();

    /// read header
    read(inputFileDescriptor, &wavHeader, sizeof(sWavHeader));
    printfHeader(&wavHeader);
    readWavData(inputFileDescriptor, &wavHeader);
    close(inputFileDescriptor);

    writeAsCFile(leftChannel, rightChannel, &wavHeader);
    writeAsMatlabFile(leftChannel, rightChannel, &wavHeader);

    free(leftChannel);
    free(rightChannel);

    printf("success\n");
    return 0;
}
