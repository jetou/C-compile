#ifndef BUFFER_H__
#define BUFFER_H__

#include <stddef.h>

typedef struct buffer_t {
	char *stack;
	size_t size;
	size_t top;
}buffer_t;



#endif