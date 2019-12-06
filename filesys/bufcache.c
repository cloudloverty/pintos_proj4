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
	//bc_lookup �Լ��� buffer_head ����Ʈ���� sector_idx�� �̿��� buffer head�� ����
	//if null, 
		//if buffer_head ����Ʈ�� �� á����
			//bc_select_victim���� ������ �� �˻�
				//�������ְ� dirty�� ��ũ�� flush
			//buffer head����Ʈ���� vicitim ����
		//block_read �Լ��� ��ũ���� ������ �����ͼ� buffer cache�� read
	//memcpy�� �����, buffer cache�� �ִ� �����͸� buf�� ����
	//buffer head ����. �Ƹ� dirty�� in_use���� �÷���...?
}
//���߿� inode_read_at �Լ��� �ִ� block_read�� bc_read�� ����

bool bufcache_write (block_Sector_t sector_idx, void* buf, off_t bytes_written, int chunk_size, int sector_ofs)
{
	//bc_lookup �Լ��� buffer_head ����Ʈ���� sector_idx�� �̿��� buffer head�� ����
	//if null, 
		//if buffer_head ����Ʈ�� �� á����
			//bc_select_victim���� ������ �� �˻�
				//�������ְ� dirty�� ��ũ�� flush
			//buffer head����Ʈ���� vicitim ����
		//block_read �Լ��� ��ũ���� ������ �����ͼ� buffer cache�� read
	//memcpy�� �����, buf�� �ִ� �����͸� buffer cache�� ����
	//buffer head ����. �Ƹ� dirty�� in_use���� �÷���...?
}
//���߿� inode_write_at �Լ��� �ִ� block_write�� bufcache_write�� ����

struct buffer_head* bufcache_select_victim (void) 
{
	//buffer head list���� pop front
	//victim�� dirty�� ��� flush
	//victim�� �ִ� buffer_head ����
	//return victim
}

struct buffer_head* bufcache_find (block_sector_t sector)
{
	//buffer head list ���鼭 sector�� ������ buffer head ã��
	//������ return, ������ return null
}

void bufcache_flush (struct buffer_head* ptr_bufcache_entry)
{
	// block_write�� �̿��ؼ� sector���ٰ� flush
	// dirty�� �ʱ�ȭ
}

void bufcache_flush_all (void)
{
	//buffer head list ���鼭 dirty�� entry�� bufcache_flush
}