import os
from pathlib import Path
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext as BuildExtBase
import sys

SOURCE_DIR = Path(__file__).parent.resolve()
VERSION_SUFFIX = ".rc1"

def get_version(cuda_version=None) -> str:
    vfile = SOURCE_DIR / 'pigz.c'
    if not vfile.exists() and "CMAKE_SOURCE_DIR" in os.environ:
        # some build tools may copy src/python into a temporary directory, which disconnects it from the source tree
        # using CMAKE_SOURCE_DIR to find the source tree
        vfile = Path(os.environ["CMAKE_SOURCE_DIR"]) / 'VERSION'
    version = None
    try:
        assert vfile.exists(), f'Version file {vfile.resolve()} does not exist'
        key = "#define VERSION \"pigz"
        for line in vfile.read_text().splitlines():
            if line.startswith(key):
                version = line.replace(key, "").split()[0].replace("\"", "")
                break
    except: # FIXME: This is a hack. We need to read version from VERSION file
        version = '0.0.0'
        print(f'WARNING: Could not read version from {vfile.resolve()}. Setting version to {version}', file=sys.stderr)
    if VERSION_SUFFIX:
        version += VERSION_SUFFIX
    return version



class CMakeExtension(Extension):

    def __init__(self, name):
        # don't invoke the original build_ext for this special extension
        super().__init__(name, sources=[])


class ExtensionBuilder(BuildExtBase):

    def run(self):
        for ext in self.extensions:
            self.build_cmake(ext)
        super().run()

    def build_cmake(self, ext):
        cwd = Path().absolute()

        # these dirs will be created in build_py, so if you don't have
        # any python sources to bundle, the dirs will be missing
        build_temp = Path(self.build_temp)
        build_temp.mkdir(parents=True, exist_ok=True)
        extdir = Path(self.get_ext_fullpath(ext.name))
        extdir.parent.mkdir(parents=True, exist_ok=True)

        # example of cmake args
        config = 'Debug' if self.debug else 'Release'
        cmake_args = [
            '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + str(extdir.parent.absolute()),
            '-DCMAKE_BUILD_TYPE=' + config
        ]

        # example of build args
        build_args = [
            '--config', config,
            '--', '-j4'
        ]

        os.chdir(str(build_temp))
        self.spawn(['cmake', str(cwd)] + cmake_args)
        if not self.dry_run:
            build_args = ['cmake', '--build', '.'] + build_args
            self.spawn(build_args)
        # Troubleshooting: if fail on line above then delete all possible
        # temporary CMake files including "CMakeCache.txt" in top level dir.
        os.chdir(str(cwd))



setup(
    name='pigz',
    version=get_version(),
    packages=[],
    ext_modules=[CMakeExtension('pigz')],
    cmdclass={
        'build_ext': ExtensionBuilder,
    }
)