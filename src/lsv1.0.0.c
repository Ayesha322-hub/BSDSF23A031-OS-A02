/* src/lsv1.0.0.c  (update your existing file with these functions) */
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
#include <sys/ioctl.h>   // ioctl, TIOCGWINSZ
#include <sys/types.h>

extern int errno;

void do_ls(const char *dir, int long_listing);
void display_long(const char *path, const char *filename);
void display_columns_default(const char *dir); // new
char **read_dir_filenames(const char *dir, int *count, int *maxlen);

int main(int argc, char *argv[]) {
    int opt;
    int long_listing = 0;

    while ((opt = getopt(argc, argv, "l")) != -1) {
        switch (opt) {
            case 'l':
                long_listing = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *dir = ".";
    if (optind < argc)
        dir = argv[optind];

    do_ls(dir, long_listing);

    return 0;
}

/* Top-level listing function: calls either long listing or default column display */
void do_ls(const char *dir, int long_listing) {
    if (long_listing) {
        /* Read and print each entry in long format (previous logic) */
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
            display_long(dir, entry->d_name);
        }
        if (errno != 0) perror("readdir failed");
        closedir(dp);
    } else {
        /* Default behavior: multi-column "down then across" display */
        display_columns_default(dir);
    }
}

/* --- Long listing function (same as before) --- */
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

    /* File type + permission bits */
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

    printf(" %ld", fileStat.st_nlink);

    pw = getpwuid(fileStat.st_uid);
    gr = getgrgid(fileStat.st_gid);
    printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

    printf(" %8ld", fileStat.st_size);

    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf(" %s %s\n", timebuf, filename);
}

/* --------- New: read directory into dynamic array of strings --------- 
   Returns a malloc'd array of char* and sets count and maxlen.
   Caller must free each string and the array.
*/
char **read_dir_filenames(const char *dir, int *count, int *maxlen) {
    DIR *dp = opendir(dir);
    struct dirent *entry;
    if (!dp) {
        fprintf(stderr, "Cannot open directory: %s\n", dir);
        *count = 0;
        *maxlen = 0;
        return NULL;
    }

    size_t capacity = 64;
    char **names = malloc(capacity * sizeof(char *));
    if (!names) {
        perror("malloc");
        closedir(dp);
        *count = 0;
        *maxlen = 0;
        return NULL;
    }

    int n = 0;
    int maxl = 0;
    errno = 0;
    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.')
            continue;
        int len = (int)strlen(entry->d_name);
        if (len > maxl) maxl = len;

        if (n >= (int)capacity) {
            capacity *= 2;
            char **tmp = realloc(names, capacity * sizeof(char *));
            if (!tmp) { perror("realloc"); break; }
            names = tmp;
        }
        names[n] = strdup(entry->d_name);
        if (!names[n]) { perror("strdup"); break; }
        n++;
    }
    if (errno != 0) perror("readdir failed");

    closedir(dp);

    *count = n;
    *maxlen = maxl;
    return names;
}

/* --------- New: display in columns, "down then across" ---------
   Algorithm:
   - get terminal width (ioctl); fallback to 80
   - compute column width = maxlen + spacing
   - compute cols = terminal_width / col_width (>=1)
   - rows = ceil(n / cols)
   - print row by row: for r in [0..rows-1], for c in [0..cols-1]:
       index = c*rows + r
       if index < n print names[index] padded to col_width
*/
void display_columns_default(const char *dir) {
    int nfiles = 0;
    int maxlen = 0;
    char **names = read_dir_filenames(dir, &nfiles, &maxlen);
    if (!names || nfiles == 0) {
        if (names) free(names);
        return;
    }

    /* Get terminal width */
    struct winsize w;
    int term_width = 80; /* fallback */
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_col > 0) {
        term_width = w.ws_col;
    }

    int spacing = 2; /* spaces between columns */
    int col_width = maxlen + spacing;
    if (col_width <= 0) col_width = 1;

    int cols = term_width / col_width;
    if (cols < 1) cols = 1;

    int rows = (nfiles + cols - 1) / cols; /* ceil */

    /* Print row by row */
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            int idx = c * rows + r;
            if (idx >= nfiles) continue;
            printf("%-*s", col_width, names[idx]); /* left align, pad to col_width */
        }
        printf("\n");
    }

    /* free memory */
    for (int i = 0; i < nfiles; ++i) free(names[i]);
    free(names);
}

