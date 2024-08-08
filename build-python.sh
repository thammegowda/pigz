
#!/usr/bin/env bash

# This script is used to build the Python wheels.
# A requirement is that we have to use older GLIBC versions to ensure maximum compatibility.
# Python folks call it "manylinux" wheels and recommed using docker images to build them.
# official manylinux docs: https://github.com/pypa/manylinux
#     But the official manylinux images doesnt have CUDA support.
# So we use the "pytorch/manylinux-builder" image which has CUDA support.
#     Available tags: https://hub.docker.com/r/pytorch/manylinux-builder/tags


LINUX_IMAGE="quay.io/pypa/manylinux2014_x86_64"
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

set -x
LINUX_BUILDER="build-python-linux.sh"
MOUNT="/work"
docker run --rm -it -v $MYDIR:$MOUNT $LINUX_IMAGE bash $MOUNT/$LINUX_BUILDER
