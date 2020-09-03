#ifndef _WL_ATMOS_H_
#define _WL_ATMOS_H_

void Init3DPoints();

void DrawStarSky(byte *vbuf, uint32_t vbufPitch);

void DrawHighQualityStarSky(byte *vbuf, uint32_t vbufPitch);

void DrawRain(byte *vbuf, uint32_t vbufPitch, byte *zbuf, uint32_t zbufPitch);

void DrawSnow(byte *vbuf, uint32_t vbufPitch, byte *zbuf, uint32_t zbufPitch);

#endif
