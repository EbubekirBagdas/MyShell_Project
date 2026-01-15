#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>

#define MAX_LINE 80
#define MAX_ALIASES 10

// --- ALIAS STRUCTURE ---
struct Alias {
    char name[32];      // The alias name (e.g., "list")
    char command[128];  // The actual command (e.g., "ls -l")
    int used;           // Flag to check if slot is used
};

struct Alias aliases[MAX_ALIASES]; // Array to store aliases
pid_t foreground_pid = -1;

/* Signal Handler for Ctrl+Z */
void handle_signal(int signo) {
    if (signo == SIGTSTP) {
        if (foreground_pid > 0) {
            printf("\n[LOG] Ctrl+Z caught. Terminating foreground process %d...\n", foreground_pid);
            kill(foreground_pid, SIGKILL);
            // We do NOT reset foreground_pid here immediately.
            // Let the waitpid loop in main handle the cleanup.
        } else {
            printf("\n"); // Just print a newline for visual cleaniness
        }
    }
}

void setup(char inputBuffer[], char *args[], int *background)
{
    int length, i, start, ct;
    ct = 0;
    length = read(STDIN_FILENO, inputBuffer, MAX_LINE);
    start = -1;
    if (length == 0) exit(0);
    if ((length < 0) && (errno != EINTR)) {
        perror("Error reading the command");
        exit(-1);
    }
    for (i = 0; i < length; i++) {
        switch (inputBuffer[i]) {
        case ' ':
        case '\t':
            if (start != -1) { args[ct] = &inputBuffer[start]; ct++; }
            inputBuffer[i] = '\0'; start = -1; break;
        case '\n':
            if (start != -1) { args[ct] = &inputBuffer[start]; ct++; }
            inputBuffer[i] = '\0'; args[ct] = NULL; break;
        default:
            if (start == -1) start = i;
            if (inputBuffer[i] == '&') {
                *background = 1; inputBuffer[i - 1] = '\0';
            }
        }
    }
    args[ct] = NULL;
}

char *search_path(char *command) {
    char *path_env = getenv("PATH");
    if (path_env == NULL) return NULL;
    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");
    static char full_path[1024];
    while (dir != NULL) {
        sprintf(full_path, "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            free(path_copy); return full_path;
        }
        dir = strtok(NULL, ":");
    }
    free(path_copy); return NULL;
}

// --- ALIAS FUNCTIONS ---
void add_alias(char *command_part, char *name_part) {
    int i;
    // Remove quotes from command if they exist (e.g., "ls -l" -> ls -l)
    if (command_part[0] == '"') {
        memmove(command_part, command_part + 1, strlen(command_part)); // Remove first char
        command_part[strlen(command_part) - 1] = '\0'; // Remove last char
    }

    for (i = 0; i < MAX_ALIASES; i++) {
        if (!aliases[i].used) {
            strcpy(aliases[i].command, command_part);
            strcpy(aliases[i].name, name_part);
            aliases[i].used = 1;
            return;
        }
    }
    printf("Alias limit reached!\n");
}

void print_aliases() {
    int i;
    for (i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used) {
            printf("%s \"%s\"\n", aliases[i].name, aliases[i].command);
        }
    }
}

void remove_alias(char *name) {
    int i;
    for (i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used && strcmp(aliases[i].name, name) == 0) {
            aliases[i].used = 0;
            return;
        }
    }
    printf("Alias not found.\n");
}

int check_alias(char *args[]) {
    int i;
    for (i = 0; i < MAX_ALIASES; i++) {
        if (aliases[i].used && strcmp(aliases[i].name, args[0]) == 0) {
            // Alias found! We need to tokenize the command stored in alias
            // and replace args.
            // WARNING: This is a simple implementation. It parses the stored command by space.
            char *cmd_copy = strdup(aliases[i].command);
            int j = 0;
            char *token = strtok(cmd_copy, " ");
            while (token != NULL) {
                args[j++] = token;
                token = strtok(NULL, " ");
            }
            args[j] = NULL;
            // Note: cmd_copy is leaked here for simplicity, in a real shell we'd manage memory better
            return 1; // Alias applied
        }
    }
    return 0; // No alias found
}

int main(void)
{
    char inputBuffer[MAX_LINE];
    int background;
    char *args[MAX_LINE / 2 + 1];

    // Clear aliases array
    memset(aliases, 0, sizeof(aliases));

    struct sigaction sa;
    sa.sa_handler = handle_signal;
    sa.sa_flags = 0; 
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGTSTP, &sa, NULL) == -1) {
        perror("Signal handler error"); exit(1);
    }
    
    while (1) {
        // Zombie Reaper
        while (waitpid(-1, NULL, WNOHANG) > 0);

        background = 0;
        printf("myshell: ");
        fflush(stdout);
        
        setup(inputBuffer, args, &background);

        if (args[0] == NULL) continue;

        // Clean '&' from args
        int k = 0;
        while(args[k] != NULL) k++;
        if (k > 0 && strcmp(args[k-1], "&") == 0) {
            args[k-1] = NULL; background = 1;
        }

        // --- INTERNAL COMMANDS ---
        if (strcmp(args[0], "exit") == 0) {
            // Check for running background processes could be added here
            printf("Shell exiting...\n");
            exit(0);
        }
        
        if (strcmp(args[0], "alias") == 0) {
            if (args[1] == NULL) {
                print_aliases(); // Just "alias" command usually lists them too
            } else if (strcmp(args[1], "-l") == 0) {
                print_aliases();
            } else {
                // Parsing logic for: alias "ls -l" list
                // Since setup splits by space, "ls -l" might be split into multiple args
                // We need to reconstruct it if quotes are present.
                char command_buffer[128] = "";
                int arg_idx = 1;
                
                // If it starts with quote
                if (args[arg_idx][0] == '"') {
                    while (args[arg_idx] != NULL) {
                        strcat(command_buffer, args[arg_idx]);
                        int len = strlen(args[arg_idx]);
                        if (args[arg_idx][len-1] == '"') {
                            arg_idx++;
                            break; // End of quoted string
                        }
                        strcat(command_buffer, " ");
                        arg_idx++;
                    }
                } else {
                    strcpy(command_buffer, args[arg_idx]); // No quotes, single word command
                    arg_idx++;
                }
                
                if (args[arg_idx] != NULL) {
                    add_alias(command_buffer, args[arg_idx]); // args[arg_idx] is the nickname
                } else {
                    printf("Usage: alias \"command\" name\n");
                }
            }
            continue; // Skip fork
        }

        if (strcmp(args[0], "unalias") == 0) {
            if (args[1] != NULL) remove_alias(args[1]);
            continue;
        }
        
        if (strcmp(args[0], "fg") == 0) {
            // Project requirement B: fg %num
            // Since we don't fully track background PIDs in a list in this simple version,
            // we can't implement full 'fg' without a process list structure.
            // For now, let's just print not implemented or ignoring.
            printf("fg command is a placeholder in this version.\n");
            continue;
        }

        // Check if the command is an ALIAS and replace args if so
        check_alias(args);

        // --- FORK & EXEC ---
        pid_t pid = fork();

        if (pid < 0) {
            perror("Fork failed");
        }
        else if (pid == 0) {
            // --- CHILD PROCESS ---
            // Signal handling: Restore default behavior for child so it can be stopped/killed normally
            // struct sigaction dfl;
            // dfl.sa_handler = SIG_DFL;
            // sigaction(SIGTSTP, &dfl, NULL);

            char *args_clean[MAX_LINE / 2 + 1];
            int i = 0, j = 0;
            while (args[i] != NULL) {
                if (strcmp(args[i], ">") == 0) {
                    if (args[i+1] == NULL) { fprintf(stderr, "Syntax Error\n"); exit(1); }
                    int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDOUT_FILENO); close(fd); i += 2;
                } else if (strcmp(args[i], ">>") == 0) {
                    if (args[i+1] == NULL) { fprintf(stderr, "Syntax Error\n"); exit(1); }
                    int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    dup2(fd, STDOUT_FILENO); close(fd); i += 2;
                } else if (strcmp(args[i], "<") == 0) {
                    if (args[i+1] == NULL) { fprintf(stderr, "Syntax Error\n"); exit(1); }
                    int fd = open(args[i+1], O_RDONLY);
                    dup2(fd, STDIN_FILENO); close(fd); i += 2;
                } else if (strcmp(args[i], "2>") == 0) {
                    if (args[i+1] == NULL) { fprintf(stderr, "Syntax Error\n"); exit(1); }
                    int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    dup2(fd, STDERR_FILENO); close(fd); i += 2;
                } else {
                    args_clean[j++] = args[i++];
                }
            }
            args_clean[j] = NULL;

            char *command_path = search_path(args_clean[0]);
            if (command_path == NULL) command_path = args_clean[0];

            execv(command_path, args_clean);
            perror("Command execution failed");
            exit(1);
        }
        else {
            // --- PARENT PROCESS ---
            
            if (background == 0) {
                foreground_pid = pid; 
                // FIX: Loop waitpid to handle signal interruption (EINTR)
                int status;
                while (waitpid(pid, &status, 0) == -1) {
                    if (errno != EINTR) {
                        perror("waitpid failed");
                        break;
                    }
                    // If EINTR (interrupted by Ctrl+Z signal handler), loop again
                    // to make sure we reap the zombie.
                }
                foreground_pid = -1; 
            } else {
                printf("[Background Process Running] PID: %d\n", pid);
            }
        }
    }
}
