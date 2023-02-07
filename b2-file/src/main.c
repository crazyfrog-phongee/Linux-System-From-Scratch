#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <time.h>

int main(int argc, char const *argv[])
{
	struct stat sb;

	if (argc != 2)
	{
		fprintf(stderr, "Usage: %s <pathname>\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	
	if (lstat(argv[1], &sb) == -1)
	{
		perror("lstat");
		exit(EXIT_FAILURE);
	}

	printf("ID of containing device:  [%jx,%jx]\n",
                   (uintmax_t) major(sb.st_dev),
		   (uintmax_t) minor(sb.st_dev));

	printf("File type:                ");

	switch(sb.st_mode & S_IFMT)
	{
		case S_IFSOCK:
			printf("socket\n");
			break;
		case S_IFLNK:
			printf("symbolic link\n");
			break;
		case S_IFREG:
			printf("regular file\n");
			break;
		case S_IFBLK:
			printf("block device\n");
			break;
		case S_IFDIR:
			printf("directory\n");
			break;
		case S_IFCHR:
			printf("character device\n");
			break;
		case S_IFIFO:
			printf("FIFO\n");
			break;
	}

	printf("File size:                %jd bytes\n",
                   (intmax_t) sb.st_size);

	printf("Last status change:       %s", ctime(&sb.st_ctime));
        printf("Last file access:         %s", ctime(&sb.st_atime));
        printf("Last file modification:   %s", ctime(&sb.st_mtime));
	
	return 0;
}

