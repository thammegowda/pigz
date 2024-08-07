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

// pigz.c has a lot of global variables that and may not be thread safe
// to be safe, we fork and exec pigz in a new process and wait for it to complete
// for vscode, run "python -m pybind11 --includes" and add them in include path

namespace py = pybind11;
using byte = unsigned char;

const size_t BUFFER_SIZE = 1024 * 8;

// rename or alias read and write from unistd.h to avoid conflict with pybind11
namespace unistd {
    using ::read;
    using ::write;
    using ::close;
}


inline auto to_cstr(const std::vector<std::string>& vec) -> char** {
    auto size = vec.size();
    auto result = new char* [size + 1];
    for (size_t i = 0; i < size; i++) {
        result[i] = const_cast<char*>(vec[i].c_str());
    }
    result[size] = nullptr;
    return result;
}


namespace logger {

    const int16_t DEBUG = 10;
    const int16_t INFO = 20;
    const int16_t ERROR = 30;
    const int16_t level = INFO;

    void debug(const std::string& message) {
        if (level <= DEBUG) {
            std::cerr << "DEBUG:: " << message << std::endl;
        }
    }
    void info(const std::string& message) {
        if (level <= INFO) {
            std::cerr << "INFO:: " << message << std::endl;
        }
    }
    void warning(const std::string& message) {
        std::cerr << "WARN:: " << message << std::endl;
    }
    void error(const std::string& message) {
        std::cerr << "ERROR:: " << message << std::endl;
    }
};


class GzFile {
protected:
    std::string filename;
    bool decompress;

    int stdout_fd[2]; // Pipe for reading child's stdout
    int stdin_fd[2];  // Pipe for writing to child's stdin
    pid_t child_pid = -1;
    bool opened = false;
    size_t buffer_size = BUFFER_SIZE;

    FILE* pipe_read = nullptr;   // used by __next__

public:
    GzFile(std::string filename, std::string mode = "r", size_t buffer_size = BUFFER_SIZE) {
        this->filename = filename;
        if (!(mode == "rt" || mode == "wt")) { // only two modes
            // TODO: support binary mode
            throw std::runtime_error("Invalid mode. Only 'rt' or 'wt' allowed");
        }
        decompress = (mode == "rt");
        this->buffer_size = buffer_size;
    }

    ~GzFile() = default;

    GzFile& __enter__() {
        if (opened) {
            throw std::runtime_error("GzFile already opened");
        }
        // Create  pipes
        if (pipe(stdin_fd) == -1 || pipe(stdout_fd) == -1) {
            throw std::runtime_error("Error creating pipes (required for forking)");
        }

        // Fork a child process
        child_pid = fork();

        if (child_pid == -1) {
            perror("fork failed");
        }
        else if (child_pid == 0) { // Child process

            // INPUT: glue reader end to STDIN; close the write end
            dup2(stdin_fd[0], STDIN_FILENO);
            close(stdin_fd[1]);

            //OUTPUT:: close the read end
            close(stdout_fd[0]);
            auto out_write_end = stdout_fd[1];
            int file_fd = -1;
            std::vector<std::string> args = { "pigz", "-ck" };  // argv[0] is placeholder
            if (decompress) {
                // decompression: file to STDOUT; parent reads stdout_fd[0]
                dup2(out_write_end, STDOUT_FILENO);
                args.push_back("-d");
                args.push_back(filename);
            }
            else {
                // compression: STDOUT to filename; parent writes to stdin_fd[1]
                //logger::debug("GzFile writing to file: " + filename);
                file_fd = open(filename.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
                dup2(file_fd, STDOUT_FILENO);
                args.push_back("-f");  // it wont write compressed data (binary) to stdout if -f is not used
            }
            std::string args_str = "";
            for (auto& arg : args) { args_str += arg + " "; }
            //logger::debug("GzFile: calling pigz with args: " + args_str);
            auto exit_code = pigz_call(args.size(), to_cstr(args));
            close(stdout_fd[1]); // Close the write end of the pipe
            if (file_fd >= 0) {
                close(file_fd);
            }
            //logger::debug("GzFile: child process exiting with code: " + std::to_string(exit_code));
            exit(exit_code);
        }
        else {
            // Parent process
            close(stdin_fd[0]); // Close the read end of the pipe (parent only writes)
            close(stdout_fd[1]); // Close the write end of the pipe (parent only reads)
            opened = true;
        }
        return *this;
    }

    void __exit__() {
        //logger::debug("GzFile closing");
        // child process should have been forked
        if (child_pid <= 0) {
            std::cerr << "GzFile: child process not forked" << std::endl;
            logger::debug("GzFile: child process not forked");
            return;
        }
        unistd::close(stdin_fd[1]);  // close the write end STDIN
        unistd::close(stdout_fd[0]); // close the read end STDOUT
        wait(nullptr); // Wait for the child process to exit

        opened = false;
        if (pipe_read != nullptr) {
            fclose(pipe_read);
            pipe_read = nullptr;
        }
    }

    auto read() -> std::string {
        if (!opened) {
            throw std::runtime_error("GzFile not opened");
        }
        char buffer[buffer_size]; // Buffer to read data from the pipe
        std::string data = "";
        ssize_t total_bytes = 0;

        while (true) {
            ssize_t bytes_read = unistd::read(stdout_fd[0], buffer, sizeof(buffer) - 1); // Read from the pipe
            if (bytes_read > 0) {
                buffer[bytes_read] = '\0'; // Null-terminate the received data
                data += std::string(buffer);
                total_bytes += bytes_read;
            }
            else {
                //logger::debug("GzFile reader reached the end:: bytes read: " + std::to_string(bytes_read));
                break;
            }
        }
        return data;
    }

    void write(std::string data) {
        if (!opened) {
            throw std::runtime_error("GzFile not opened");
        }
        const char* message = data.c_str();
        auto msg_len = strlen(message);
        ssize_t bytes_written = unistd::write(stdin_fd[1], message, msg_len);

        if (bytes_written != msg_len) {
            throw std::runtime_error("Error writing to pipe. Expected # bytes: " + std::to_string(msg_len) +
                " Wrote bytes: " + std::to_string(bytes_written));
        }
    }

    GzFile& __iter__() {
        //logger::debug("GzFile: opening stdout pipe for reading");
        pipe_read = fdopen(stdout_fd[0], "r");
        //logger::debug("GzFile: opened stdout pipe for reading");
        return *this;
    }
    std::string __next__() {
        //logger::debug("GzFile: reading next line");
        // only read the next line and return the line
        char* buffer = nullptr;
        size_t len = 0;
        ssize_t bytes_read = getline(&buffer, &len, pipe_read);
        //logger::debug("GzFile: read next line, bytes read: " + std::to_string(bytes_read));
        if (bytes_read != -1) {
            auto tmp = std::string(buffer, bytes_read);
            return tmp;
        }
        else { // reached the end
            free(buffer);
            //logger::debug("GzFile: closing pipe_read");
            throw py::stop_iteration();
        }
    }
};



PYBIND11_MODULE(pigz, m) {
    m.doc() = "pigz native API";
    //m.def("__main__", &pigz_call, "Call pigz with arguments");
    
    py::class_<GzFile>(m, "open", "Open a file with pigz")
        .def(py::init<std::string, std::string>())
        .def("__enter__",
            &GzFile::__enter__, "Enter the context")
        .def("__exit__",
            [&](GzFile& obj, py::object exc_type, py::object exc_value, py::object traceback) { obj.__exit__(); }
            , "Exit the context")
        .def("read", &GzFile::read, "Read all data from the file")
        .def("write", &GzFile::write, "Write all data to the file")
        .def("__iter__", &GzFile::__iter__, "Iterable over lines")
        .def("__next__", &GzFile::__next__, "Reads next line")
        ;

}