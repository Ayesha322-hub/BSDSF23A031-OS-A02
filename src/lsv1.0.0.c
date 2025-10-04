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

enum DisplayMode {
    DEFAULT_MODE,
    LONG_MODE,
    HORIZONTAL_MODE
};

void do_ls(const char *dir, enum DisplayMode mode);
void display_long(const char *path, const char *filename);
void display_columns_default(char **names, int n, int maxlen);
void display_horizontal(char **names, int n, int maxlen);
char **read_dir_filenames(const char *dir, int *count, int *maxlen);
int compare_names(const void *a, const void *b);   // NEW

int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;

    while ((opt = getopt(argc, argv, "lx")) != -1) {
        switch (opt) {
            case 'l':
                mode = LONG_MODE;
                break;
            case 'x':
                mode = HORIZONTAL_MODE;
                break;
            default:
                fprintf(stderr, "Usage: %s [-l | -x] [dir]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    const char *dir = ".";
    if (optind < argc)
        dir = argv[optind];

    do_ls(dir, mode);
    return 0;
}

/* --- Comparison Function for qsort() --- */
int compare_names(const void *a, const void *b) {
    const char *name1 = *(const char **)a;
    const char *name2 = *(const char **)b;
    return strcmp(name1, name2);
}

/* --- Core LS Logic --- */
void do_ls(const char *dir, enum DisplayMode mode) {
    int n = 0, maxlen = 0;
    char **names = read_dir_filenames(dir, &n, &maxlen);
    if (!names || n == 0) return;

    if (mode == LONG_MODE) {
        for (int i = 0; i < n; i++)
            display_long(dir, names[i]);
    } else if (mode == HORIZONTAL_MODE) {
        display_horizontal(names, n, maxlen);
    } else {
        display_columns_default(names, n, maxlen);
    }

    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

/* --- Long Listing --- */
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
    printf(" %-8s %-8s %8ld ", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?", fileStat.st_size);


    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf("%s %s\n", timebuf, filename);
}

/* --- Read Filenames + Sort --- */
char **read_dir_filenames(const char *dir, int *count, int *maxlen) {
    DIR *dp = opendir(dir);
    struct dirent *entry;
    if (!dp) return NULL;

    size_t capacity = 64;
    char **names = malloc(capacity * sizeof(char *));
    int n = 0, maxl = 0;

    while ((entry = readdir(dp)) != NULL) {
        if (entry->d_name[0] == '.') continue;
        int len = strlen(entry->d_name);
        if (len > maxl) maxl = len;
        if (n >= (int)capacity) {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char *));
        }
        names[n++] = strdup(entry->d_name);
    }
    closedir(dp);

    /* 🔹 Sort the filenames alphabetically using qsort() */
    qsort(names, n, sizeof(char *), compare_names);

    *count = n;
    *maxlen = maxl;
    return names;
}

/* --- Default (Down Then Across) --- */
void display_columns_default(char **names, int n, int maxlen) {
    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int cols = width / col_width;
    if (cols < 1) cols = 1;
    int rows = (n + cols - 1) / cols;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int idx = c * rows + r;
            if (idx >= n) continue;
            printf("%-*s", col_width, names[idx]);
        }
        printf("\n");
    }
}

/* --- Horizontal (-x) --- */
void display_horizontal(char **names, int n, int maxlen) {
    struct winsize w;
    int width = 80;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0)
        width = w.ws_col;

    int spacing = 2;
    int col_width = maxlen + spacing;
    int current_width = 0;

    for (int i = 0; i < n; i++) {
        if (current_width + col_width > width) {
            printf("\n");
            current_width = 0;
        }
        printf("%-*s", col_width, names[i]);
        current_width += col_width;
    }
    printf("\n");
}

