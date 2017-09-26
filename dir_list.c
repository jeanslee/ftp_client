/*****************************************************************************/
/*  file name: dir_list.c													 */
/*  author: Tao Tong														 */
/*  function: create a list to store directories, and find the sub dir 		 */
/*****************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "dir_list.h"

dir_list* create_dir_list(int length, int dir_is_local)
{
	dir_list* p_dir_list = (dir_list*)malloc(sizeof(dir_list));
	p_dir_list->list = (dir_node**)malloc(sizeof(dir_node*) * length);
	p_dir_list->count = 0;
	if (dir_is_local)
		p_dir_list->is_local = 1;
	else
		p_dir_list->is_local = 0;
	return (p_dir_list);
}

dir_node* create_dir_node(char* up_level_name, char* dir_name)
{
	dir_node* p = (dir_node*)malloc(sizeof(dir_node));
	p->dir_name = (char*)malloc(sizeof(char) * (strlen(up_level_name) + strlen(dir_name)) + 4);
	strcpy(p->dir_name, up_level_name);
	if (strlen(p->dir_name) > 0)	strcat(p->dir_name, "/");
	strcat(p->dir_name, dir_name);
	p->is_dir = 0;
	return (p);
}

int add_dir_list(dir_list* p_dir_list, char* up_level_name, char* dir_name)
{
	p_dir_list->list[p_dir_list->count] = create_dir_node(up_level_name, dir_name);
	if (p_dir_list->is_local)
	{
		p_dir_list->list[p_dir_list->count]->is_dir = local_is_dir(p_dir_list->list[p_dir_list->count]->dir_name);
	}
	p_dir_list->count++;
	return (p_dir_list->count);
}

int modify_last_node_to_dir(dir_list* p_dir_list)
{
	p_dir_list->list[p_dir_list->count - 1]->is_dir = 1;
	return (0);
}

int get_last_node_mode(dir_list* p_dir_list)
{
	return (p_dir_list->list[p_dir_list->count - 1]->is_dir);
}

char* get_last_node_name(dir_list* p_dir_list)
{
	return (p_dir_list->list[p_dir_list->count - 1]->dir_name);
}

int clean_dir_list(dir_list* p_dir_list)
{
	int i = 0;
	for (i = 0; i < p_dir_list->count; i++)
	{
		free(p_dir_list->list[i]->dir_name);
		free(p_dir_list->list[i]);
	}
	free(p_dir_list->list);
	free(p_dir_list);
	return (0);
}

int local_is_dir(char* dirname)
{
	struct stat buf;
	stat(dirname, &buf);
//	printf("local_is_dir: %s\n", dirname);
	return S_ISDIR(buf.st_mode);
}

int get_dir_list(dir_list* p_dir_list, char* cur_dir)
{
	DIR* pdir;
	struct dirent* pdirent;
	if (NULL == (pdir = opendir(cur_dir)))
	{
		perror("opendir");
		return (-1);
	}
	while (NULL != (pdirent = readdir(pdir)))
	{
		if (!strcmp(pdirent->d_name, ".") || !strcmp(pdirent->d_name, ".."))	continue;
		add_dir_list(p_dir_list, cur_dir, pdirent->d_name);
		if (get_last_node_mode(p_dir_list))
		{
			get_dir_list(p_dir_list, get_last_node_name(p_dir_list));
		}
	}
	closedir(pdir);
	return (0);	
}
