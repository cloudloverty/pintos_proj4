#include "filesys/inode.h"
#include <list.h>


struct buffer_head
{
	struct inode* inode;
	block_sector_t sector;
	void* data;				// for buffer cache entry

	bool dirty;
	bool in_use;

	//lock

};