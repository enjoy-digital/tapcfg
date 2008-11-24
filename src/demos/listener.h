
#ifndef LISTENER_H
#define LISTENER_H

#include "tapcfg.h"

typedef struct listener_s listener_t;

listener_t *listener_init();
void listener_destroy(listener_t *listener);

int listener_start(listener_t *listener, tapcfg_t *tapcfg);
unsigned short listener_get_port(listener_t *listener);
void listener_set_port(listener_t *listener, unsigned short port);

#endif /* LISTENER_H */

