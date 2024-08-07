#!/usr/bin/env python3
import argparse
import gzip
from pathlib import Path
import subprocess as sp

import pigz

def pigz_compress(input_file, output_file):
    output_file = str(output_file)
    with open(input_file, 'rt') as r, pigz.open(output_file, 'wt') as w:
        for line in r:
            w.write(line)


def pigz_decompress(input_file, output_file):
    input_file = str(input_file)
    with pigz.open(input_file, 'rt') as r, open(output_file, 'wt') as w:
        for line in r:
            w.write(line)

def pigz_subproc_compress(input_file, output_file):
    """Use pigz as a subprocess"""
    with open(input_file, 'rt') as r, sp.Popen(['pigz', '-c'], stdin=sp.PIPE, stdout=open(output_file, 'wb'), text=True) as p:
        for line in r:
            p.stdin.write(line)


def pigz_subproc_decompress(input_file, output_file):
    """Use pigz as a subprocess"""
    with sp.Popen(['pigz', '-dc', input_file], stdout=sp.PIPE, text=True) as p, open(output_file, 'wt') as w:
        for line in p.stdout:
            w.write(line)

def gzip_compress(input_file, output_file):
    with open(input_file, 'rt') as r, gzip.open(output_file, 'wt') as w:
        for line in r:
            w.write(line)

def gzip_decompress(input_file, output_file):
    with gzip.open(input_file, 'rt') as r, open(output_file, 'wt') as w:
        for line in r:
            w.write(line)

def main():
    parser = argparse.ArgumentParser(description='')
    parser.add_argument('impl', type=str, help='Implementation to use', choices=['pigz', 'gzip', 'pigz_subproc'])
    parser.add_argument('task', type=str, help='Task to perform', choices=['compress', 'decompress'])
    parser.add_argument('input', type=Path, help='Input file')
    parser.add_argument('output', type=Path, help='Output file')
    args = parser.parse_args()
    table = {
        ('pigz', 'compress'): pigz_compress,
        ('pigz', 'decompress'): pigz_decompress,
        ('pigz_subproc', 'compress'): pigz_subproc_compress,
        ('pigz_subproc', 'decompress'): pigz_subproc_decompress,
        ('gzip', 'compress'): gzip_compress,
        ('gzip', 'decompress'): gzip_decompress,
    }
    func = table[(args.impl, args.task)]
    func(args.input, args.output)

if __name__ == '__main__':
    main()