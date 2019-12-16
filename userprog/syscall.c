#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"
#include "filesys/file.h"


static void syscall_handler (struct intr_frame *);

static void check_add_valid(void* addr);
void set_arg(void* esp, uint32_t* argv, int argc);

void
syscall_init (void) 
{
	lock_init(&filesys_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	//printf("esp address is %x\n", f->esp);
	//hex_dump(f->esp, f->esp, 96, true);
	//printf(*(uint32_t*)0x0804f000);

  check_add_valid((uint32_t)f->esp);
  uint32_t syscall_num = *(uint32_t*)f->esp;
  //printf("syscall number: %d\n", syscall_num);
  //printf ("system call!\n");
  uint32_t argv[3];


  switch (syscall_num) {
  case SYS_HALT:
	  halt();
	  break;

  case SYS_EXIT:
	  set_arg(f->esp, argv, 1);
	  exit((int)*(uint32_t*)argv[0]);
	  break;

  case SYS_EXEC:
	  set_arg(f->esp, argv, 1);
	  check_add_valid(argv[0]);
	  f->eax = exec((const char*) * (uint32_t*)argv[0]);
	  break;

  case SYS_WAIT:
	  set_arg(f->esp, argv, 1);
	  f->eax = wait((pid_t) * (uint32_t*)argv[0]);
	  break;

  case SYS_CREATE:
	  set_arg(f->esp, argv, 2);
	  check_add_valid(argv[0]);
	  f->eax = create((const char*) * (uint32_t*)argv[0], (unsigned) * (uint32_t*)argv[1]);
	  break;

  case SYS_REMOVE:
	  set_arg(f->esp, argv, 1);
	  check_add_valid(argv[0]);
	  f->eax = remove((const char*) * (uint32_t*)argv[0]);
	  break;

  case SYS_OPEN:
	  set_arg(f->esp, argv, 1);
	  check_add_valid(argv[0]);
	  f->eax = open((const char*) * (uint32_t*)argv[0]);
	  break;

  case SYS_FILESIZE:
	  set_arg(f->esp, argv, 1);
	  check_add_valid(argv[0]);
	  f->eax = filesize((int) * (uint32_t*)argv[0]);
	  break;

  case SYS_READ:
	  set_arg(f->esp, argv, 3);
	  check_add_valid(argv[1]);
	  f->eax = read((int) * (uint32_t*)argv[0], (void*) * (uint32_t*)argv[1], (unsigned) * (uint32_t*)argv[2]);
	  break;

  case SYS_WRITE:
	  set_arg(f->esp, argv, 3);
	  check_add_valid(argv[1]);
	  f->eax = write((int)* (uint32_t*)argv[0], (const void*)* (uint32_t*)argv[1], (unsigned)* (uint32_t*)argv[2]);
	  break;

  case SYS_SEEK:
	  set_arg(f->esp, argv, 2);
	  seek((int) * (uint32_t*) argv[0], (unsigned) * (uint32_t*) argv[1]);
	  break;

  case SYS_TELL:
	  set_arg(f->esp, argv, 1);
	  f->eax = ((int) * (uint32_t*)argv[0]);
	  break;

  case SYS_CLOSE:
	  set_arg(f->esp, argv, 1);
	  close((int) * (uint32_t*)argv[0]);
	  break;

  case SYS_CHDIR:
	  set_arg(f->esp, argv, 1);
	  f->eax = chdir((const char*) * (uint32_t*)argv[0]);
	  break;

  case SYS_MKDIR:
	  set_arg(f->esp, argv, 1);
	  check_add_valid(argv[0]);
	  f->eax = mkdir((const char*) * (uint32_t*)argv[0]);
	  break;

  case SYS_READDIR:
	  set_arg(f->esp, argv, 2);
	  f->eax = readdir((int) * (uint32_t*)argv[0], (char*) * (uint32_t*)argv[1]);

  case SYS_ISDIR:
	  set_arg(f->esp, argv, 1);
	  f->eax = isdir((int) * (uint32_t*)argv[0]);
	  break;

  case SYS_INUMBER:
	  set_arg(f->esp, argv, 1);
	  f->eax = inumber((int) * (uint32_t*)argv[0]);
	  break;

  }


  //thread_exit ();
}

void 
set_arg(void* esp, uint32_t* argv, int argc)
{
	//printf("esp point to %d\n", (int) * (uint32_t*)(esp));
	//printf("esp address is %x\n", esp);
	check_add_valid(esp);

	esp += 4;							// skip syscall number
	for (int i = 0; i < argc; i++)
	{
		//printf("%d roop\n", i);
		//printf("esp address is %x\n", esp);
		check_add_valid(esp);
		argv[i] = (uint32_t*) esp;
		
		//printf("esp point to %d\n", (int) * (uint32_t*)(esp));
		//printf("arg[%d] is %d\n", i, (int) * (uint32_t*)argv[i]);

		esp += 4;
	}
}

static void 
check_add_valid(void* addr)
{
	if ((uint32_t)addr <= 0x8048000 || (uint32_t)addr >= 0xc0000000)
		exit(-1);
}

void		//0
halt(void)
{
	shutdown_power_off();
}

void		//1
exit(int status)
{
	//printf("\n\ntry to exit with %d\n", status);
	struct thread* t = thread_current();
	//printf("thread name is %s\n", t->name);
	t->exit_status = status;

	for (int i = 3; i < 128; i++) {
		if (t->fd_table[i] != NULL) {
			close(i);
		}
	}
	file_close(t->running_file);
	t->running_file = NULL;

	printf("%s: exit(%d)\n", t->name, status);
	thread_exit();
}

pid_t		//2
exec(const char* cmd_line)
{
	check_add_valid(cmd_line);

	if (*cmd_line == NULL) exit(-1);
	//return process_execute(cmd_line);
	//printf("exec. process_execute\n");
	tid_t cpid = process_execute(cmd_line);

	if (cpid == -1 || cpid == TID_ERROR) return -1;

	struct thread* c = get_child_process(cpid);
	//printf("exec. sema_down for sema_load\n");
	sema_down(&c->sema_load);


	if (!c->load_success)
	{
		remove_child_process(c);
		return -1;
	}
	else
		return cpid;

}

int			//3
wait(pid_t pid)
{
	//printf("\nwait for: pid %d\n", (int)pid);
	struct thread* c = get_child_process(pid);

	if (c == NULL) return -1;
	//printf("child pid is %d\n", c->tid);
	//printf("getChild at syscall done\n");


	int exit_status = process_wait(c->tid);

	//remove_child_process(c);
	//printf("return exit_status: %d\n", exit_status);
	return exit_status;
}

bool		//4
create(const char* file, unsigned initial_size)
{
	check_add_valid(file);

	if (*file == NULL) exit(-1);
	return filesys_create(file, initial_size);
}

bool		//5
remove(const char* file)
{
	check_add_valid(file);

	if (*file == NULL) exit(-1);
	if (strcmp(file, "/") == 0) return false;
	return filesys_remove(file);
}

int			//6
open(const char* file)
{
	check_add_valid(file);
	if (file == NULL) {
		return -1;
	}

	lock_acquire(&filesys_lock);
	struct file* open_file = filesys_open(file);
	if (open_file == NULL) {
		lock_release(&filesys_lock);
		return -1;
	}
	else
	{
		int fd = new_file(open_file);
		lock_release(&filesys_lock);
		return fd;
	}
}

int			//7
filesize(int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) return -1;

	return file_length(f);
}

int			//8
read(int fd, void* buffer, unsigned size)
{
	check_add_valid(buffer);
	lock_acquire(&filesys_lock);

	if (fd == 0)
	{
		input_getc();
		lock_release(&filesys_lock);
		return size;
	}
	else if (fd > 2) {
		struct file* f = get_file(fd);
		if (f == NULL) { 
			lock_release(&filesys_lock);
			return -1; 
		}

		
		int ans = file_read(f, buffer, size);
		lock_release(&filesys_lock);

		return ans;
	}
	lock_release(&filesys_lock);
	return -1;
}

int			//9
write(int fd, const void* buffer, unsigned size)
{
	check_add_valid(buffer);
	lock_acquire(&filesys_lock);

	if (fd == 1)
	{
		putbuf(buffer, size);
		lock_release(&filesys_lock);
		return size;
	}
	else if (fd > 2){
		struct file* f = get_file(fd);
		if (f == NULL) { 
			lock_release(&filesys_lock);
			return -1; 
		}

		struct inode* inode = file_get_inode(f);
		if (inode_is_dir(inode)) {
			lock_release(&filesys_lock);
			return -1;			// can not write to dir
		}

		int ans = file_write(f, buffer, size);
		
		lock_release(&filesys_lock);
		return ans;
	}
	lock_release(&filesys_lock);
	return -1;
}

void		//10
seek(int fd, unsigned position)
{
	struct file* f = get_file(fd);
	if (f == NULL) exit(-1);
	file_seek(f, position);
}

unsigned	//11
tell(int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) return -1;
	return file_tell(f);
}

void		//12
close(int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) exit(-1);
	close_file(fd);
}

bool		//13
chdir(const char* dir)
{
	//check_add_valid(dir);
	bool result = filesys_chdir(dir);
	return result;
}

bool		//14
mkdir(const char* dir)
{
	check_add_valid(dir);
	if (strlen(dir) == 0) {
		return false;
	}
	return filesys_create_dir(dir, 0);
}

bool		//15
readdir(int fd, char* name)
{
	struct file* f = get_file(fd);
	if (f == NULL) 
		return false;
	struct inode* inode = file_get_inode(f);
	if (!inode_is_dir(inode)) {			// file
		return false;
	}

	struct dir* dir = dir_open(inode);
	bool result = dir_readdir(dir, name);
	dir_close(dir);
	return result;
}

bool		//16
isdir (int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) exit(-1);
	if (inode_is_dir(file_get_inode(f))) return true;
	else return false;
}

int
inumber(int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) exit(-1);
	return file_get_sector(f);
}





int 
new_file(struct file* file)
{
	if (file == NULL) return -1;

	struct thread* t = thread_current();
	int new_fd_num = t->fd_max;
	t->fd_table[new_fd_num] = file;
	t->fd_max = new_fd_num + 1;
	return new_fd_num;
}

struct file*
get_file(int fd)
{
	struct thread* t = thread_current();
	if (t->fd_table[fd] == NULL)
		return NULL;
	else
		return t->fd_table[fd];
}

void 
close_file(int fd)
{
	struct file* f = get_file(fd);
	if (f == NULL) return;

	file_close(f);
	thread_current()->fd_table[fd] = NULL;
}

