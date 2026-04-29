#include "CS43L22.h"


static CS43L22_config_t* _cfg;
static uint16_t *p_audio_buffer = NULL;
static uint32_t audio_buffer_len = 0;

// cs43l22 commands
#define ADDR_POWER_CTRL1 	 0x02
#define ADDR_POWER_CTRL2 	 0x04
#define ADDR_CLK_CTRL    	 0x05
#define ADDR_INTERFACE_CTRL1 0x06
#define ADDR_INTERFACE_CTRL2 0x07
#define ADDR_PLAYBACK_CTRL1  0x0D
#define ADDR_PLAYBACK_CTRL2  0x0F
#define ADDR_PCMA			 0x1A
#define ADDR_PCMB			 0x1B
#define ADDR_MASTER_VOL_A	 0x20
#define ADDR_MASTER_VOL_B	 0x21

#define REG_POWER_ON  0b10011110
#define REG_POWER_OFF 0b00000001
#define CLK_AUTO_DETECT (1 << 7)

#define CS43L22_I2C_ADDRESS (0b1001010 << 1)

#define VOLUME_CONVERT(vol)		((vol > 100) ? 255 : (uint8_t)(vol * 255 / 100))

void CS43L22_init(CS43L22_config_t *cfg, uint16_t *buffer, uint32_t len)
{
	_cfg = cfg;
	p_audio_buffer = buffer;
	audio_buffer_len = len;
	HAL_GPIO_WritePin(_cfg->reset_port, _cfg->reset_pin, GPIO_PIN_RESET);
	HAL_Delay(20);
	HAL_GPIO_WritePin(_cfg->reset_port, _cfg->reset_pin, GPIO_PIN_SET);
	HAL_Delay(20);

	CS43L22_write(ADDR_POWER_CTRL1, REG_POWER_OFF);
	CS43L22_write(ADDR_POWER_CTRL2, 0xAF);

	//CS43L22_write(ADDR_CLK_CTRL, CLK_AUTO_DETECT);
	CS43L22_write(ADDR_CLK_CTRL, 0x81);
	CS43L22_write(ADDR_INTERFACE_CTRL1, 0x04);

	//CS43L22_write(ADDR_PCMA, 0x00);
	//CS43L22_write(ADDR_PCMB, 0x00);
	CS43L22_write(ADDR_POWER_CTRL1, REG_POWER_ON);
	//CS43L22_write(ADDR_PCMA, 0x00);
	//CS43L22_write(ADDR_PCMB, 0x00);
	CS43L22_set_volume(70);

}
uint8_t CS43L22_read(uint8_t address)
{
	uint8_t value = 0;
	HAL_I2C_Master_Transmit(_cfg->hi2c, CS43L22_I2C_ADDRESS, &address, 1, 100);
	HAL_I2C_Master_Receive(_cfg->hi2c, CS43L22_I2C_ADDRESS, &value, 1, 100);
	return value;
}


void CS43L22_play()
{
	if (HAL_I2S_GetState(_cfg->hi2s) == HAL_I2S_STATE_READY)
	{
		HAL_I2S_Transmit_DMA(_cfg->hi2s, p_audio_buffer, audio_buffer_len);
		CS43L22_write(0x0E, 0x06);
		CS43L22_write(ADDR_POWER_CTRL1, 0x9E);
	}
}

void CS43L22_stop()
{
	HAL_I2S_DMAStop(_cfg->hi2s);
}

void CS43L22_write(uint8_t address, uint8_t value)
{
	uint8_t cmd[2] = {address, value};
	HAL_I2C_Master_Transmit(_cfg->hi2c, CS43L22_I2C_ADDRESS, cmd, 2, 100);
}

void CS43L22_set_volume(uint8_t volume_percent)
{
	uint8_t volume = VOLUME_CONVERT(volume_percent);
	if (volume > 0xE6)
	        volume -= 0xE7;
	    else
	        volume += 0x19;
	CS43L22_write(ADDR_MASTER_VOL_A, volume);
	HAL_Delay(2);
	CS43L22_write(ADDR_MASTER_VOL_B, volume);
}

