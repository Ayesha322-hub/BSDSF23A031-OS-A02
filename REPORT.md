feature-long-listing-v1.1.0
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

Feature 4 — Horizontal Column Display (-x Option)

Q1: Compare the implementation complexity of the “down then across” format and the “across” (horizontal) format. Which one needs more pre-calculation, and why?

The “down then across” format is harder to build because it first needs to figure out how many rows and columns can fit in the terminal.
To do that, I had to find the longest filename, calculate the width of each column, and then use formulas like c * rows + r to print items in order.
The horizontal “across” display is much simpler. It just prints one name after another, and whenever the next name would cross the terminal width, it starts a new line.
So the “down then across” method needs more pre-calculation and control over layout.

Q2: Describe the strategy you used in your code to manage the different display modes (-l, -x, and default). How did your program decide which function to call for printing?

I used an enum variable called display_mode to keep track of which mode is selected.
When the program starts, it checks the command-line options using getopt().
If the user gives -l, it sets the mode to LONG_MODE; if -x is given, it sets it to HORIZONTAL_MODE.
If no option is provided, it stays as the default mode.
Later, in the do_ls() function, the program checks this flag and calls the matching display function:

display_long() for -l

display_horizontal() for -x

display_columns_default() for normal output
This made the structure simple and easy to extend for future features.

S_IRUSR, S_IWUSR, and S_IXUSR check if the owner has read, write, and execute permissions.

This method helps us interpret the file’s type and permissions from the st_mode value in a clear way.

