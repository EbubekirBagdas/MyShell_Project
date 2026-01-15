# Simple Shell Implementation

This project is a custom shell implementation written in C for Linux environments. It demonstrates fundamental operating system concepts such as process creation (`fork`), execution (`execv`), signal handling, and I/O redirection.

## Features

This shell supports the following functionalities:

* **Process Execution:** Executes commands by creating new processes using `fork()` and `execv()`. It searches the `PATH` environment variable to locate executables.
* **Background Processing:** Supports running processes in the background using the `&` operator. The shell creates a child process and immediately prompts for the next command without waiting.
* **I/O Redirection:**
    * Standard Output Redirection (`>`) and Append (`>>`).
    * Standard Input Redirection (`<`).
    * Standard Error Redirection (`2>`).
    * Combined Redirection (Input & Output).
* **Built-in Commands:**
    * `alias` / `unalias`: Create and remove custom shortcuts for commands.
    * `exit`: Terminates the shell safely, checking for running background processes first.
    * `fg %num`: Brings a background process to the foreground.
* **Signal Handling:** Handles `Ctrl+Z` to terminate the currently running foreground process and its descendants.

## Constraints

* The implementation strictly avoids the use of the `system()` function.
* Handles foreground and background process management manually using `wait()` and PID checks.

## How to Compile and Run

```bash
# Compile the program
gcc myshell.c -o myshell

# Run the shell
./myshell
