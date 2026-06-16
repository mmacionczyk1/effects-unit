#ifndef INC_DISPATCHER_H_
#define INC_DISPATCHER_H_

#include "command_ids.h"
#include <stdint.h>

#define DISPATCH_TABLE_LEN 255

typedef void (*dispatch_handler_t)(const uint8_t* data, uint32_t len);

typedef struct
{
	cmd_id_t id;
	dispatch_handler_t handler;

} handler_entry_t;


void dispatcher_init(void);
void dispatcher_register_handler(handler_entry_t* entry);
void dispatcher_dispatch(uint32_t id, const uint8_t* data, uint32_t len);


void vDispatcherTask(void* pvParameters);

#endif /* INC_DISPATCHER_H_ */
