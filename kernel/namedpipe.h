#ifndef NAMEPIPE_H
#define NAMEPIPE_H

#include "kernel/types.h"

struct pipe *namepipe_create(char *fname);
struct pipe *namepipe_addref( struct pipe *p );
void namepipe_delete(struct pipe *p);
void namepipe_flush(struct pipe *p);

int namepipe_write(struct pipe *p, char *buffer, int size);
int namepipe_write_nonblock(struct pipe *p, char *buffer, int size);
int namepipe_read(struct pipe *p, char *buffer, int size);
int namepipe_read_nonblock(struct pipe *p, char *buffer, int size);
int namepipe_size( struct pipe *p);

#endif
