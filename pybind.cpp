#include "pybind11/pybind11.h"
#include "pybind11/stl.h"
#include <pybind11/stl_bind.h>
#include "pybind11/pytypes.h"
#include "pigz.h"

// for vscode, run "python -m pybind11 --includes" and add them in include path

namespace py = pybind11;

// dummy function before we add real bindings
int add(int i, int j) {
    return i + j;
}

PYBIND11_MODULE(_pigz, m) {
    m.doc() = "pigz native API"; // optional module docstring
    m.def("add", &add, "A function that adds two numbers"); // dummy function for testing
    m.def("call", [](std::vector<std::string> args) -> int64_t {
            return pigz_call(args.size(), (char **)args.data());
        }
    ); // only semicolon here
}