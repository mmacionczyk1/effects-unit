#include "crc16.h"
#include "app_config.h"

uint16_t crc16_initial_value()
{
	return CRC16_INITIAL_VALUE;
}

uint16_t crc16_update(uint16_t current_crc, uint8_t byte)
{
    current_crc ^= ((uint16_t)byte << 8);
    for (uint8_t i = 0; i < 8; i++)
    {
        if (current_crc & 0x8000U)
        {
            current_crc = (current_crc << 1) ^ CRC16_POLYNOMIAL;
        }
        else
        {
            current_crc <<= 1;
        }
    }
    return current_crc;
}

uint16_t crc16_update_block(uint16_t current_crc, const uint8_t* block, uint32_t size)
{
	for(uint32_t i = 0; i < size; i++)
	{
		current_crc = crc16_update(current_crc, block[i]);
	}
	return current_crc;
}



uint16_t crc16_compute(const uint8_t* data, uint32_t size)
{
    uint16_t crc = CRC16_INITIAL_VALUE;
    for (uint32_t i = 0; i < size; i++)
    {
        crc = crc16_update(crc, data[i]);
    }
    return crc;
}

bool crc16_verify(const uint8_t* data, uint32_t size_with_crc)
{
    if (size_with_crc < 2)
    {
        return false;
    }
    uint32_t data_size = size_with_crc - 2;
    uint16_t computed_crc = crc16_compute(data, data_size);
    uint16_t received_crc = ((uint16_t)data[data_size] << 8) | data[data_size + 1];
    return (computed_crc == received_crc);
}
