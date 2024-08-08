#!/usr/bin/env bash

# DO NOT call this script directly (unless you know what you are doing).
#    Use the build.sh script instead.
# this script builds pigz wheels for multiple python versions
# it uses mamba to create python environments and builds the wheels
# it also creates manylinux wheels using auditwheel

set -eu
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cd $MYDIR
PY_VERSIONS="$(echo 3.{7..13})"
PY_PLATFORM="manylinux2014_x86_64"

# cleanup
rm -rf dist dist-manylinux build

# build wheel for all supported python versions
for v in $PY_VERSIONS; do
    which python$v >& /dev/null || {
        echo "Python $v not found. Skipping it."
        continue
    }
    p=$(dirname $(realpath $(which python$v)));
    rm -rf build
    # python$v-config bin is required but was unavailable in the default PATH, hence $p is added to enable python$v-config
    PATH=$p:$PATH python$v -m build -w -o dist/
 done

# wheels are stored here
ls -lh dist/

# make the wheels manylinux compatible
auditwheel repair --plat $PY_PLATFORM dist/*.whl -w dist-manylinux/
ls -lh dist-manylinux/

echo "=== Done ==="
