#include "filesys/inode.h"
#include "filesys/filesys.h"
#include "devices/block.h"
#include "threads/synch.h"
#include <list.h>


struct buffer_head
{
	struct inode* inode;
	block_sector_t sector;	// for disk
	void* data;				// for buffer cache entry

	bool dirty;
	bool cache;

	struct list_elem bh_elem;
};

struct lock bufcache_lock;

void bufcache_init(void);
void bufcache_delete(void);
bool bufcache_read(block_sector_t, void*, off_t, int, off_t);
bool bufcache_write(block_sector_t, void*, off_t, int, off_t);
struct buffer_head* bufcache_select_victim(void);
struct buffer_head* bufcache_find(block_sector_t);
void bufcache_flush(struct buffer_head*);
void bufcache_flush_all(void);
int bufcache_get_empty_slot(void);