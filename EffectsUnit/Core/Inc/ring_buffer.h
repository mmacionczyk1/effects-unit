#ifndef CONTROL_TRANSPORT_RING_BUFFER_H_
#define CONTROL_TRANSPORT_RING_BUFFER_H_

#include <stdint.h>
#include <stdbool.h>
#define RING_BUFFER_SIZE_BYTES 4096

typedef struct ring_buffer
{
    uint8_t data[RING_BUFFER_SIZE_BYTES];
    uint32_t head;
    uint32_t tail;
    uint32_t overruns;
} ring_buffer_t;

void ring_buffer_init(ring_buffer_t* rb);

uint32_t ring_buffer_push(ring_buffer_t* rb, const uint8_t* data, uint32_t size);
uint32_t ring_buffer_read(ring_buffer_t* rb, uint8_t* data, uint32_t max_size);
uint32_t ring_buffer_peek(ring_buffer_t* rb, uint8_t* data, uint32_t max_size);
uint32_t ring_buffer_discard(ring_buffer_t* rb, uint32_t size);

uint32_t ring_buffer_flush(ring_buffer_t* rb);

bool ring_buffer_empty(const ring_buffer_t* rb);
bool ring_buffer_full(const ring_buffer_t* rb);

uint32_t ring_buffer_get_count(const ring_buffer_t* rb);
uint32_t ring_buffer_get_free(const ring_buffer_t* rb);

uint32_t ring_buffer_get_overruns(const ring_buffer_t* rb);



#endif /* CONTROL_TRANSPORT_RING_BUFFER_H_ */
