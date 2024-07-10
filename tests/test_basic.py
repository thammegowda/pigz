import sys
from pathlib import Path

BUILD_DIR = Path(__file__).resolve().parent.parent / 'build'
assert BUILD_DIR.is_dir(), f"BUILD_DIR::{BUILD_DIR} not found!"
sys.path.append(str(BUILD_DIR))


import pigz
import tempfile
import atexit
import shutil

tmpdir = Path(tempfile.gettempdir())
atexit.register(lambda: shutil.rmtree(tmpdir, ignore_errors=True))


def test_roundtrip_small():
    """Test writing and reading a file."""

    data = "Hello, World!\nThis is a test"
    out_path = tmpdir / 'test_roundtrip_small.gz'
    out_path.unlink(missing_ok=True)

    with pigz.open(str(out_path), 'w') as f:
        f.write_all(data)
    with pigz.open(str(out_path), 'r') as f:
        data2 = f.read_all()
    assert data == data2
