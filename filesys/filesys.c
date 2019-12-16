#include "filesys/filesys.h"
#include <debug.h>
#include <stdio.h>
#include <string.h>
#include "filesys/file.h"
#include "filesys/free-map.h"
#include "filesys/inode.h"
#include "filesys/directory.h"
#include "threads/thread.h"

/* Partition that contains the file system. */
struct block *fs_device;

static void do_format (void);

/* Initializes the file system module.
   If FORMAT is true, reformats the file system. */
void
filesys_init (bool format) 
{
  fs_device = block_get_role (BLOCK_FILESYS);
  if (fs_device == NULL)
    PANIC ("No file system device found, can't initialize file system.");

  inode_init ();
  free_map_init ();

  if (format) 
    do_format ();

  free_map_open ();

  thread_current()->dir = dir_open_root();
}

/* Shuts down the file system module, writing any unwritten data
   to disk. */
void
filesys_done (void) 
{
  free_map_close ();
}

/* Creates a file named NAME with the given INITIAL_SIZE.
   Returns true if successful, false otherwise.
   Fails if a file named NAME already exists,
   or if internal memory allocation fails. */
bool
filesys_create (const char *name, off_t initial_size) 
{
	//printf("filesys_create: creating \"%s\"...\n", name);
  block_sector_t inode_sector = 0;
  char* filename = filesys_get_last(name);
  //printf("%d\n\n", initial_size);
  //printf("filesys_create: filename is %s\n", filename);
  struct dir* dir = filesys_get_dir_path(name);

  bool success;
  if (filename != NULL)
  {
	  success = (dir != NULL
		  && free_map_allocate(1, &inode_sector)
		  && inode_create(inode_sector, initial_size, FILE)
		  && dir_add(dir, filename, inode_sector));

  }
  else {
	  success = false;
  }

  if (success)
	  //printf("create file %s\n", filename);

  if (!success && inode_sector != 0) 
    free_map_release (inode_sector, 1);
  dir_close (dir);
  free(filename);
  //printf("create done\n");

  return success;
}

bool
filesys_create_dir(const char* name, off_t initial_size)
{
	//printf("filesys_create: creating \"%s\"...\n", name);
	block_sector_t inode_sector = 0;
	char* filename = filesys_get_last(name);
	//printf("filesys_create: filename is %s\n", filename);
	struct dir* dir = filesys_get_dir_path(name);

	bool success;
	if (filename != NULL)
	{
		int Entrysize = dir_get_dirEntrySize();
		struct inode* inode = NULL;
		success = (dir != NULL
			&& !dir_lookup(dir, filename, &inode)
			&& free_map_allocate(1, &inode_sector)
			&& dir_create(inode_sector, 16 * Entrysize, dir_get_inode(dir))
			&& dir_add(dir, filename, inode_sector));
	}
	else {
		success = false;
	}

	if (!success && inode_sector != 0)
		free_map_release(inode_sector, 1);
	dir_close(dir);
	free(filename);
	//printf("create done\n");

	return success;
}

/* Opens the file with the given NAME.
   Returns the new file if successful or a null pointer
   otherwise.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
struct file *
filesys_open (const char *name)
{
   if (strlen(name) == 1 | name[0] == "/") {
	   return file_open(inode_open(ROOT_DIR_SECTOR));
   }

  char* filename = filesys_get_last(name);
  struct dir* dir = filesys_get_dir_path(name);
  //if (dir_is_root(dir))
  	//printf("dir is root. it should not be root\n");
  
  //printf("filesys_open: filename is %s\n", filename);
  struct inode *inode = NULL;
  if (dir != NULL) {
	  bool result;
	  result = dir_lookup(dir, filename, &inode);
	  //if (result) //printf("success to find %s\n", filename);
	  //else //printf("fail to find %s\n", filename);
  }


  dir_close (dir);
  free(filename);

  return file_open (inode);
}

/* Deletes the file named NAME.
   Returns true if successful, false on failure.
   Fails if no file named NAME exists,
   or if an internal memory allocation fails. */
bool
filesys_remove (const char *name) 
{
  char* filename = filesys_get_last(name);
  struct dir* dir = filesys_get_dir_path(name);
  //printf("try to remove %s\n", filename);
  bool success = dir != NULL && dir_remove (dir, filename);
  free(filename);
  dir_close (dir); 
  //printf("%s\n", success ? "true" : "false");
  return success;
}

/* Formats the file system. */
static void
do_format (void)
{
  printf ("Formatting file system...");
  free_map_create ();
  if (!dir_create (ROOT_DIR_SECTOR, 16, NULL))
    PANIC ("root directory creation failed");
  free_map_close ();
  printf ("done.\n");
}

/*
	<<< usage of strtok_r >>>
   char s[] = "  String to  tokenize. ";
   char *token, *save_ptr;
   for (token = strtok_r (s, " ", &save_ptr); token != NULL;
		token = strtok_r (NULL, " ", &save_ptr))
	 printf ("'%s'\n", token);
	   strlcpy(temp, file_name, strlen(file_name)+1);
*/
// return dir which contain last directory
// open directory through the path
// . => current dir, do nothing
// .. => parent dir
struct dir*
filesys_get_dir_path (char* path_name)
{
	//printf("filesys_get_dir_parth: start\n");
	if (strlen(path_name) == 0) {
		return NULL;
	}

	struct dir* dir;

	char s[strlen(path_name) + 1];					// including NULL at the end
	strlcpy(s, path_name, strlen(path_name)+1);		// add NULL at the end automatically
	char* token, * save_ptr, * next_token;


	if (s[0] == '/')		// absolute path
	{					
		dir = dir_open_root();
	}
	else                    // relative path
	{
		if (thread_current()->dir == NULL)
		{
			//printf("filesys_get_dir_parth: cwd is null\n");
			return NULL;
		}
		//if (dir_is_root(thread_current()->dir))
		//	printf("dir is root. Why is it not updated???\n");
		dir = dir_reopen(thread_current()->dir);


	}

	for (token = strtok_r(s, "/", &save_ptr); token != NULL;)
	{
		next_token = strtok_r(NULL, "/", &save_ptr);
		if (next_token == NULL) 
		{
			//file_name = token;
			//printf("filesys_get_dir_path: current token %s is last\n", token);
			break;
		}

		struct inode* inode = NULL;
		if (strcmp(token, ".") == 0) 
		{
			token = next_token;
			continue;
		}
		else if (strcmp(token, "..") == 0)
		{
			struct inode* inodeTemp = dir_get_inode(dir);
			block_sector_t parentSector = inode_get_parent(inodeTemp);
			inode = inode_open(parentSector);
			if (inode == NULL) {
				return NULL;
			}
			dir_close(dir);
			dir = dir_open(inode);
		}
		else
		{
			if (!dir_lookup(dir, token, &inode)) {
				//printf("filesys_get_dir_path: no such name %s\n", token);
				return NULL;
			}

			if (inode_is_dir(inode)) // if dir
			{
				dir_close(dir);
				dir = dir_open(inode);
			}
			else
			{
				inode_close(inode);
				break;
			}
		}
		token = next_token;
	}
	//printf("filesys_get_dir_path: done\n");
	return dir;
}



// return NULL if file name is . or ..
// else, return file name in given path_name
char*
filesys_get_last(char* path_name)
{
	//printf("filesys_get_filename: start\n");

	if (strlen(path_name) == 0) {
		return NULL;
	}


	char s[strlen(path_name) + 1];						// including NULL at the end
	strlcpy(s, path_name, strlen(path_name) + 1);		// add NULL at the end automatically
	char* token, * save_ptr, * temp;

	for (token = strtok_r(s, "/", &save_ptr); token != NULL; token = strtok_r(NULL, "/", &save_ptr))
	{
		if (token == NULL) {
			return NULL;
		}
		temp = token;

	}

	char* filename = malloc(strlen(temp) + 1);			// including NULL at the end
	strlcpy(filename, temp, strlen(temp) + 1);
	if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
		return NULL;
	}
	//printf("filesys_get_filename: done\n");
	return filename;
}

// return file name in given path_name, including . and ..
char*
filesys_get_filename_special(char* path_name)
{
	//printf("filesys_get_filename: start\n");

	if (strlen(path_name) == 0) {
		return NULL;
	}

	char s[strlen(path_name) + 1];						// including NULL at the end
	strlcpy(s, path_name, strlen(path_name) + 1);		// add NULL at the end automatically
	char* token, * save_ptr, * temp;

	for (token = strtok_r(s, "/", &save_ptr); token != NULL; token = strtok_r(NULL, "/", &save_ptr))
	{
		temp = token;
		if (temp == NULL)
			return NULL;
	}
	char* filename = malloc(strlen(temp) + 1);			// including NULL at the end
	strlcpy(filename, temp, strlen(temp) + 1);

	return filename;
}

bool
filesys_chdir (const char* path_name)
{
	char* filename = filesys_get_last(path_name);
	//printf("chdir: filename is %s\n", filename);
	struct dir* dir = filesys_get_dir_path(path_name);

	if (strcmp(filename, ".") == 0)
	{
		// current dir is right dir
	}
	else if (strcmp(filename, "..") == 0)
	{
		dir = dir_get_parent(dir);
		if (dir == NULL) {
			free(filename);
			return false;
		}
	}
	else
	{
		struct inode* inodeT;
			//if (dir_is_root(dir))
				//printf("current dir is root and filename is %s\n", filename);
		if (!dir_lookup(dir, filename, &inodeT)) {
			dir_close(dir);
			free(filename);
			//printf("fail to find %s\n", filename);
			return false;
		}
		//printf("find %s in dir\n", filename);
		if (inode_is_dir(inodeT))
		{
			dir_close(dir);
			dir = dir_open(inodeT);
			//printf("found real dir\n");
		}
		else {
			dir_close(dir);
			free(filename);
			return false;
		}
	}


	//struct inode* inode_ = NULL;
	////if (dir_is_root(dir))
	////	printf("dir is root\n");
	//struct dir* dirParent = dir_get_parent(dir);
	////if (dir_is_root(dirParent))
	////	printf("parent of root is root and filename is %s\n", filename);
	//if (!dir_lookup(dirParent, filename, &inode_)) {
	//	//printf("there is no %s in dir\n", filename);
	//    inode_close(inode_);
	//	dir_close(dir);
	//	free(filename);
	//	return false;
	//}

	//if (dir_is_root(dir))
    	//printf("dir is root. it should be a pleaseafjasodfj\n");
	struct inode* inode = dir_get_inode(dir);
	if (inode == NULL || inode_get_removed(inode)) {
		dir_close(dir);
		free(filename);
		return false;
	}
	//free(dirname);
	dir_close(thread_current()->dir);
	//dir_close(dir);
	thread_current()->dir = dir;
	free(filename);
	return true;
}