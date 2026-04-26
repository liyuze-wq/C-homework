#ifndef __READ_FILE_TO_ARRAY_H
#define __READ_FILE_TO_ARRAY_H

#include <stdint.h>

int read_file_to_array(const char *filename, uint8_t *buffer, uint32_t len);

#endif
