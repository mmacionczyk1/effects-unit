#ifndef INC_TRANSPORT_H_
#define INC_TRANSPORT_H_

#include "ring_buffer.h"
#include <stdbool.h>
#include <stdint.h>
#include "lwip.h"
#include "lwip/api.h"
#include "FreeRTOS.h"

#define FRAME_MAX_PAYLOAD_LEN 1024
#define FRAME_HEADER_MAGIC_LEN 2
#define FRAME_HEADER_TYPE_LEN 4
#define FRAME_HEADER_PAYLOAD_LEN 4
#define FRAME_HEADER_LEN (FRAME_HEADER_MAGIC_LEN + FRAME_HEADER_TYPE_LEN + FRAME_HEADER_PAYLOAD_LEN)

#define CRC_BYTES 2

#define FRAME_MAX_LEN (FRAME_MAX_PAYLOAD_LEN + FRAME_HEADER_LEN + CRC_BYTES)

#define FRAME_HEADER_MAGIC 0xAAAB

#define TCP_RX_QUEUE_LEN 3


typedef struct __attribute__((packed))
{
	uint16_t magic;
	uint32_t type;
	uint32_t payload_len;
} frame_header_t;


typedef struct
{
	frame_header_t header;
	uint8_t data[FRAME_MAX_PAYLOAD_LEN];
	uint16_t crc;
} rx_message_t;

void transport_init();

bool copy_netbuf_rb(ring_buffer_t* rb, struct netbuf *buf);

bool parse_frame(ring_buffer_t* rb, rx_message_t* result);

void vTCP_ReceiveWorkerTask(void* pvParameters);

QueueHandle_t get_tcp_rx_queue();


#endif /* INC_TRANSPORT_H_ */

