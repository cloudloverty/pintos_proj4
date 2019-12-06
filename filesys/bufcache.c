#include "filesys/bufcache.h"

#define BUFFER_CACHE_ENTRY_NUMBER 64


void* ptr_bufcache;
struct list buffer_head_list;

void bufcache_init (void)
{
	ptr_bufcache = malloc(512 * BUFFER_CACHE_ENTRY_NUMBER);
	list_init(&buffer_head_list);
}

void bufcache_delete (void)
{
	bufcache_flush_all();
	free(ptr_bufcache);
}

bool bufcache_read (block_sector_t sector_idx, void* buf, off_t bytes_read, int chunk_size, int sector_ofs)
{
	//bc_lookup 함수로 buffer_head 리스트에서 sector_idx를 이용해 buffer head를 구함
	//if null, 
		//if buffer_head 리스트가 꽉 찼으면
			//bc_select_victim으로 내보낼 애 검색
				//내보낼애가 dirty면 디스크에 flush
			//buffer head리스트에서 vicitim 제거
		//block_read 함수로 디스크에서 데이터 꺼내와서 buffer cache로 read
	//memcpy를 사용해, buffer cache에 있는 데이터를 buf에 복사
	//buffer head 갱신. 아마 dirty나 in_use같은 플래그...?
}
//나중에 inode_read_at 함수에 있는 block_read를 bc_read로 수정

bool bufcache_write (block_Sector_t sector_idx, void* buf, off_t bytes_written, int chunk_size, int sector_ofs)
{
	//bc_lookup 함수로 buffer_head 리스트에서 sector_idx를 이용해 buffer head를 구함
	//if null, 
		//if buffer_head 리스트가 꽉 찼으면
			//bc_select_victim으로 내보낼 애 검색
				//내보낼애가 dirty면 디스크에 flush
			//buffer head리스트에서 vicitim 제거
		//block_read 함수로 디스크에서 데이터 꺼내와서 buffer cache로 read
	//memcpy를 사용해, buf에 있는 데이터를 buffer cache에 복사
	//buffer head 갱신. 아마 dirty나 in_use같은 플래그...?
}
//나중에 inode_write_at 함수에 있는 block_write를 bufcache_write로 수정

struct buffer_head* bufcache_select_victim (void) 
{
	//buffer head list에서 pop front
	//victim이 dirty인 경우 flush
	//victim이 있던 buffer_head 갱신
	//return victim
}

struct buffer_head* bufcache_find (block_sector_t sector)
{
	//buffer head list 돌면서 sector값 동일한 buffer head 찾기
	//있으면 return, 없으면 return null
}

void bufcache_flush (struct buffer_head* ptr_bufcache_entry)
{
	// block_write을 이용해서 sector에다가 flush
	// dirty값 초기화
}

void bufcache_flush_all (void)
{
	//buffer head list 돌면서 dirty인 entry는 bufcache_flush
}