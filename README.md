# `pigz`

 2.8 (19 Aug 2022) by Mark Adler

pigz, which stands for Parallel Implementation of GZip, is a fully functional
replacement for gzip that exploits multiple processors and multiple cores to
the hilt when compressing data.

pigz was written by Mark Adler and does not include third-party code. I am
making my contributions to and distributions of this project solely in my
personal capacity, and am not conveying any rights to any intellectual property
of any third parties.

This version of pigz is written to be portable across Unix-style operating
systems that provide the zlib and pthread libraries.

Type "make" in this directory to build the "pigz" executable.  You can then
install the executable wherever you like in your path (e.g. /usr/local/bin/).
Type "pigz" to see the command help and all of the command options.

The latest version of pigz can be found at http://zlib.net/pigz/ .  You need
zlib version 1.2.3 or later to compile pigz.  zlib version 1.2.6 or later is
recommended, which reduces the overhead between blocks.  You can find the
latest version of zlib at http://zlib.net/ .  You can look in pigz.c for the
change history.

Questions, comments, bug reports, fixes, etc. can be emailed to Mark at his
address in the license below.

The license from pigz.c is copied here:

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler
  madler@alumni.caltech.edu

## Python Bindings

Python Bindings are powered by Pybind11 and the source code is available at https://github.com/thammegowda/pigz.

### Install pigz
```bash
# Option 1: from PyPI
pip install pigz

# Option 2: from source code
pip install git+https://github.com/thammegowda/pigz
```

### Features
1. Drop-in replacement for built in `gzip` (Aiming to be. Currently WIP)
2. Handles context managers. Automatically opens and closes files when used correctly with `with` block


### Limitations
1. Python APIs are currently tailored for text mode
2. Maybe unstable on some platforms. Tested on GNU/Linux for now.

### Usage

```python
import pigz

data = "Hello, World!\nThis is a test\nThis is the third line"
out_path = 'test_iterator.gz'

# How to write data
with pigz.open(str(out_path), 'wt') as f:
   f.write(data)

# How to read data
with pigz.open(str(out_path), 'rt') as f:
   data2 = ''
   for line in f:
      data2 += line
assert data == data2
```

See `tests/` directory for some more examples


## Run Tests for Python Module

```bash
cmake -B build
cmake --build build
python -m pytest -vs tests/

```


## Build Wheels for Release

> NOTE: this for developers only

`bash build.sh`

Manylinux wheels are produced under `dist-manylinux/`


**Upload to PyPI**
```bash
twine upload -r pypi dist-manylinux/pigz-*.whl
```
