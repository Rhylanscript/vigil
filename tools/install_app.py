#!/usr/bin/env python3
"""
tools/install_app.py

Copies a file from PC into VIGILs disk image (dist/storage.img) as a
normal VIGIL file, creating any missing folders on the way.

It does what VIGILs filesystem code (kernel/fs/fs.c) does by hand, so
the two must agree on the layout. If fs.c is ever changed, change this too

Usage:
    python tools/install_app.py <image> <file on your pc> <path inside VIGIL>
Example:
    python tools/install_app.py dist/storage.img build/hello.vapp apps/hello

NOTE!!:
Close QEMU first: it keeps the image file open while it is running.
"""

import struct
import sys
from collections.abc import Sequence
from typing import BinaryIO, NoReturn, TypedDict

# --- disk layout ---

SECTOR_SIZE     = 512
TABLE_LBA       = 1             # the file table starts at sector 1
TABLE_SECTORS   = 2             # -> and is 2 sectors (1024 bytes) long
DATA_START_LBA  = 100           # file contents are stored from sector 100 on
MAGIC           = 0x46535632    # marks a disk that already has a file table
ROOT            = 0xFF          # "parent" value meaning "toplevel folder"
MAX_FILES       = 32
MAX_NAME_LEN    = 16            # 15 characters + a terminating zero byte

# One table entry is packed with no padding:
#   name[16] | start_lba (4 bytes) | size_bytes (4) | used (1) | is_dir (1) | parent (1)
# '<' means littleendian (the byte order of x86) with no padding
ENTRY_FORMAT = '<16sIIBBB'


class Entry(TypedDict):
    name: str
    start_lba: int
    size: int
    used: int
    is_dir: int
    parent: int


# --- reading and writing the table ---

def empty_entry() -> Entry:
    return {'name': '', 'start_lba': 0, 'size': 0, 'used': 0, 'is_dir': 0, 'parent': ROOT}


def read_table(image: BinaryIO) -> list[Entry]:
    image.seek(TABLE_LBA * SECTOR_SIZE)
    raw = image.read(TABLE_SECTORS * SECTOR_SIZE)

    magic = struct.unpack_from('<I', raw, 0)[0] if len(raw) >= 4 else 0
    if magic != MAGIC:
        return [empty_entry() for _ in range(MAX_FILES)]

    entries: list[Entry] = []
    entry_size = struct.calcsize(ENTRY_FORMAT)
    for i in range(MAX_FILES):
        name, start, size, used, is_dir, parent = struct.unpack_from(ENTRY_FORMAT, raw, 4 + i * entry_size)
        entries.append({
            'name': name.split(b'\0')[0].decode('ascii', errors='replace'),
            'start_lba': start,
            'size': size,
            'used': used,
            'is_dir': is_dir,
            'parent': parent,
        })
    return entries


def write_table(image: BinaryIO, entries: Sequence[Entry]) -> None:
    raw = struct.pack('<I', MAGIC)
    for e in entries:
        raw += struct.pack(
            ENTRY_FORMAT,
            e['name'].encode('ascii'),
            e['start_lba'],
            e['size'],
            e['used'],
            e['is_dir'],
            e['parent'],
        )
    raw = raw.ljust(TABLE_SECTORS * SECTOR_SIZE, b'\0')
    image.seek(TABLE_LBA * SECTOR_SIZE)
    image.write(raw)


# --- looking up ---

def find_child(entries: Sequence[Entry], parent: int, name: str) -> int | None:
    for i, e in enumerate(entries):
        if e['used'] and e['parent'] == parent and e['name'] == name:
            return i
    return None


def find_free_slot(entries: Sequence[Entry]) -> int | None:
    for i, e in enumerate(entries):
        if not e['used']:
            return i
    return None


def sectors_needed(size: int) -> int:
    return (size + SECTOR_SIZE - 1) // SECTOR_SIZE


def find_free_space(entries: Sequence[Entry], sectors: int, skip_index: int | None) -> int:
    """the first gap between existing files big enough to hold"""
    ranges: list[tuple[int, int]] = []
    for i, e in enumerate(entries):
        if e['used'] and not e['is_dir'] and i != skip_index:
            ranges.append((e['start_lba'], e['start_lba'] + sectors_needed(e['size'])))
    ranges.sort()

    cursor = DATA_START_LBA
    for start, end in ranges:
        if start > cursor and start - cursor >= sectors:
            return cursor
        if end > cursor:
            cursor = end
    return cursor


# --- the install ---

def fail(message: str) -> NoReturn:
    print('install_app: ' + message, file=sys.stderr)
    sys.exit(1)

def image_size(image: BinaryIO) -> int:
    image.seek(0, 2)
    return image.tell()

def install(image_path: str, pc_file: str, vigil_path: str) -> None:
    parts = [p for p in vigil_path.split('/') if p]
    if not parts:
        fail('empty path inside VIGIL')
    for p in parts:
        if len(p) >= MAX_NAME_LEN or p in ('.', '..'):
            fail("bad name '%s' (1-15 characters)" % p)

    try:
        with open(pc_file, 'rb') as f:
            data = f.read()
    except OSError as err:
        fail('cannot read %s: %s' % (pc_file, err))

    try:
        image = open(image_path, 'r+b')   # r+b = read AND write, binary
    except OSError as err:
        fail('cannot open %s: %s (run the build first?)' % (image_path, err))

    with image:
        entries = read_table(image)

        # walk down the folders, creating any that are missing
        parent = ROOT
        for folder in parts[:-1]:
            index = find_child(entries, parent, folder)
            if index is None:
                index = find_free_slot(entries)
                if index is None:
                    fail('the VIGIL file table is full (32 items)')
                entries[index] = {'name': folder, 'start_lba': 0, 'size': 0,
                                  'used': 1, 'is_dir': 1, 'parent': parent}
            elif not entries[index]['is_dir']:
                fail("'%s' is a file, not a folder" % folder)
            parent = index

        leaf = parts[-1]
        existing = find_child(entries, parent, leaf)
        if existing is not None and entries[existing]['is_dir']:
            fail("'%s' is a folder" % leaf)

        index = existing if existing is not None else find_free_slot(entries)
        if index is None:
            fail('the VIGIL file table is full (32 items)')

        sectors = sectors_needed(len(data))
        start = find_free_space(entries, sectors, existing)

        if (start + sectors) * SECTOR_SIZE > image_size(image):
            fail('the disk image is too small for this file')

        image.seek(start * SECTOR_SIZE)
        image.write(data.ljust(sectors * SECTOR_SIZE, b'\0'))

        entries[index] = {'name': leaf, 'start_lba': start, 'size': len(data),
                          'used': 1, 'is_dir': 0, 'parent': parent}
        write_table(image, entries)

    print('installed %s -> %s (%d bytes)' % (pc_file, vigil_path, len(data)))


if __name__ == "__main__":
    if len(sys.argv) != 4:
        fail('usage: install_app.py <image> <file on pc> <path inside VIGIL>')
    install(sys.argv[1], sys.argv[2], sys.argv[3])
