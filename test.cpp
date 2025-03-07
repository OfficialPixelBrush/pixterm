#include <iostream>
#include <pty.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <cstring>

int main() {
    int master_fd, slave_fd;
    pid_t pid;

    // Create PTY
    if (openpty(&master_fd, &slave_fd, nullptr, nullptr, nullptr) == -1) {
        perror("openpty");
        return 1;
    }

    pid = fork();
    if (pid == -1) {
        perror("fork");
        return 1;
    } else if (pid == 0) {  // Child process: Attach to PTY and start shell
        close(master_fd);
        setsid();
        dup2(slave_fd, STDIN_FILENO);
        dup2(slave_fd, STDOUT_FILENO);
        dup2(slave_fd, STDERR_FILENO);
        close(slave_fd);
        execl("/bin/bash", "/bin/bash", nullptr);
        perror("execl");
        return 1;
    } else {  // Parent process: Read and write to shell
        close(slave_fd);
        char buffer[256];

        // Make stdin non-blocking
        fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

        while (true) {
            struct pollfd fds[] = {
                { master_fd, POLLIN, 0 }, // Shell output
                { STDIN_FILENO, POLLIN, 0 } // User input
            };

            if (poll(fds, 2, -1) > 0) {
                // Read from shell and print to stdout
                if (fds[0].revents & POLLIN) {
                    ssize_t len = read(master_fd, buffer, sizeof(buffer) - 1);
                    if (len > 0) {
                        buffer[len] = '\0';
                        std::cout << buffer << std::flush;
                    }
                }

                // Read user input and send to shell
                if (fds[1].revents & POLLIN) {
                    ssize_t len = read(STDIN_FILENO, buffer, sizeof(buffer) - 1);
                    if (len > 0) {
                        write(master_fd, buffer, len);
                    }
                }
            }
        }
    }

    return 0;
}
