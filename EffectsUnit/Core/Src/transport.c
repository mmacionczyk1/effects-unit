#include "transport.h"
#include "stm32f4xx_hal.h"
#include "lwip/api.h"
#include "crc16.h"
#include <string.h>
#include "app_config.h"
#include "queue.h"
#include "task.h"

static ring_buffer_t rx_rb;
static QueueHandle_t rx_queue;
static StaticQueue_t queue_buffer;

static uint8_t queue_storage[TCP_RX_QUEUE_LEN * sizeof(rx_message_t)] __attribute__((aligned(4)));


void transport_init()
{
	if (rx_queue != NULL)
		return;
	rx_queue = xQueueCreateStatic(TCP_RX_QUEUE_LEN, sizeof(rx_message_t), queue_storage, &queue_buffer);
	configASSERT(rx_queue != NULL);
}

bool copy_netbuf_rb(ring_buffer_t* rb, struct netbuf *buf)
{
    struct pbuf *p;
    uint32_t total_packet_len = buf->p->tot_len;

    if (ring_buffer_get_free(rb) < total_packet_len)
    {
        rb->overruns++;
        return false;
    }
    for (p = buf->p; p != NULL; p = p->next)
    {
        uint32_t bytes_written = ring_buffer_push(rb, (const uint8_t*)p->payload, p->len);

        if (bytes_written != p->len) {
            return false;
        }
    }

    return true;
}


bool parse_frame(ring_buffer_t* rb, rx_message_t* result)
{
	uint8_t raw_header[FRAME_HEADER_LEN];
	uint32_t r = ring_buffer_peek(rb, raw_header, FRAME_HEADER_LEN);
	if (r < FRAME_HEADER_LEN)
	{
		return false;
	}

	frame_header_t fh;
	memcpy(&fh, raw_header, FRAME_HEADER_LEN);

	if(fh.magic != FRAME_HEADER_MAGIC)
	{
		ring_buffer_discard(rb, 1);
		return false;
	}
	if(fh.payload_len > FRAME_MAX_PAYLOAD_LEN)
	{
		ring_buffer_discard(rb, 1);
		return false;
	}

	uint32_t total_len = fh.payload_len + FRAME_HEADER_LEN + CRC_BYTES;
	if(ring_buffer_get_count(rb) < total_len)
	{
		return false;
	}
	uint8_t frame_buf[FRAME_MAX_LEN];
	ring_buffer_peek(rb, frame_buf, total_len);
	uint16_t crc = crc16_compute(frame_buf, total_len - CRC_BYTES);

	uint16_t received_crc = frame_buf[total_len - 2] | (frame_buf[total_len - 1] << 8);

	if(crc != received_crc)
	{
		ring_buffer_discard(rb, 1);
		return false;
	}
	/*
	ring_buffer_read(rb, (uint8_t*)&result->header, FRAME_HEADER_LEN);
	ring_buffer_read(rb, result->data, plen);
	ring_buffer_read(rb, (uint8_t*)&result->crc, CRC_BYTES);
	*/
	memcpy(&result->header, frame_buf, FRAME_HEADER_LEN);
	memcpy(result->data, &frame_buf[FRAME_HEADER_LEN], fh.payload_len);
	result->crc = received_crc;
	ring_buffer_discard(rb, total_len);

	return true;
}


void vTCP_ReceiveWorkerTask(void* pvParameters)
{
	rx_message_t msg;
	struct netconn *server_conn, *client_conn;
	struct netbuf *buf;
	(void)pvParameters;

	ring_buffer_init(&rx_rb);

	server_conn = netconn_new(NETCONN_TCP);
	configASSERT(server_conn != NULL);

	if (netconn_bind(server_conn, IP_ADDR_ANY, TCP_PORT) != ERR_OK)
	{
		vTaskDelete(NULL);
	}

	netconn_listen(server_conn);

	while (1)
	{
		if (netconn_accept(server_conn, &client_conn) != ERR_OK)
		{
			continue;
		}

		ring_buffer_flush(&rx_rb);

		while (netconn_recv(client_conn, &buf) == ERR_OK)
		{

			if (!copy_netbuf_rb(&rx_rb, buf))
			{
				netbuf_delete(buf);
				break;
			}

			netbuf_delete(buf);
			while (parse_frame(&rx_rb, &msg))
				xQueueSend(rx_queue, &msg, pdMS_TO_TICKS(1000));
		}
		netconn_close(client_conn);
		netconn_delete(client_conn);
	}
}


QueueHandle_t get_tcp_rx_queue()
{
	return rx_queue;
}
