#ifndef INC_CRC16_H_
#define INC_CRC16_H_

/*
CRC-16-CCITT
poly 0x1021
initial 0xffff
big endian
*/



#include <stdint.h>
#include <stdbool.h>


uint16_t crc16_initial_value();

uint16_t crc16_update(uint16_t current_crc, uint8_t byte);

uint16_t crc16_update_block(uint16_t current_crc, const uint8_t* block, uint32_t size);

uint16_t crc16_compute(const uint8_t* data, uint32_t size);

bool crc16_verify(const uint8_t* data, uint32_t size_with_crc);


#endif /* INC_CRC16_H_ */
