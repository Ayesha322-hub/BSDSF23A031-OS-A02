#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

extern int errno;

// Function declarations
void do_ls(const char *dir, int long_listing);
void display_long(const char *path, const char *filename);

int main(int argc, char *argv[]) {
    int opt;
    int long_listing = 0;

    // Parse command-line options (-l)
    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Directory to list (default = current directory)
    const char *dir = ".";
    if (optind < argc)
        dir = argv[optind];

    // Perform directory listing
    do_ls(dir, long_listing);

    return 0;
}

// Main listing function
void do_ls(const char *dir, int long_listing) {
    struct dirent *entry;
    DIR *dp = opendir(dir);

    if (dp == NULL) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        return;
    }

    errno = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;

        if (long_listing)
            display_long(dir, entry->d_name);
        else
            printf("%s  ", entry->d_name);
    }

    if (!long_listing)
        printf("\n");

    if (errno != 0)
        perror("readdir failed");

    closedir(dp);
}

// Function to display long format details
void display_long(const char *path, const char *filename) {
    struct stat fileStat;
    char fullpath[1024];
    struct passwd *pw;
    struct group *gr;

    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);

    if (lstat(fullpath, &fileStat) < 0) {
        perror("lstat");
        return;
    }

    // File type and permissions
    printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");

    // Number of hard links
    printf(" %ld", fileStat.st_nlink);

    // User and group
    pw = getpwuid(fileStat.st_uid);
    gr = getgrgid(fileStat.st_gid);
    printf(" %-8s %-8s", pw->pw_name, gr->gr_name);

    // File size
    printf(" %8ld", fileStat.st_size);

    // Modification time
    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf(" %s", timebuf);

    // File name
    printf(" %s\n", filename);
}

