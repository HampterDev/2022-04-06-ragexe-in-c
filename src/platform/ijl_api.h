#ifndef PLATFORM_IJL_API_H
#define PLATFORM_IJL_API_H

#include <stddef.h>
#include <stdint.h>

struct IjlProps {
    uint32_t useProps;
    unsigned char *dibBytes;
    int dibWidth, dibHeight, dibPadBytes, dibChannels, dibColor, dibSubsampling;
    const char *jpgFile;
    const unsigned char *jpgBytes;
    int jpgSizeBytes, jpgWidth, jpgHeight, jpgChannels, jpgColor, jpgSubsampling;
    int jpgThumbWidth, jpgThumbHeight;
    int cconversionReqd, upsamplingReqd, jquality;
    unsigned char jprops[0x4e68 - 0x54];
};

enum {
    IJL_JBUFF_READPARAMS = 1,
    IJL_JBUFF_READWHOLEIMAGE = 3,
    IJL_JFILE_WRITEWHOLEIMAGE = 8,
    IJL_BGR = 2
};

__declspec(dllimport) int __stdcall ijlInit(struct IjlProps *properties);
__declspec(dllimport) int __stdcall ijlFree(struct IjlProps *properties);
__declspec(dllimport) int __stdcall ijlRead(struct IjlProps *properties, int operation);
__declspec(dllimport) int __stdcall ijlWrite(struct IjlProps *properties, int operation);

#endif
