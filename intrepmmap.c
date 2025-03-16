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
        fprintf(stderr, "Usage %s <filename> <search_str> <replace_str>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char *filename = argv[1];
    char *search_str = argv[2];
    char *replace_str = argv[3];

    if (strlen(search_str) != strlen(replace_str))
    {
        perror("Only can replace strings of same length");
        return EXIT_FAILURE;
    }

    return search_and_replace(filename, search_str, replace_str);
}

int search_and_replace(const char *filename, char *search_str, char *replace_str)
{
    struct stat file_stat;
    int result = EXIT_FAILURE;
    size_t search_len = strlen(search_str);
    size_t replace_len = strlen(replace_str);
	
    if(stat(filename, &file_stat) == -1)
    {
        perror("Stat failed");
        return EXIT_FAILURE;
    }
    off_t file_size = file_stat.st_size;
    char *buffer = NULL;

    int fd = open(filename, O_RDWR);
    if(fd == -1)
    {
        perror("Opening file failed");
        goto cleanup;
    }

	buffer = (char *)mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(buffer == MAP_FAILED)
    {
        perror("MMAP failed");
        buffer = NULL;
        goto cleanup;
    }

    char *position = memmem(buffer, file_size, search_str, search_len);
    while (position != NULL)
    {
        off_t replace_pos = (position - buffer);
        memcpy(buffer + replace_pos, replace_str, replace_len);
        position = memmem(buffer + replace_pos + replace_len, file_size - (replace_pos + replace_len), search_str, search_len);
    }

    if (msync(buffer, file_size, MS_SYNC) == -1)
    {
        perror("MSYNC failed");
        goto cleanup;
    }
    result = EXIT_SUCCESS;

cleanup:
    if (buffer && buffer != MAP_FAILED)
    {
        if (munmap(buffer, file_size) == -1)
        {
            perror("MUNMAP failed");
        }
    }
    if (fd != -1)
    {
        close(fd);
    }
    return result;
}