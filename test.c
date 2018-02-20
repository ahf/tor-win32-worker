#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

#include <event2/event.h>

#define UNUSED(x) ((void)(x))

typedef struct process_handle_t {
#ifdef _WIN32
    HANDLE stdin_pipe;
    HANDLE stdout_pipe;
    HANDLE stderr_pipe;

    PROCESS_INFORMATION pid;
#else
    int stdin_pipe;
    int stdout_pipe;
    int stderr_pipe;

    pid_t pid;
#endif
} process_handle_t;

#ifdef _WIN32
void spawn_process_win32(const char *filename, const char *argv[], process_handle_t **process_handle_out)
{
    HANDLE stdin_pipe_read = NULL;
    HANDLE stdin_pipe_write = NULL;
    HANDLE stdout_pipe_read = NULL;
    HANDLE stdout_pipe_write = NULL;
    HANDLE stderr_pipe_read = NULL;
    HANDLE stderr_pipe_write = NULL;

    // STARTUPINFOA siStartInfo;

    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    // Setup stdout pipe.
    if (! CreatePipe(&stdout_pipe_read, &stdout_pipe_write, &saAttr, 0)) {
        fprintf(stderr, "Unable to create pipe for stdout.\n");
        return;
    }

    if (! SetHandleInformation(stdout_pipe_read, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Unable to configure stdout pipe.\n");
        return;
    }

    // Setup stderr pipe.
    if (! CreatePipe(&stderr_pipe_read, &stderr_pipe_write, &saAttr, 0)) {
        fprintf(stderr, "Unable to create pipe for stderr.\n");
        return;
    }

    if (! SetHandleInformation(stderr_pipe_read, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Unable to configure stderr pipe.\n");
        return;
    }

    // Setup stdin pipe.
    if (! CreatePipe(&stdin_pipe_read, &stdin_pipe_write, &saAttr, 0)) {
        fprintf(stderr, "Unable to create pipe for stdin.\n");
        return;
    }

    if (! SetHandleInformation(stdin_pipe_read, HANDLE_FLAG_INHERIT, 0)) {
        fprintf(stderr, "Unable to configure stdin pipe.\n");
        return;
    }
}
#else
void spawn_process_unix(const char *filename, const char *argv[], process_handle_t **process_handle_out)
{
    int stdout_pipe[2];
    int stderr_pipe[2];
    int stdin_pipe[2];
    pid_t pid;

    if (pipe(stdout_pipe) == -1) {
        fprintf(stderr, "Unable to create pipe for stdout.\n");
        return;
    }

    if (pipe(stderr_pipe) == -1) {
        fprintf(stderr, "Unable to create pipe for stderr.\n");

        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        return;
    }

    if (pipe(stdin_pipe) == -1) {
        fprintf(stderr, "Unable to create pipe for stdin.\n");

        close(stdout_pipe[0]);
        close(stdout_pipe[1]);

        close(stderr_pipe[0]);
        close(stderr_pipe[1]);

        return;
    }

    pid = fork();

    if (pid == 0) {
        // In the child process.

        if (dup2(stdout_pipe[1], STDOUT_FILENO)) {
            fprintf(stderr, "dup2(..., STDOUT_FILENO) failed.\n");
            return;
        }

        if (dup2(stderr_pipe[1], STDERR_FILENO)) {
            fprintf(stderr, "dup2(..., STDERR_FILENO) failed.\n");
            return;
        }

        if (dup2(stdin_pipe[0], STDIN_FILENO)) {
            fprintf(stderr, "dup2(..., STDIN_FILENO) failed.\n");
            return;
        }
    }
}
#endif

void spawn_process(const char *filename, const char *argv[], process_handle_t **process_handle_out)
{
    assert(filename);
    assert(argv);
    assert(process_handle_out);

#ifdef _WIN32
    spawn_process_win32(filename, argv, process_handle_out);
#else
    spawn_process_unix(filename, argv, process_handle_out);
#endif
}

struct event_base *get_event_base(void)
{
    static struct event_base *base = NULL;

    if (base == NULL) {
        base = event_base_new();
        assert(base);
    }

    return base;
}

void signal_callback(evutil_socket_t socket, short signal, void *user_data)
{
    UNUSED(socket);
    UNUSED(signal);
    UNUSED(user_data);

    printf("Interrupted - shutting down.\n");
    event_base_loopexit(get_event_base(), NULL);
}

int main(int argc, char *argv[])
{
    struct event *signal_event = NULL;

#ifdef _WIN32
    WSADATA wsa_data;
    WSAStartup(0x0201, &wsa_data);
#endif

    // Setup interrupt handler.
    signal_event = evsignal_new(get_event_base(), SIGINT, signal_callback, NULL);
    assert(signal_event);

    if (event_add(signal_event, NULL) < 0) {
        fprintf(stderr, "Unable to setup interrupt handler.\n");

        return EXIT_FAILURE;
    }

    // Dispatch our main loop.
    printf("Entering main loop.\n");
    event_base_dispatch(get_event_base());

    return EXIT_SUCCESS;
}
