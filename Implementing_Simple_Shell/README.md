## Project #1: My Amazing Shell

### *** Due on 24:00, April 12 (Friday)***

### Goal
With the system calls learned in the class and a few additional ones, you are ready to control processes in the system.
Let's build my amazing shell with those system calls.


### Background
- *Shell* is a program that gets inputs from users, interpretes the inputs, and processes them accordingly. Command Prompt in Windows, Bourne Shell (`bash`) in Linux, and zsh in macOS are the examples of shells.

- An user can enter a command by writing a sentence on the shell and press the "Enter" key. Upon receiving the input, the shell parses the requests into command *tokens*, and processes the request according to the first token (i.e., `tokens[0]`).

- The shell *always* assumes that the first token is the filename of the executable to run **EXCEPT FOR** `exit`, `cd`, and `alias` (see belows for the descriptions for those). The shell executes the executable with the rest of the tokens as the arguments. For example, if an user inputs `ls -al /home/sce213`, the shell will execute `ls` executable with `-al` and `/home/sce213` as its arguments.


### Problem Specification
- The shell program `mash` (**M**y **A**mazing **SH**ell) awaits your command line input after printing out "$" as the prompt. When you enter a line of command the framework tokenizes the command with `parse_command()` and calls `run_command()` with the tokens. Implement following features starting in `run_command()`.

- Currently, the shell keeps getting input commands and processes them until the user enters `exit`. In that case, the shell program exits.


#### Execute external commands (50 pts)
- When the shell gets a command, it should **run the executable** as explained above. Each command can be comprised of one exeutable followed by zero or more arguments. For example;

  ```bash
  $ /bin/ls   # 0 argument
  list_head.h  Makefile  pa1.c  parser.c  parser.h  README.md  types.h
  $ ls        # 0 argument
  list_head.h  Makefile  pa1.c  parser.c  parser.h  README.md  types.h
  $ pwd       # 0 argument
  /home/sanghoon/os/pa1
  $ cp pa1.c pa1-backup.c   # two arguments
  $ ls
  list_head.h  Makefile  pa1-backup.c  pa1.c  parser.c  parser.h  README.md  types.h
  $ exit
  ```
- The shell may execute the executable using the p-variant of the `exec()` system call family so that the executable file is automatically located from the *`$PATH`* environment variable.

- The shell should print the prompt only after the executed process is exited.

- Your task is to **EXECUTE** external executables (such as `ls`, `pwd`, and `cp`), **NOT to implement** the features of the commands.

- Use the `toy` program which is included in the handout code for your development and testing. It simply prints out the arguments it receives, so you can check whether your implementation handles input commands properly.

  ```bash
  $ ./toy arg1 arg2 arg3 arg4
  pid  = xxxxxx
  argc = 5
  argv[0] = ./toy
  argv[1] = arg1
  argv[2] = arg2
  argv[3] = arg3
  argv[4] = arg4
  done!
  ```

- Hint: `fork(2), exec(3), wait(2), waitpid(2)`


#### Change working directory (20 pts)
- Imagine when you browse folders with the Explorer. When you select the "New Folder" item in the menu, a new folder will be created in the currently viewing folder. You can change the current folder by selecting one of folders in the directory.

- The shell has the similar concept called *current working directory*. The shell is treated as running in the current working directory and the location of files is computed from the current working directory. You can check the current working directory with `/bin/pwd` command.

- Implement `cd`, a special command manipulating the current working directory. This command is special in that this feature is not handled by executing executables but the shell understands the command and processes itself. In this sense, this type of command is called a *built-in command*.

- Each user has one's *home directory* which is denoted by `~`. The actual path is defined in `$HOME` environment variable. Make `cd` command to understand it

  ```bash
  $ pwd
  /home/directory/of/your/account  # Assume this is the home directory of the user
  $ cd /somewhere/i/dont/know
  $ pwd
  /somewhere/i/dont/know
  $ cd ~
  $ pwd
  /home/directory/of/your/account
  $ cd /somewhere/i/know
  $ pwd
  /somewhere/i/know
  $ cd   # Equivalent to cd ~
  $ pwd
  /home/directory/of/your/account
  ```

- Hints: `chdir(2), getenv(3), environ(7)`


#### Command aliasing (100 pts)
- It is very boring and error-prone to enter the long command whenever you need to ask the shell to do something repeatedly. For example, one of the most frequently used command is `ls -al`. It would be awesome if we can define an *alias* `ll` for `ls -al` and the shell does `ls -al` when we enter `ll` only!!

- So, your task is to implement the `alias` command. Like `cd`, the `alias` is a built-in command processed by the shell itself rather than executing a program.

- You can define an alias for a word as follows:
  ```bash
  $ ls -al              # The full command
  list_head.h  Makefile  pa1.c  parser.c  parser.h  README.md  types.h
  $ alias ll ls -al     # Define an alias for ll
  $ ll                  # The shell runs ls -al
  list_head.h  Makefile  pa1.c  parser.c  parser.h  README.md  types.h
  $ alias xyz echo Hello world  # Define xyz to echo Hello world
  $ xyz
  Hello world
  $ alias zzz operating systems
  $ echo SCE213 zzz PA1 # Process the keyword in the middle of command
  SCE213 operating systems PA1
  ```
- The shell should be able to process an unlimited number of aliases. This implies you need to use a list to maintain the defined aliases.

- When you enter `alias` without any following arguments, the shell should list up currently defined aliases. The alias defined earlier should come before later ones. You should print the aliases into `stderr` to get graded properly. Also handle spaces carefully so that there is no trailing space.
  ```bash
  $ alias
  ll: ls -al
  xyz: echo Hello world
  zzz: operating systems
  ```

- The word in the translated with alias should not be translated again with another alias.
  ```bash
  $ alias xyz Hello world
  $ alias world Korea
  $ echo xyz
  Hello world
  $ echo world
  Korea
  $ echo xyz world
  Hello world Korea   # 'world' is translated from xyz so it is not translated again to Korea
  ```

- (Added 3/29 3pm) Don't worry about alias redefining, PASS would not check the case.


- Hint
  - If `tokens[]` were a list of words, you might be able to implement the feature easily by replacing a word with multiple words.
  - Take care of the string in `tokens[]` which are allocated and deallocated in the functions in `parser.c`. It is advised to have a look at them before fiddling with `tokens[]`.


#### Connect two processes with a pipe (100 pts)
- As we discussed in the class, we can connect two processes with an ordinary pipe. Implement this feature.

- You may enter two commands with the pipe symbol `|` in between. All output of the first command should be carried to the input of the second command.

  ```bash
  $ cat pa1.c | sort -n
  $ echo hello_my_cruel_world | cut -c2-5
  ```

- Note that the shell should be *sane* after processing the pipe.

- Aliases should be applied to the both commands before and after the pipe symbol.

- You may assume that bulit-in commands are not used when using the pipe; only external commands will be used for using the pipe in this PA.

- (Added 4/2 9pm) Use the newly included `pipe` toy to test whether your implementation handles pipe properly or not.

- Hints
  - `pipe(2)` and `dup2(2)`.
  - Implement incrementally. First check whether the pipe symbol exists in the tokens. If not, just do execute the command. If exists, split the tokens into two pars and feed them to **two** different processes which are connected through a pipe.
  - You will *not be able to* implement this feature by manually getting the output of the first process in a buffer and sending it to the second process. Check the sample code in the lecture slide.


### Restriction and hints
- For your coding practice, the compiler is set to halt on some (important) warnings. Write your code to fully comply the C99 standard.
- You can define/change edit whatever you want in `pa1.c`. Also you may leave `initialize()` and `finalizne()` blank if you don't need them.
- You might not use some or all of the hinted system calls if you don't want to use them.
- DO NOT DO FOLLOWINGS. You will get 0 pts if you don't follow the instructions.
  - Do not use `system()` system call.
  - Do not implement external programs' features by yourself (e.g., printing out a message to handle `echo` command, listing the current directory to handle `ls` command, etc).
  - Do not make files to implement the pipe feature. Also, it is not valid to use a temporary buffer to convey the output of the first process to next one.
- It is advised to test your code on your computer first and to implement incrementally. Some sample inputs are included under `testcases` directory. Try to input each line or just run `./mash < [input file]`.
- FYI, the instructor's implementation took ~250 lines of C code. Believe me, the implementation is not difficult if you fully understand the concepts of processes.


### Submission / Grading
- 310 pts in total
- Source: ***pa1.c*** (270 pts in total)
  - You can submit up to **30** times to PASS.
  - Points will be prorated by testcase results.
- Document: ***One PDF document*** (30 pts) that includes **ALL** the followings:
  - Outline how programs are launched and arguments are passed.
  - Explain your approach to implement the aliasing feature.
  - Your strategy to implement the pipe, including how many forks did you used and for what.
  - AND lessons learned
  - Focus on explaining your idea and approaches, not reading your code.
- The document should comply following restrictions:
  - No more than ***four*** pages
  - Should include all items listed above
  - Do not include cover page, your name, nor student id
  - Do not include any code nor screenshot on the document

- Git repository URL at git.ajou.ac.kr (10 pts)
  - To get the points, you should actually use the repository to manage your code.
  - Check the instructions and restrictions in the class.
    - Maintain the repository in Ajou Gitlab ([https://git.ajou.ac.kr](https://git.ajou.ac.kr)).
    - The repository is properly *cloned*.
    - Have more than two commits which are hours aparts.
    - You will not get any point if you just committed your final code.
  - How to submit your git repository
    - Generate a deploy token from Settings/Repository/Deploy Token. Make sure you're working with deploy **token** not deploy **key**.
      - For the slip token policy, the grading will happen after the deadline. Thus, the token should be valid through the due date + 4 days.
    - Register at PASS using the repository URL and deploy token.
    - PASS only accepts the repository address over HTTP. **SSH URL will be rejected**.

- **YOU MIGHT NOT GET AN ANSWER IF THE ISSUE/TOPIC IS ALREADY DISCUSSED ON THIS HANDOUT**.
- **QUESTIONS OVER EMAIL WILL BE IGNORED unless it concerns your privacy**.
