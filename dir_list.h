#ifndef _DIR_LIST_H
#define _DIR_LIST_H

// store a node for just a dir or a single file
typedef struct _dir_node
{
	char* dir_name;
	int is_dir;
} dir_node;

// store a list of dirs and files
typedef struct _dir_list
{
	dir_node** list;
	int count;
	int is_local;
} dir_list;

// create a list to store dirs
// argument length: the length of the list
// argument dir_is_local: non-zero to store local dir, zero to store remote dir(dir on ftp server)
dir_list* create_dir_list(int length, int dir_is_local);

// create a node to store a single dir or a single file
dir_node* create_dir_node(char* up_level_name, char* dir_name);

// add a node to the specificate dir list
int add_dir_list(dir_list* p_dir_list, char* up_level_name, char* dir_name);

// specificate the last node of dir list to dir mode
int modify_last_node_to_dir(dir_list* p_dir_list);

// get the last node mode, return 1 for dir, return 0 for regular file
int get_last_node_mode(dir_list* p_dir_list);

// get the dir name or file name of the last node
char* get_last_node_name(dir_list* p_dir_list);

// clean the dir list, free allocated memory
int clean_dir_list(dir_list* p_dir_list);

// check if the specificated dirname is local dir
int local_is_dir(char* dirname);

// store the dir and sub dir and sub files to this dir list
int get_dir_list(dir_list* p_dir_list, char* cur_dir);

#endif
