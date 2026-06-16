#include "ring_buffer.h"


void ring_buffer_init(ring_buffer_t* rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->overruns = 0;
}

uint32_t ring_buffer_push(ring_buffer_t* rb, const uint8_t* data, uint32_t size)
{
    uint32_t head = rb->head;
    uint32_t free_space = ring_buffer_get_free(rb);
    uint32_t to_write = size;
    if (to_write > free_space)
    {
        to_write = free_space;
        rb->overruns++;
    }

    for (uint32_t i = 0; i < to_write; i++)
    {
        rb->data[head] = data[i];
        head++;
        if (head >= RING_BUFFER_SIZE_BYTES)
        {
            head = 0;
        }
    }
    rb->head = head;
    return to_write;
}

uint32_t ring_buffer_read(ring_buffer_t* rb, uint8_t* data, uint32_t max_size)
{
    uint32_t tail = rb->tail;
    uint32_t available = ring_buffer_get_count(rb);
    uint32_t to_read = available < max_size ? available : max_size;

    for (uint32_t i = 0; i < to_read; i++)
    {
        data[i] = rb->data[tail];
        tail++;
        if (tail >= RING_BUFFER_SIZE_BYTES)
        {
            tail = 0;
        }
    }
    rb->tail = tail;
    return to_read;
}

uint32_t ring_buffer_peek(ring_buffer_t* rb, uint8_t* data, uint32_t max_size)
{
	uint32_t tail = rb->tail;
	uint32_t available = ring_buffer_get_count(rb);
	uint32_t to_read = available < max_size ? available : max_size;
	for (uint32_t i = 0; i < to_read; i++)
	{
		data[i] = rb->data[tail];
		tail++;
		if (tail >= RING_BUFFER_SIZE_BYTES)
		{
			tail = 0;
		}
	}
	return to_read;
}

uint32_t ring_buffer_discard(ring_buffer_t* rb, uint32_t size)
{
	uint32_t taken = ring_buffer_get_count(rb);
	uint32_t to_discard = taken < size ? taken : size;
	rb->tail = (rb->tail + to_discard) % RING_BUFFER_SIZE_BYTES;
	return to_discard;
}

uint32_t ring_buffer_flush(ring_buffer_t* rb)
{
    uint32_t tmp = ring_buffer_get_count(rb);
    rb->head = 0;
    rb->tail = 0;
    rb->overruns = 0;
    return tmp;
}

bool ring_buffer_empty(const ring_buffer_t* rb)
{
    return rb->head == rb->tail;
}

bool ring_buffer_full(const ring_buffer_t* rb)
{
    return ((rb->head + 1) % RING_BUFFER_SIZE_BYTES) == rb->tail;
}

uint32_t ring_buffer_get_count(const ring_buffer_t* rb)
{
    return (RING_BUFFER_SIZE_BYTES + rb->head - rb->tail) % RING_BUFFER_SIZE_BYTES;
}

uint32_t ring_buffer_get_free(const ring_buffer_t* rb)
{
    return (RING_BUFFER_SIZE_BYTES - 1) - ring_buffer_get_count(rb);
}

uint32_t ring_buffer_get_overruns(const ring_buffer_t* rb)
{
    return rb->overruns;
}
