feature-column-display-v1.2.0
REPORT.md – Feature 2
Question 1

What is the difference between stat() and lstat() system calls? When should lstat() be used in the ls command?

Answer:
The main difference is that stat() follows a symbolic link and gives details about the actual file it points to, while lstat() gives details about the link itself.
In the ls command, using lstat() is better because it helps show the information of symbolic links correctly, such as their type and permissions, without following them to their target files.

Question 2

The st_mode field in struct stat contains both file type and permission bits. How can bitwise operators and macros be used to extract this information?

Answer:
The st_mode field stores binary flags that describe what kind of file it is and what permissions it has.
We can use bitwise operators like & along with predefined macros to check these bits.

Example:

if (S_ISDIR(fileStat.st_mode))
    printf("This is a directory");

if (fileStat.st_mode & S_IRUSR)
    printf("Owner has read permission");


Here:
S_ISDIR checks if the file is a directory.

S_IRUSR, S_IWUSR, and S_IXUSR check if the owner has read, write, and execute permissions.

This method helps us interpret the file’s type and permissions from the st_mode value in a clear way.

Feature 3 — Column Display (down then across)

Q1 — Explain the logic for printing items in "down then across" columnar format. Why is a simple single loop insufficient?

Answer:
The "down then across" layout fills each column top-to-bottom before moving to the next column. To do that correctly you must know how many rows each column will have beforehand. That means you first read and store all filenames and find the longest filename. With the number of files and the number of columns (based on terminal width) you can compute the number of rows as rows = ceil(nfiles / cols). After this, to print row 0 you print names[0], names[rows], names[2*rows], ... For row 1 you print names[1], names[1+rows], etc. A single linear loop over filename array prints left-to-right across rows instead of down each column, so it won’t produce the desired ordering or alignment. Precomputing rows/columns is necessary.

Q2 — What is the purpose of ioctl here? What are limitations of using a fixed-width fallback?

Answer:
ioctl with TIOCGWINSZ lets the program ask the terminal how many columns wide it is. Using that real width allows our program to pack as many columns as fit and to reflow the output if the user resizes the terminal. If we always used a fixed width like 80 columns, the layout could look wrong on narrow or very wide terminals — either wrapping too early or leaving lots of unused space. Using ioctl makes the output adapt to the actual environment and gives a much better user experience.
