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
