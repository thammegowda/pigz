#include<unistd.h>
#include <iostream>
#include <vector>
#include <string>
#include <memory>
#include <sys/time.h>
#include <sys/wait.h>
#include <cstring>
#include <fcntl.h>

#include "pybind11/pybind11.h"
#include <pybind11/stl.h>
#include "pigz.h"


// for vscode, run "python -m pybind11 --includes" and add them in include path

namespace py = pybind11;
using byte = unsigned char;


/** TODOs,. add utility functions for the following
 *  1. compress
 *  2. decompress
 *  3. word, line, char counter (wc) 
 * 
*/

// pigz.c has a lot of global variables that and may not be thread safe
// to be safe, we fork and exec pigz in a new process and wait for it to complete


inline auto to_cstr(const std::vector<std::string> &vec) -> char** {
    auto size = vec.size();
    auto result = new char*[size + 1];
    for (size_t i = 0; i < size; i++) {
        result[i] = const_cast<char*>(vec[i].c_str());
    }
    result[size] = nullptr;
    return result;
}

inline int64_t subprocess_pigz(std::vector<std::string> args) {
    char **argv = to_cstr(args);
    auto child_pid = fork();
    if (child_pid == -1) { // error forking
        std::cerr << "ERROR:: pigz fork failed" << std::endl;
        return -1;
    } else if (child_pid == 0) { // child process
        auto return_code = pigz_call(args.size(), argv);
        exit(return_code);
    } else { // parent process
        //TODO: write to stdin and read from stdout of child <child_pid> process if needed
        int status;
        waitpid(child_pid, &status, 0);
        return status;
    }
}

class GzFile {
public:
    std::string filename;
    bool decompress;

    int stdout_fd[2]; // Pipe for reading child's stdout
    int stdin_fd[2];  // Pipe for writing to child's stdin
    pid_t child_pid = -1;

    GzFile(std::string filename, std::string mode="r") {
        this->filename = filename;
        assert (mode == "r" || mode == "w"); // only two modes
        decompress = (mode == "r" );
        //std::cerr << "Mode: " << mode << " decompress: " << decompress << std::endl;
    }
    ~GzFile() = default;

    GzFile& open_() {
        // Create  pipes
        if (pipe(stdin_fd) == -1 || pipe(stdout_fd) == -1) {
            perror("Error creating pipes");
            throw std::runtime_error("Error creating pipes (required for forking)");
        }

        // Fork a child process
        pid_t child_pid = fork();

        if (child_pid == -1) {
            perror("fork failed");
        } else if (child_pid == 0) { // Child process

            // INPUT: glue readend to STDIN; close the write end
            dup2(stdin_fd[0], STDIN_FILENO);
            close(stdin_fd[1]);

            //OUTPUT:: close the read end
            close(stdout_fd[0]);
            auto out_write_end = stdout_fd[1];
            int file_fd = -1;
            std::vector<std::string> args = {"pigz", "-ck"};  // argv[0] is placeholder
            if (decompress) {
                dup2(out_write_end, STDOUT_FILENO);
                args.push_back("-d");
                args.push_back(filename);
                // STDOUT to pipe (parent reads)
            } else {
                // STDOUT to filename
                std::cerr << "GzFile writing to file: " << filename << std::endl;
                file_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(file_fd, STDOUT_FILENO);
                args.push_back("-f");  // it wont write compressed data (binary) to stdout if -f is not used
                //std::cout << "Hello there from pybind" << std::endl;
                //dup2(out_write_end, STDOUT_FILENO);
            }
            std::cerr << "GzFile child process:: pigz args: ";
            for (auto arg: args) { std::cerr << arg << " "; }
            std::cerr << std::endl;

            auto exit_code = pigz_call(args.size(), to_cstr(args));
            std::cerr << "Child finished:: pigz exit code:" << exit_code << std::endl;
            close(stdout_fd[1]); // Close the write end of the pipe
            if (file_fd >= 0) {
                close(file_fd);
            }
            exit(exit_code);

        } else {
            // Parent process
            close(stdin_fd[0]); // Close the read end of the pipe (parent only writes)
            close(stdout_fd[1]); // Close the write end of the pipe (parent only reads)
            // parent return to caller
        }
        return *this;
    }

    auto read_all() -> std::string {
        char buffer[1024 * 8]; // Buffer to read data from the pipe
        std::string data = "";
        ssize_t total_bytes = 0;

        while (true) {
            ssize_t bytes_read = read(stdout_fd[0], buffer, sizeof(buffer)-1); // Read from the pipe
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0'; // Null-terminate the received data
                data += std::string(buffer);
                total_bytes += bytes_read;
            } else {
                std::cerr << "GzFile reader reached the end:: bytes read: " << bytes_read << std::endl;
                break;
            }
        }
        //close(stdout_fd[0]); // Close the read end of the pipe
        //wait(nullptr); // Wait for the child process to exit
        std:: cerr << "GzFile read_all:: total bytes read: " << total_bytes << std::endl;
        return data;
    }

    void write_all(std::string data) {
        std::cerr << "GzFile writing:: data:\n" << data << std::endl;
        const char* message = data.c_str();
        auto msg_len = strlen(message);
        ssize_t bytes_written = write(stdin_fd[1], message, msg_len);
        //const char* message = "Hello from parent process!";
        //ssize_t bytes_written = write(stdin_fd[1], message, strlen(message));
        //std::cerr << "GzFile writing:: bytes written: " << bytes_written << std::endl;

        if (bytes_written != msg_len) {
            std::cerr << "Error writing to pipe. Expected # bytes: " << msg_len
                << " Wrote bytes: " << bytes_written  << std::endl;
        }

        //close(stdin_fd[1]); // Close the write end of the pipe
        //wait(nullptr); // Wait for the child process to exit
    }

    void close_() {
        std::cerr << "GzFile closing" << std::endl;
        assert(child_pid > 0); // child process should have been forked
        close(stdin_fd[1]);  // close the write end STDIN
        wait(nullptr); // Wait for the child process to exit
        close(stdout_fd[0]); // close the read end STDOUT
        //kill(child_pid, SIGKILL);
        std::cerr << "GzFile closed" << std::endl;
    }
};


PYBIND11_MODULE(pigz, m) {
    m.doc() = "pigz native API";
    // functional
    m.def("call", &subprocess_pigz, "Call pigz with arguments similar to CLI. Returns exit code.");
    // object oriented
    py::class_<GzFile>(m, "open", "Open a file with pigz")
        .def(py::init<std::string, std::string>())
        .def("__enter__",
            &GzFile::open_, "Enter the context")
        .def("__exit__",
            [&](GzFile& obj, py::object exc_type, py::object exc_value, py::object traceback) { obj.close_(); }
            , "Exit the context")
        .def("read_all", &GzFile::read_all, "Read all data from the file")
        .def("write_all", &GzFile::write_all, "Write all data to the file")
        ;

}