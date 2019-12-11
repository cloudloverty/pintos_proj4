#include "filesys/bufcache.h"

#define BUFFER_CACHE_ENTRY_NUMBER 64


void* ptr_bufcache;
static struct buffer_head buffer_head_list[BUFFER_CACHE_ENTRY_NUMBER];
static int clock;


void bufcache_init (void)
{
	ptr_bufcache = malloc(512 * BUFFER_CACHE_ENTRY_NUMBER);
	for (int i = 0; i < BUFFER_CACHE_ENTRY_NUMBER; i++) {
		buffer_head_list[i].cache = false;
		buffer_head_list[i].dirty = false;
		buffer_head_list[i].data = ptr_bufcache + BLOCK_SECTOR_SIZE*i;
	}

	lock_init(&bufcache_lock);

	clock = 0;
}

void bufcache_delete (void)
{
	bufcache_flush_all();
	free(ptr_bufcache);
}


//bc_lookup �Լ��� buffer_head ����Ʈ���� sector_idx�� �̿��� buffer head�� ����
//if null, 
	//if buffer_head ����Ʈ�� �� á����
		//bc_select_victim���� ������ �� �˻�
			//�������ְ� dirty�� ��ũ�� flush -> select victim���� ��
		//buffer head����Ʈ���� vicitim ����
	//block_read �Լ��� ��ũ���� ������ �����ͼ� buffer cache�� read
//memcpy�� �����, buffer cache�� �ִ� �����͸� buf�� ����
//buffer head ����. �Ƹ� dirty�� cache���� �÷���...?
//���߿� inode_read_at �Լ��� �ִ� block_read�� bc_read�� ����
bool bufcache_read (block_sector_t sector_idx, void* buf, int chunk_size)
{
	lock_acquire(&bufcache_lock);
	struct buffer_head* buf_head = bufcache_find(sector_idx);
	if (buf_head == NULL)
	{
		int cacheIdx = bufcache_get_empty_slot();
		if (cacheIdx == -1) {
			buf_head = bufcache_select_victim();
		}
		else {
			buf_head = &buffer_head_list[cacheIdx];
		}
		buf_head->dirty = false;
		buf_head->sector = sector_idx;
		buf_head->cache = true;
		block_read(fs_device, sector_idx, buf_head->data);
	}

	memcpy(buf, buf_head->data, BLOCK_SECTOR_SIZE);
	lock_release(&bufcache_lock);

	return true;	//���߿� �����ϴ°� check�ؼ� false return��Ű��
}

bool bufcache_write (block_sector_t sector_idx, void* buf, int chunk_size)
{
	lock_acquire(&bufcache_lock);

	struct buffer_head* buf_head = bufcache_find(sector_idx);
	if (buf_head == NULL)
	{
		int cacheIdx = bufcache_get_empty_slot();
		if (cacheIdx == -1) {
			buf_head = bufcache_select_victim();
		}
		else {
			buf_head = &buffer_head_list[cacheIdx];
		}
		buf_head->dirty = false;
		buf_head->sector = sector_idx;
		buf_head->cache = true;
		block_read(fs_device, sector_idx, buf_head->data);
	}
	//printf("trying memcpy...\n");
	memcpy(buf_head->data, buf, BLOCK_SECTOR_SIZE);
	//printf("success\n");
	buf_head->dirty = true;

	lock_release(&bufcache_lock);

	return true;	//���߿� �����ϴ°� check�ؼ� false return��Ű��
}

//victim�� dirty�� ��� flush
//victim�� �ִ� buffer_head ����
//return victim
struct buffer_head* bufcache_select_victim (void) 
{
	struct buffer_head* victim = &buffer_head_list[clock];
	if (victim->dirty) {
		bufcache_flush(victim);
	}
	
	//victim buffer head update...?
	victim->cache = false;
	clock = (clock + 1) % BUFFER_CACHE_ENTRY_NUMBER;

	return victim;
}

//buffer head list ���鼭 sector�� ������ buffer head ã��
//������ return, ������ return null
struct buffer_head* bufcache_find (block_sector_t sector)
{
	for (int i = 0; i < BUFFER_CACHE_ENTRY_NUMBER; i++)
	{
		if (buffer_head_list[i].sector == sector) {
			return &buffer_head_list[i];
		}
	}
	return NULL;
}

// block_write�� �̿��ؼ� sector���ٰ� flush
// dirty�� �ʱ�ȭ
void bufcache_flush (struct buffer_head* ptr_bufcache_entry)
{
	// block_write(struct block*, block_sector_t, void* buffer)
	block_write(fs_device, ptr_bufcache_entry->sector, ptr_bufcache_entry->data);
	ptr_bufcache_entry->dirty = false;
}

void bufcache_flush_all (void)
{
	lock_acquire(&bufcache_lock);

	for (int i = 0; i < BUFFER_CACHE_ENTRY_NUMBER; i++)
	{
		if (buffer_head_list[i].cache && buffer_head_list[i].dirty) {
			bufcache_flush(&buffer_head_list[i]);
		}
	}

	lock_release(&bufcache_lock);

}

int bufcache_get_empty_slot(void) 
{
	for (int i = 0; i < BUFFER_CACHE_ENTRY_NUMBER; i++)
	{
		if (buffer_head_list[i].cache == false)
			return i;
	}
	return -1;
}