#ifndef CS43L22_H_
#define CS43L22_H_

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"

typedef struct {
    I2C_HandleTypeDef *hi2c;
    I2S_HandleTypeDef *hi2s;
    GPIO_TypeDef *reset_port;
    uint16_t reset_pin;
} CS43L22_config_t;




void CS43L22_init(CS43L22_config_t *cfg, uint16_t *buffer, uint32_t len);
void CS43L22_write(uint8_t address, uint8_t value);
void CS43L22_play();
void CS43L22_set_volume(uint8_t value);
uint8_t CS43L22_read(uint8_t address);




#endif /* CS43L22_H_ */
