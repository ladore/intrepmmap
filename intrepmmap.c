#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

int search_and_replace(const char *filename, char *search_str, char *replace_str);

int main(int argc, char *argv[]) 
{
	if (argc < 4)
	{
		fprintf(stderr, "Usage %s <filename> <search_str> <replace_str>\n",argv[0]);
		return EXIT_FAILURE;
	}

	char *filename = argv[1];
	char *search_str = argv[2];
	char *replace_str = argv[3];

	if(strlen(search_str) != strlen(replace_str))
	{
		perror("Only can replace strings of same length");
		return EXIT_FAILURE;
	}

	return search_and_replace(filename, search_str, replace_str);
}

int search_and_replace(const char *filename, char *search_str, char *replace_str)
{
	struct stat file_stat;
	if(stat(filename, &file_stat) == -1)
	{
		perror("Stat failed");
		return EXIT_FAILURE;
	}
	off_t file_size = file_stat.st_size;

	int fd = open(filename, O_RDWR);
	if(fd == -1)
	{
		perror("Opening file failed");
		return EXIT_FAILURE;
	}

	if(ftruncate(fd,file_size) == -1)
	{
		close(fd);
		perror("Truncate failed");
		return EXIT_FAILURE;
	}

	char *buffer= (char *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(buffer == MAP_FAILED)
	{
		perror("MMAP failed");
		close(fd);
		return EXIT_FAILURE;
	}

	char *position = memmem(buffer,file_size, search_str, strlen(search_str));
	while(position != NULL)
	{
		off_t replace_pos = (position - buffer);
		memcpy(buffer + replace_pos, replace_str, strlen(replace_str));
		position = memmem(buffer + replace_pos + strlen(replace_str), file_size, search_str, strlen(search_str));
	}

	if(msync(buffer,file_size, MS_SYNC) == -1)
	{
		perror("MSYNC failed");
		close(fd);
		return EXIT_FAILURE;
	}

	if(munmap(buffer, file_size) == -1)
	{
		perror("MUNMAP failed");
		close(fd);
		return EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}
