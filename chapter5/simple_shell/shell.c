#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <pwd.h>
#include <glob.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdbool.h>

#define JOBS_LIMIT 20
#define PATH_BUFFER_SIZE 1024
#define COMMAND_BUFFER_SIZE 1024
#define TOKEN_BUFFER_SIZE 64
#define TOKEN_DELIMITERS " \t\r\n\a"

#define BACKGROUND_EXECUTION 0
#define FOREGROUND_EXECUTION 1
#define PIPELINE_EXECUTION 2

#define COMMAND_EXTERNAL 0
#define COMMAND_EXIT 1
#define COMMAND_CD 2

#define STATUS_RUNNING 0
#define STATUS_DONE 1
#define STATUS_SUSPENDED 2
#define STATUS_CONTINUED 3
#define STATUS_TERMINATED 4

#define PROC_FILTER_ALL 0
#define PROC_FILTER_DONE 1
#define PROC_FILTER_REMAINING 2

#define COLOR_CYAN "\033[0;36m"
#define COLOR_NONE "\033[m"
#define COLOR_GRAY "\033[1;30m"

#define HISTORY_MAX_ITEMS 100

const char *STATUS_STRING[] = {
        "running",
        "done",
        "suspended",
        "continued",
        "terminated"
};

struct process {
    char *command;
    int argc;
    char **argv;
    char *input_path;
    char *output_path;
    pid_t pid;
    int type;
    int status;
    struct process *next;
};

struct job {
    int id;
    struct process *root;
    char *command;
    pid_t pgid;
    int mode;
};

struct shell_info {
    char user[TOKEN_BUFFER_SIZE];
    char cur_dir[PATH_BUFFER_SIZE];
    char pw_dir[PATH_BUFFER_SIZE];
    struct job *jobs[JOBS_LIMIT + 1];
};

static char **history;
static int history_index;
struct shell_info *shell;

char *trim_string(char *line) {
    char *head = line, *tail = line + strlen(line);

    while (*head == ' ') {
        head++;
    }
    while (*tail == ' ') {
        tail--;
    }
    *(tail + 1) = '\0';

    return head;
}

bool is_history_command(char *input) {
    const char *key = "history";

    if (strlen(input) < strlen(key))
        return 0;
    int i;

    for (i = 0; i < (int) strlen(key); i++) {
        if (input[i] != key[i])
            return false;
    }
    return true;
}

void add_to_history(char *input) {
    char *line;

    line = strdup(input);
    if (line == NULL)
        return;

    if (history_index == HISTORY_MAX_ITEMS) {
        free(history[0]);
        int space_to_move = sizeof(char *) * (HISTORY_MAX_ITEMS - 1);

        memmove(history, history + 1, space_to_move);
        if (history == NULL) {
            fprintf(stderr, "error: memory alloc error\n");
            return;
        }

        history_index--;
    }

    history[history_index++] = line;
    return;
}

void print_history() {
    int i;
    for (i = 0; i < history_index; i++) {
        printf("%d %s\n", i + 1, history[i]);
    }
}

int get_job_id_by_pid(int pid) {
    int i;
    struct process *proc;

    for (i = 1; i <= JOBS_LIMIT; i++) {
        if (shell->jobs[i] != NULL) {
            for (proc = shell->jobs[i]->root; proc != NULL; proc = proc->next) {
                if (proc->pid == pid) {
                    return i;
                }
            }
        }
    }

    return -1;
}

int get_proc_count(int id, int filter) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return -1;
    }

    int count = 0;
    struct process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        if (filter == PROC_FILTER_ALL ||
            (filter == PROC_FILTER_DONE && proc->status == STATUS_DONE) ||
            (filter == PROC_FILTER_REMAINING && proc->status != STATUS_DONE)) {
            count++;
        }
    }

    return count;
}

int get_next_job_id() {
    int i;

    for (i = 1; i <= JOBS_LIMIT; i++) {
        if (shell->jobs[i] == NULL) {
            return i;
        }
    }

    return -1;
}

void print_processes_of_job(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return;
    }

    printf("[%d]", id);

    struct process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        printf(" %d", proc->pid);
    }
    printf("\n");
}

void print_job_status(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return;
    }

    printf("[%d]", id);

    struct process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        printf("\t%d\t%s\t%s", proc->pid,
               STATUS_STRING[proc->status], proc->command);
        if (proc->next != NULL) {
            printf("|\n");
        } else {
            printf("\n");
        }
    }
}

void release_job(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return;
    }

    struct job *job = shell->jobs[id];
    struct process *proc, *tmp;
    for (proc = job->root; proc != NULL;) {
        tmp = proc->next;
        free(proc->command);
        free(proc->argv);
        free(proc->input_path);
        free(proc->output_path);
        free(proc);
        proc = tmp;
    }

    free(job->command);
    free(job);

    return;
}

int insert_job(struct job *job) {
    int id = get_next_job_id();

    if (id < 0) {
        return -1;
    }

    job->id = id;
    shell->jobs[id] = job;
    return id;
}

void remove_job(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return;
    }

    release_job(id);
    shell->jobs[id] = NULL;
}

bool is_job_completed(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return false;
    }

    struct process *proc;
    for (proc = shell->jobs[id]->root; proc != NULL; proc = proc->next) {
        if (proc->status != STATUS_DONE) {
            return false;
        }
    }

    return true;
}

int set_process_status(int pid, int status) {
    int i;
    struct process *proc;

    for (i = 1; i <= JOBS_LIMIT; i++) {
        if (shell->jobs[i] == NULL) {
            continue;
        }
        for (proc = shell->jobs[i]->root; proc != NULL; proc = proc->next) {
            if (proc->pid == pid) {
                proc->status = status;
                return 0;
            }
        }
    }

    return -1;
}

int wait_for_job(int id) {
    if (id > JOBS_LIMIT || shell->jobs[id] == NULL) {
        return -1;
    }

    int proc_count = get_proc_count(id, PROC_FILTER_REMAINING);
    int wait_pid, wait_count = 0;
    int status = 0;

    do {
        wait_pid = waitpid(-shell->jobs[id]->pgid, &status, WUNTRACED);
        wait_count++;

        if (WIFEXITED(status)) {
            set_process_status(wait_pid, STATUS_DONE);
        } else if (WIFSIGNALED(status)) {
            set_process_status(wait_pid, STATUS_TERMINATED);
        } else if (WSTOPSIG(status)) {
            status = -1;
            set_process_status(wait_pid, STATUS_SUSPENDED);
            if (wait_count == proc_count) {
                print_job_status(id);
            }
        }
    } while (wait_count < proc_count);

    return status;
}

int get_command_type(char *command) {
    if (strcmp(command, "exit") == 0) {
        return COMMAND_EXIT;
    } else if (strcmp(command, "cd") == 0) {
        return COMMAND_CD;
    } else {
        return COMMAND_EXTERNAL;
    }
}

void update_cwd_info() {
    getcwd(shell->cur_dir, sizeof(shell->cur_dir));
}

void change_dir(int argc, char **argv) {
    if (argc == 1) {
        chdir(shell->pw_dir);
        update_cwd_info();
        return;
    }

    if (chdir(argv[1]) == 0) {
        update_cwd_info();
        return;
    } else {
        printf("shell: cd %s: No such file or directory\n", argv[1]);
        return;
    }
}

void soft_exit() {
    printf("Goodbye!\n");
    exit(EXIT_SUCCESS);
}

void check_zombie() {
    int status, pid;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status)) {
            set_process_status(pid, STATUS_DONE);
        } else if (WIFSTOPPED(status)) {
            set_process_status(pid, STATUS_SUSPENDED);
        } else if (WIFCONTINUED(status)) {
            set_process_status(pid, STATUS_CONTINUED);
        }

        int job_id = get_job_id_by_pid(pid);
        if (job_id > 0 && is_job_completed(job_id)) {
            print_job_status(job_id);
            remove_job(job_id);
        }
    }
}

int launch_proc(struct job *job, struct process *proc, int in_fd, int out_fd, int mode) {
    proc->status = STATUS_RUNNING;

    if (proc->type != COMMAND_EXTERNAL) {
        switch (proc->type) {
            case COMMAND_EXIT:
                soft_exit();
                break;
            case COMMAND_CD:
                change_dir(proc->argc, proc->argv);
                break;
            default:
                break;
        }
        return 0;
    }

    pid_t child_pid = fork();
    int status = 0;

    if (child_pid < 0) {
        return -1;
    } else if (child_pid == 0) {

        proc->pid = getpid();
        if (job->pgid > 0) {
            setpgid(0, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(0, job->pgid);
        }

        if (in_fd != 0) {
            dup2(in_fd, 0);
            close(in_fd);
        }

        if (out_fd != 1) {
            dup2(out_fd, 1);
            close(out_fd);
        }

        if (execvp(proc->argv[0], proc->argv) < 0) {
            printf("shell: %s: command not found\n", proc->argv[0]);
            exit(EXIT_SUCCESS);
        }

        exit(EXIT_SUCCESS);
    } else {
        proc->pid = child_pid;
        if (job->pgid > 0) {
            setpgid(child_pid, job->pgid);
        } else {
            job->pgid = proc->pid;
            setpgid(child_pid, job->pgid);
        }

        if (mode == FOREGROUND_EXECUTION) {
            tcsetpgrp(0, job->pgid);
            status = wait_for_job(job->id);
            tcsetpgrp(0, getpid());
        }
    }

    return status;
}

void launch_job(struct job *job) {
    struct process *proc;
    int status = 0, in_fd = 0, fd[2], job_id = -1;

    check_zombie();
    if (job->root->type == COMMAND_EXTERNAL) {
        job_id = insert_job(job);
    }

    for (proc = job->root; proc != NULL; proc = proc->next) {
        if (proc == job->root && proc->input_path != NULL) {
            in_fd = open(proc->input_path, O_RDONLY);
            if (in_fd < 0) {
                printf("shell: no such file or directory: %s\n", proc->input_path);
                remove_job(job_id);
                return;
            }
        }
        if (proc->next != NULL) {
            pipe(fd);
            status = launch_proc(job, proc, in_fd, fd[1], PIPELINE_EXECUTION);
            close(fd[1]);
            in_fd = fd[0];
        } else {
            int out_fd = 1;
            if (proc->output_path != NULL) {
                out_fd = open(proc->output_path, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                if (out_fd < 0) {
                    out_fd = 1;
                }
            }
            status = launch_proc(job, proc, in_fd, out_fd, job->mode);
        }
    }

    if (job->root->type == COMMAND_EXTERNAL) {
        if (status >= 0 && job->mode == FOREGROUND_EXECUTION) {
            remove_job(job_id);
        } else if (job->mode == BACKGROUND_EXECUTION) {
            print_processes_of_job(job_id);
        }
    }
}

struct process *parse_command_segment(char *segment) {
    int bufsize = TOKEN_BUFFER_SIZE;
    int position = 0;
    char *command = strdup(segment);
    char *token;
    char **tokens = (char **) malloc(bufsize * sizeof(char *));

    token = strtok(segment, TOKEN_DELIMITERS);
    while (token != NULL) {
        glob_t glob_buffer;
        int glob_count = 0;
        if (strchr(token, '*') != NULL || strchr(token, '?') != NULL) {
            glob(token, 0, NULL, &glob_buffer);
            glob_count = glob_buffer.gl_pathc;
        }

        if (position + glob_count >= bufsize) {
            fprintf(stderr, "shell: buffer overflow\n");
            exit(EXIT_FAILURE);
        }

        if (glob_count > 0) {
            int i;
            for (i = 0; i < glob_count; i++) {
                tokens[position++] = strdup(glob_buffer.gl_pathv[i]);
            }
            globfree(&glob_buffer);
        } else {
            tokens[position] = token;
            position++;
        }

        token = strtok(NULL, TOKEN_DELIMITERS);
    }

    int i = 0, argc = 0;
    char *input_path = NULL, *output_path = NULL;
    while (i < position) {
        if (tokens[i][0] == '<' || tokens[i][0] == '>') {
            break;
        }
        i++;
    }
    argc = i;

    for (; i < position; i++) {
        if (tokens[i][0] == '<') {
            if (strlen(tokens[i]) == 1) {
                input_path = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(input_path, tokens[i + 1]);
                i++;
            } else {
                input_path = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(input_path, tokens[i] + 1);
            }
        } else if (tokens[i][0] == '>') {
            if (strlen(tokens[i]) == 1) {
                output_path = (char *) malloc((strlen(tokens[i + 1]) + 1) * sizeof(char));
                strcpy(output_path, tokens[i + 1]);
                i++;
            } else {
                output_path = (char *) malloc(strlen(tokens[i]) * sizeof(char));
                strcpy(output_path, tokens[i] + 1);
            }
        } else {
            break;
        }
    }

    for (i = argc; i <= position; i++) {
        tokens[i] = NULL;
    }

    struct process *new_proc = (struct process *) malloc(sizeof(struct process));
    new_proc->command = command;
    new_proc->argv = tokens;
    new_proc->argc = argc;
    new_proc->input_path = input_path;
    new_proc->output_path = output_path;
    new_proc->pid = -1;
    new_proc->type = get_command_type(tokens[0]);
    new_proc->next = NULL;
    return new_proc;
}

struct job *parse_command(char *line) {
    line = trim_string(line);
    char *command = strdup(line);

    struct process *root_proc = NULL, *proc = NULL;
    char *line_cursor = line, *c = line, *seg;
    int seg_len = 0, mode = FOREGROUND_EXECUTION;

    if (line[strlen(line) - 1] == '&') {
        mode = BACKGROUND_EXECUTION;
        line[strlen(line) - 1] = '\0';
    }

    while (true) {
        if (*c == '\0' || *c == '|') {
            seg = (char *) malloc((seg_len + 1) * sizeof(char));
            strncpy(seg, line_cursor, seg_len);
            seg[seg_len] = '\0';

            struct process *new_proc = parse_command_segment(seg);
            if (!root_proc) {
                root_proc = new_proc;
                proc = root_proc;
            } else {
                proc->next = new_proc;
                proc = new_proc;
            }

            if (*c != '\0') {
                line_cursor = c;
                while (*(++line_cursor) == ' ');
                c = line_cursor;
                seg_len = 0;
                continue;
            } else {
                break;
            }
        } else {
            seg_len++;
            c++;
        }
    }

    struct job *new_job = (struct job *) malloc(sizeof(struct job));
    new_job->root = root_proc;
    new_job->command = command;
    new_job->pgid = -1;
    new_job->mode = mode;
    return new_job;
}

char *read_line() {
    int bufsize = COMMAND_BUFFER_SIZE;
    int position = 0;
    char *buffer = malloc(sizeof(char) * bufsize);
    int c;

    while (true) {
        c = getchar();

        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        if (position >= bufsize) {
            fprintf(stderr, "shell: Command too long.\n");
            exit(EXIT_FAILURE);
        }
    }
}

void shell_print_prompt() {
    printf(COLOR_CYAN "%s" COLOR_NONE ": %s$ " COLOR_GRAY, shell->user, shell->cur_dir);
}

void shell_loop() {
    char *line;
    struct job *job;

    while (true) {
        shell_print_prompt();
        line = read_line();

        if (strlen(line) == 0) {
            check_zombie();
            continue;
        } else if (is_history_command(line)) {
            print_history();
            add_to_history(line);
            continue;
        }

        job = parse_command(line);
        launch_job(job);
        add_to_history(line);
    }
}

void shell_init() {
    shell = (struct shell_info *) malloc(sizeof(struct shell_info));
    getlogin_r(shell->user, sizeof(shell->user));

    struct passwd *pw = getpwuid(getuid());
    strcpy(shell->pw_dir, pw->pw_dir);

    int i;
    for (i = 0; i < JOBS_LIMIT; i++) {
        shell->jobs[i] = NULL;
    }

    update_cwd_info();

    history = (char **) malloc(sizeof(char *) * HISTORY_MAX_ITEMS);
    if (history == NULL) {
        fprintf(stderr, "error: memory alloc error\n");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char **argv) {
    shell_init();
    shell_loop();

    return EXIT_SUCCESS;
}
