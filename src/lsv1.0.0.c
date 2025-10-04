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

// --- Display Modes ---
enum DisplayMode {
    DEFAULT_MODE,       // down then across
    LONG_MODE,          // -l
    HORIZONTAL_MODE     // -x
};

// --- Function Prototypes ---
void do_ls(const char *dir, enum DisplayMode mode);
void display_long(const char *path, const char *filename);
void display_columns_default(const char *dir);
void display_horizontal(const char *dir);
char **read_dir_filenames(const char *dir, int *count, int *maxlen);

// --- MAIN FUNCTION ---
int main(int argc, char *argv[]) {
    int opt;
    enum DisplayMode mode = DEFAULT_MODE;

    // Parse command-line arguments (-l, -x)
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

// --- MAIN LOGIC HANDLER ---
void do_ls(const char *dir, enum DisplayMode mode) {
    if (mode == LONG_MODE) {
        DIR *dp = opendir(dir);
        struct dirent *entry;
        if (!dp) {
            fprintf(stderr, "Cannot open directory: %s\n", dir);
            return;
        }
        while ((entry = readdir(dp)) != NULL) {
            if (entry->d_name[0] == '.')
                continue;
            display_long(dir, entry->d_name);
        }
        closedir(dp);
    } 
    else if (mode == HORIZONTAL_MODE) {
        display_horizontal(dir);
    } 
    else {
        display_columns_default(dir);
    }
}

// --- LONG LISTING FORMAT (-l) ---
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

    // File type
    printf((S_ISDIR(fileStat.st_mode)) ? "d" : "-");
    // Permissions
    printf((fileStat.st_mode & S_IRUSR) ? "r" : "-");
    printf((fileStat.st_mode & S_IWUSR) ? "w" : "-");
    printf((fileStat.st_mode & S_IXUSR) ? "x" : "-");
    printf((fileStat.st_mode & S_IRGRP) ? "r" : "-");
    printf((fileStat.st_mode & S_IWGRP) ? "w" : "-");
    printf((fileStat.st_mode & S_IXGRP) ? "x" : "-");
    printf((fileStat.st_mode & S_IROTH) ? "r" : "-");
    printf((fileStat.st_mode & S_IWOTH) ? "w" : "-");
    printf((fileStat.st_mode & S_IXOTH) ? "x" : "-");

    // Links
    printf(" %ld", fileStat.st_nlink);

    // User and group names
    pw = getpwuid(fileStat.st_uid);
    gr = getgrgid(fileStat.st_gid);
    printf(" %-8s %-8s", pw ? pw->pw_name : "?", gr ? gr->gr_name : "?");

    // Size
    printf(" %8ld ", fileStat.st_size);

    // Time
    char timebuf[80];
    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", localtime(&fileStat.st_mtime));
    printf("%s ", timebuf);

    // Filename
    printf("%s\n", filename);
}

// --- READ FILENAMES INTO ARRAY ---
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
    *count = n;
    *maxlen = maxl;
    return names;
}

// --- DEFAULT: DOWN-THEN-ACROSS DISPLAY ---
void display_columns_default(const char *dir) {
    int n = 0, maxlen = 0;
    char **names = read_dir_filenames(dir, &n, &maxlen);
    if (!names || n == 0) return;

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

    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

// --- HORIZONTAL (-x) DISPLAY ---
void display_horizontal(const char *dir) {
    int n = 0, maxlen = 0;
    char **names = read_dir_filenames(dir, &n, &maxlen);
    if (!names || n == 0) return;

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

    for (int i = 0; i < n; i++) free(names[i]);
    free(names);
}

