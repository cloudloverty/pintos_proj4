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


//bc_lookup 함수로 buffer_head 리스트에서 sector_idx를 이용해 buffer head를 구함
//if null, 
	//if buffer_head 리스트가 꽉 찼으면
		//bc_select_victim으로 내보낼 애 검색
			//내보낼애가 dirty면 디스크에 flush -> select victim에서 함
		//buffer head리스트에서 vicitim 제거
	//block_read 함수로 디스크에서 데이터 꺼내와서 buffer cache로 read
//memcpy를 사용해, buffer cache에 있는 데이터를 buf에 복사
//buffer head 갱신. 아마 dirty나 cache같은 플래그...?
//나중에 inode_read_at 함수에 있는 block_read를 bc_read로 수정
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

	return true;	//나중에 실패하는거 check해서 false return시키기
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

	return true;	//나중에 실패하는거 check해서 false return시키기
}

//victim이 dirty인 경우 flush
//victim이 있던 buffer_head 갱신
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

//buffer head list 돌면서 sector값 동일한 buffer head 찾기
//있으면 return, 없으면 return null
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

// block_write을 이용해서 sector에다가 flush
// dirty값 초기화
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