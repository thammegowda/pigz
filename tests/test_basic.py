import atexit
import gzip
import shutil
import sys
import tempfile
from pathlib import Path

BUILD_DIR = Path(__file__).resolve().parent.parent / 'build'
assert BUILD_DIR.is_dir(), f"BUILD_DIR::{BUILD_DIR} not found!"
sys.path.append(str(BUILD_DIR))
import pigz  # should be imported after sys.path is set


tmpdir = Path(tempfile.mkdtemp())
atexit.register(lambda: shutil.rmtree(tmpdir, ignore_errors=True))


def test_roundtrip_small():
    """Test writing and reading a file."""

    data = "Hello, World!\nThis is a test"
    out_path = tmpdir / 'test_roundtrip_small.gz'

    for reader, writer in [
        (pigz.open, pigz.open),    # self consistency (pigz -> pigz)
        (pigz.open, gzip.open),    # pigz writes -> gzip reads
        (gzip.open, pigz.open),    # gzip writes -> pigz reads
    ]:
        out_path.unlink(missing_ok=True)
        with writer(str(out_path), 'wt') as f:
            f.write(data)
        with reader(str(out_path), 'rt') as f:
            data2 = f.read()
        assert data == data2, f"reader::{reader} writer::{writer} failed"


def test_iterator():
    """Test reading a file line by line."""

    data = "Hello, World!\nThis is a test\nThis is the third line"
    out_path = tmpdir / 'test_iterator.gz'
    out_path.unlink(missing_ok=True)

    with pigz.open(str(out_path), 'wt') as f:
        f.write(data)

    with pigz.open(str(out_path), 'rt') as f:
        data2 = ''
        for line in f:
            data2 += line
    assert data == data2


def test_multi_writes():
    """Test writing multiple times to the same file."""

    data = "Hello, World!\nThis is a test\nThis is the third line"
    out_path = tmpdir / 'test_multi_writes.gz'
    out_path.unlink(missing_ok=True)
    n_writes = 3
    with pigz.open(str(out_path), 'wt') as f:
        for _ in range(n_writes):
            f.write(data)

    with pigz.open(str(out_path), 'rt') as f:
        data2 = f.read()
    assert data * n_writes == data2


def test_large_file():
    """Test writing and reading a large file."""

    import subprocess
    import uuid
    n_lines = 1_000_000
    n_lines = 100

    pigz_out = tmpdir / 'test_large_pigz.gz'
    lines = []
    with pigz.open(str(pigz_out), 'wt') as f1:
        for i in range(n_lines):
            # line =  f'{i:,}\n'
            line = str(uuid.uuid4()) + '\n'
            f1.write(line)
            lines.append(line)
    subprocess.call(f'ls -lh {pigz_out}', shell=True)

    with pigz.open(str(pigz_out), 'rt') as f1:
        num = 1
        for line in f1:
            print(f'{num}: {line}', end='')
