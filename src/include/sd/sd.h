// #define _SD_H_

int k_sd_init(void);
int sdRead(u8 *buf, u64 startSector, u32 sectorNumber);
int sdWrite(u8 *buf, u64 startSector, u32 sectorNumber);
int sdTest(void);
