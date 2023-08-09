#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <syslog.h>
#include <errno.h>
#include <stdlib.h>

static void _mkdir(const char *dir) {
    char tmp[256];
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, S_IRWXU);
            *p = '/';
        }
    mkdir(tmp, S_IRWXU);
}

int main(int argc, char **argv)
{
    openlog(NULL, 0, LOG_USER);
    char *writefile = argv[1];
    char *writestr = argv[2];

    if (!writefile || !writestr)
    {
        exit(1);
    }

    // Create directory if needed
    struct stat st;
    if (stat(writefile, &st) == -1)
    {
        _mkdir(writefile);
    }
    rmdir(writefile);



    int fd;
    fd = creat(writefile, 0777);

    if (fd != -1)
    {
        syslog(LOG_INFO, "Writing %s to %s", writestr, writefile);
        write(fd, writestr, strlen(writestr));
    } 
    else
    {
        syslog(LOG_ERR, "File error: %s", strerror(errno));
    }

    return 0;
}