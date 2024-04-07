
#include "vpuapi.h"
#include "header_struct.h"
#include "cvi_h265_interface.h"
#undef CLIP3
#define CLIP3(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))

typedef struct UserDataList_struct {
    struct list_head list;
    Uint8 *userDataBuf;
    Uint32 userDataLen;
} UserDataList;

Uint32 seiEncode(CodStd format, Uint8 *pSrc, Uint32 srcLen, Uint8 *pBuffer,
         Uint32 bufferSize);
BOOL H264SpsAddVui(cviH264Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize);
BOOL H265SpsAddVui(cviH265Vui *pVui, void **ppBuffer, Int32 *pBufferSize, Int32 *pBufferBitSize);
void GenQpMapFromRoiRegion(cviRoiParam *roiParam, Uint32 picWidth, Uint32 picHeight, int initQp, Uint8 *roiCtuMap, CodStd bitstreamFormat);
int setMapData(int core_idx, cviRoiParam *roi_rect, int roi_base_qp, int picWidth, int picHeight, WaveCustomMapOpt *customMapOpt,
                                                CodStd  bitstreamFormat, PhysicalAddress *addr);