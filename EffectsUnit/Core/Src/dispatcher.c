#include "dispatcher.h"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "transport.h"

static dispatch_handler_t dispatch_table[DISPATCH_TABLE_LEN] = {NULL};
static QueueHandle_t rx_queue = NULL;


void dispatcher_init(void)
{
	if (rx_queue != NULL)
			return;
	rx_queue = get_tcp_rx_queue();
	return;
}

void dispatcher_register_handler(handler_entry_t* entry)
{
	if(entry == NULL || entry->id >= DISPATCH_TABLE_LEN)
	{
		return;
	}
	dispatch_table[entry->id] = entry->handler;
}

void dispatcher_dispatch(uint32_t id, const uint8_t* data, uint32_t len)
{
	if(id >= DISPATCH_TABLE_LEN)
	{
		return;
	}

	if (dispatch_table[id] != NULL)
	{
		dispatch_table[id](data, len);
	}
}

void vDispatcherTask(void* pvParameters)
{
	dispatcher_init();

	if (rx_queue == NULL)
	{
		configASSERT(false);
		vTaskDelete(NULL);
	}
	while(1)
	{
		rx_message_t rx_msg;
		if (xQueueReceive(rx_queue, &rx_msg, pdMS_TO_TICKS(1000)) == pdTRUE)
			dispatcher_dispatch(rx_msg.header.type, rx_msg.data, rx_msg.header.payload_len);
	}

}
