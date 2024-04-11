/*
Copyright (C) 2016-2019 The University of Notre Dame
This software is distributed under the GNU General Public License.
See the file LICENSE for details.
*/

//fs_dirent_mkfile
//fs_dirent_write
//fs_dirent_read

#include "kernel/types.h"
#include "namepipe.h"
#include "kmalloc.h"
#include "process.h"
#include "page.h"
#include "fs.h"

#define PIPE_SIZE PAGE_SIZE

struct namepipe {
	char *fname;
	char *buffer;
	int read_pos;
	int write_pos;
	int flushed;
	int refcount;
	struct list queue;
};

struct namepipe *namepipe_create()
{
	struct namepipe *p = kmalloc(sizeof(*p));
	if(!p) return 0;
	
	p->buffer = page_alloc(1);
	if(!p->buffer) {
		kfree(p);
		return 0;
	}
	p->read_pos = 0;
	p->write_pos = 0;
	p->flushed = 0;
	p->queue.head = 0;
	p->queue.tail = 0;
	p->refcount = 1;
	return p;
}

struct namepipe *namepipe_addref( struct namepipe *p )
{
	p->refcount++;
	return p;
}

void namepipe_flush(struct namepipe *p)
{
	if(p) {
		p->flushed = 1;
	}
}

void namepipe_delete(struct namepipe *p)
{
	if(!p) return;

	p->refcount--;
	if(p->refcount==0) {
		if(p->buffer) {
			page_free(p->buffer);
		}
		kfree(p);
	}
}

static int namepipe_write_internal(struct namepipe *p, char *buffer, int size, int blocking )
{
	if(!p || !buffer) {
		return -1;
	}
	int written = 0;
	if(blocking) {
		for(written = 0; written < size; written++) {
			while((p->write_pos + 1) % PIPE_SIZE == p->read_pos) {
				if(p->flushed) {
					p->flushed = 0;
					return written;
				}
				process_wait(&p->queue);
			}
			p->buffer[p->write_pos] = buffer[written];
			p->write_pos = (p->write_pos + 1) % PIPE_SIZE;
		}
		process_wakeup_all(&p->queue);
	} else {
		while(written < size && p->write_pos != (p->read_pos - 1) % PIPE_SIZE) {
			p->buffer[p->write_pos] = buffer[written];
			p->write_pos = (p->write_pos + 1) % PIPE_SIZE;
			written++;
		}
	}
	p->flushed = 0;
	return written;
}

int namepipe_write(struct namepipe *p, char *buffer, int size)
{
	return pipe_write_internal(p, buffer, size, 1);
}

int namepipe_write_nonblock(struct namepipe *p, char *buffer, int size)
{
	return pipe_write_internal(p, buffer, size, 0);
}

static int namepipe_read_internal(struct namepipe *p, char *buffer, int size, int blocking)
{
	if(!p || !buffer) {
		return -1;
	}
	int read = 0;
	if(blocking) {
		for(read = 0; read < size; read++) {
			while(p->write_pos == p->read_pos) {
				if(p->flushed) {
					p->flushed = 0;
					return read;
				}
				if (blocking == 0) {
					return -1;
				}
				process_wait(&p->queue);
			}
			buffer[read] = p->buffer[p->read_pos];
			p->read_pos = (p->read_pos + 1) % PIPE_SIZE;
		}
		process_wakeup_all(&p->queue);
	} else {
		while(read < size && p->read_pos != p->write_pos) {
			buffer[read] = p->buffer[p->read_pos];
			p->read_pos = (p->read_pos + 1) % PIPE_SIZE;
			read++;
		}
	}
	p->flushed = 0;
	return read;
}

int namepipe_read(struct namepipe *p, char *buffer, int size)
{
	return namepipe_read_internal(p, buffer, size, 1);
}

int namepipe_read_nonblock(struct namepipe *p, char *buffer, int size)
{
	return pipe_read_internal(p, buffer, size, 0);
}

int namepipe_size( struct namepipe *p )
{
	return PIPE_SIZE;
}
