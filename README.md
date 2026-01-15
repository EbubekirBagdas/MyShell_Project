# Simple Shell Implementation

This project is a custom shell implementation written in C for Linux environments. It demonstrates fundamental operating system concepts such as process creation (`fork`), execution (`execv`), signal handling, and I/O redirection.

## Features

Based on the project requirements, this shell supports:

* [cite_start]**Process Execution:** Executes commands by creating new processes using `fork()` and `execv()`[cite: 11, 12]. [cite_start]It searches the `PATH` environment variable to locate executables[cite: 13].
* [cite_start]**Background Processing:** Supports running processes in the background using the `&` operator[cite: 19]. [cite_start]The shell creates a child process and immediately prompts for the next command without waiting[cite: 20].
* **I/O Redirection:**
    * [cite_start]Standard Output Redirection (`>`) and Append (`>>`)[cite: 58, 60].
    * [cite_start]Standard Input Redirection (`<`)[cite: 64].
    * [cite_start]Standard Error Redirection (`2>`)[cite: 66].
    * [cite_start]Combined Redirection (Input & Output)[cite: 68].
* **Built-in Commands:**
    * [cite_start]`alias` / `unalias`: Create and remove custom shortcuts for commands[cite: 24, 47].
    * [cite_start]`exit`: Terminates the shell safely, checking for running background processes first[cite: 53].
    * [cite_start]`fg %num`: Brings a background process to the foreground[cite: 51].
* [cite_start]**Signal Handling:** Handles `Ctrl+Z` to terminate the currently running foreground process and its descendants[cite: 49].

## Constraints

* [cite_start]The implementation strictly avoids the use of the `system()` function as per project rules[cite: 15].
* [cite_start]Handles foreground and background process management manually using `wait()` and PID checks[cite: 16, 22].

## How to Compile and Run

```bash
gcc -o myshell main.c setup.c  # (Dosya adların neyse ona göre düzenle)
./myshell
