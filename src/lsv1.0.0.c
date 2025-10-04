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
#include <sys/ioctl.h>

extern int errno;

// --- ANSI COLOR CODES ---
#define COLOR_RESET   "\033[0m"
#define COLOR_BLUE    "\033[0;34m"   // directories
#define COLOR_GREEN   "\033[0;32m"   // executables
#define COLOR_RED     "\033[0;31m"   // archives
#define COLOR_PINK    "\033[1;35m"   // symbolic links
#define COLOR_REVERSE "\033[7m"      // special files

enum DisplayMode {
    DEFAULT_MODE,
    LONG_MODE,
    HORIZONTAL_MODE
};

void do_ls(const char *dir, enum DisplayMode mode, int recursive);
void display_long(const char *path, const char *filename);
void display_columns_default(char **names, int n, int maxlen, const char *dir);
void display_horizontal(char **names, int n, int maxlen, const char *dir);
char **read_dir_filenames(const char *dir, int *count, int *maxlen);
int compare_names(const void *a, const void *b);
void print_colorized(const char *dir, const char *filename);

int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;
    int recursive_flag = 0;

    while ((opt = getopt(argc, argv, "lxR")) != -1) {
        switch (opt) {
            case 'l':
                mode = LONG_MODE;
                break;
            case 'x':
                mode = HORIZONTAL_MODE;
                break;
            case 'R':
                recursive_flag = 1;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x | -R] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *dir = ".";
    if (optind < argc)
        dir = argv[optind];

    do_ls(dir, mode, recursive_flag);
    return 0;
}

/* --- Comparison for qsort --- */
int compare_names(const void *a, const void *b) {
    const char *n1 = *(const char **)a;
    const char *n2 = *(const char **)b;
    return strcmp(n1, n2);
}

/* --- Colorized Printing --- */
void print_colorized(const char *dir, const char *filename) {
    struct stat fileStat;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", dir, filename);

    if (lstat(fullpath, &fileStat) < 0) {
        perror("lstat");
        printf("%s", filename);
        return;
    }

    const char *color = COLOR_RESET;

    if (S_ISDIR(fileStat.st_mode)) color = COLOR_BLUE;
    else if (S_ISLNK(fileStat.st_mode)) color = COLOR_PINK;
    else if (S_ISCHR(fileStat.st_mode) || S_ISBLK(fileStat.st_mode) ||
             S_ISSOCK(fileStat.st_mode) || S_ISFIFO(fileStat.st_mode)) color = COLOR_REVERSE;
    else if (fileStat.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) color = COLOR_GREEN;
    else if (strstr(filename, ".tar") || strstr(filename, ".gz") || strstr(filename, ".zip")) color = COLOR_RED;

    printf("%s%s%s", color, filename, COLOR_RESET);
}

/* --- Read Filenames and Sort --- */
char **read_dir_filenames(const char *dir, int *count, int *maxlen) {
    DIR *dp = opendir(dir);
    if (!dp) return NULL;
    struct dirent *entry;
    size_t cap = 64;
    char **names = malloc(cap * sizeof(char *));
    int n = 0, maxl = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        int len = strlen(entry->d_name);
        if (len > maxl) maxl = len;
        if (n >= (int)cap) {
            cap *= 2;
            names = realloc(names, cap * sizeof(char *));
        }
        names[n++] = strdup(entry->d_name);
    }
    closedir(dp);
    qsort(names, n, sizeof(char *), compare_names);
    *count = n;
    *maxlen = maxl;
    return names;
}

/* --- Long Listing --- */
void display_long(const char *path, const char *filename) {
    struct stat fileStat;
    char fullpath[1024];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, filename);
    if (lstat(fullpath, &fileStat) < 0) return;

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

    struct passwd *pw = getpwuid(fileStat.st_uid);
    struct group *gr = getgrgid(fileStat.st_gid);
    printf(" %ld %-8s %-8s %8ld ", fileStat.st_nlink,
           pw ? pw->pw_name : "?", gr ? gr->gr_name : "?",
           fileStat.st_size);

    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf("%s ", timebuf);
    print_colorized(path, filename);
    printf("\n");
}

/* --- Default (Down Then Across) --- */
void display_columns_default(char **names, int n, int maxlen, const char *dir) {
    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = width / col_width;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int i = c * rows + r;
            if (i >= n) continue;
            print_colorized(dir, names[i]);
            int len = strlen(names[i]);
            for (int s = 0; s < col_width - len; s++) printf(" ");
        }
        printf("\n");
    }
}

/* --- Horizontal (-x) --- */
void display_horizontal(char **names, int n, int maxlen, const char *dir) {
    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) width = w.ws_col;
    int spacing = 2;
    int col_width = maxlen + spacing;
    int cur = 0;

    for (int i = 0; i < n; i++) {
        if (cur + col_width > width) {
            printf("\n");
            cur = 0;
        }
        print_colorized(dir, names[i]);
        int len = strlen(names[i]);
        for (int s = 0; s < col_width - len; s++) printf(" ");
        cur += col_width;
    }
    printf("\n");
}

/* --- Recursive LS Core --- */
void do_ls(const char *dir, enum DisplayMode mode, int recursive) {
    int n = 0, maxlen = 0;
    char **names = read_dir_filenames(dir, &n, &maxlen);
    if (!names) return;

    printf("\n%s:\n", dir);
    if (mode == LONG_MODE) {
        for (int i = 0; i < n; i++) display_long(dir, names[i]);
    } else if (mode == HORIZONTAL_MODE) {
        display_horizontal(names, n, maxlen, dir);
    } else {
        display_columns_default(names, n, maxlen, dir);
    }

    if (recursive) {
        for (int i = 0; i < n; i++) {
            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", dir, names[i]);
            struct stat st;
            if (lstat(path, &st) == 0 && S_ISDIR(st.st_mode) &&
                strcmp(names[i], ".") != 0 && strcmp(names[i], "..") != 0) {
                do_ls(path, mode, recursive);
            }
        }
    }

    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}
