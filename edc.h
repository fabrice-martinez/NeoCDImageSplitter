#ifndef EDC_H
#define EDC_H

#include <cstdint>

constexpr int CDROM_SECTOR_SIZE = 2352;
constexpr int CDROM_DATA_SIZE = 2048;
constexpr int CDROM_HEADER_SIZE = 16;

extern bool checkSector(const void* data);

#endif // EDC_H

