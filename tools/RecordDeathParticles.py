"""Read-only particle measurements from the owner's running Steam Cube World.

Windows/Python 3. Load a test character, then run:
  python tools/RecordDeathParticles.py --pid PID --game "D:/.../Cube World" --seconds 120
Record a creature or test-character death during the capture. Only decoded particle measurements
are written, under the installation's private Extract/death folder. No injection,
process writes, native saves, input or executable dumps. Omit --seconds to inspect.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
import json
import math
import pathlib
import struct
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('--game', required=True, type=pathlib.Path)
    parser.add_argument('--seconds', type=float, default=0)
    args = parser.parse_args()
    if not 0 <= args.seconds <= 120:
        parser.error('capture duration must be 0-120 seconds')
    game = args.game.resolve()
    if not (game / 'cubeworld.exe').is_file():
        parser.error('choose the installed Steam Cube World directory')
    k = C.WinDLL('kernel32', use_last_error=True)
    k.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]; k.OpenProcess.restype = W.HANDLE
    k.CloseHandle.argtypes = [W.HANDLE]
    k.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    k.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)]
    k.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]; k.CreateToolhelp32Snapshot.restype = W.HANDLE
    k.GetTickCount64.restype = C.c_uint64
    process = k.OpenProcess(0x1000 | 0x10, False, args.pid)
    if not process:
        raise OSError(C.get_last_error(), 'cannot read Cube World')

    def read(address, count):
        data = C.create_string_buffer(count); got = C.c_size_t()
        if not k.ReadProcessMemory(process, address, data, count, C.byref(got)) or got.value != count:
            raise OSError(C.get_last_error(), 'native particle data is unavailable')
        return data.raw

    def u64(address):
        return struct.unpack('<Q', read(address, 8))[0]

    class Module(C.Structure):
        _fields_ = [('size', W.DWORD), ('id', W.DWORD), ('pid', W.DWORD), ('globalCount', W.DWORD),
                    ('processCount', W.DWORD), ('base', C.c_void_p), ('bytes', W.DWORD), ('handle', W.HMODULE),
                    ('name', W.WCHAR * 256), ('path', W.WCHAR * 260)]
    k.Module32FirstW.argtypes = [W.HANDLE, C.POINTER(Module)]
    k.Module32NextW.argtypes = [W.HANDLE, C.POINTER(Module)]
    try:
        path = C.create_unicode_buffer(32768); length = W.DWORD(32768)
        if not k.QueryFullProcessImageNameW(process, 0, path, C.byref(length)) or pathlib.Path(path.value).resolve() != game / 'cubeworld.exe':
            raise ValueError('PID does not belong to this installed cubeworld.exe')
        modules = k.CreateToolhelp32Snapshot(0x8 | 0x10, args.pid)
        if modules == C.c_void_p(-1).value:
            raise OSError(C.get_last_error(), 'cannot inspect the native module')
        try:
            module = Module(); module.size = C.sizeof(module); base = 0
            ok = k.Module32FirstW(modules, C.byref(module))
            while ok:
                if pathlib.Path(module.path).resolve() == game / 'cubeworld.exe':
                    base = module.base; break
                ok = k.Module32NextW(modules, C.byref(module))
        finally:
            k.CloseHandle(modules)
        if not base:
            raise ValueError('native game module not found')
        pe = struct.unpack('<I', read(base + 0x3c, 4))[0]
        if struct.unpack('<I', read(base + pe + 8, 4))[0] != 0x5d93ab0d:
            raise ValueError('unsupported Steam build (expected PE timestamp 5d93ab0d)')
        native_game = u64(base + 0x551a80); world = u64(native_game + 8)
        creature = u64(world + 1096)
        name = read(creature + 16 + 2376, 16).split(b'\0')[0].decode('utf8', errors='replace')
        head, count = struct.unpack('<QQ', read(native_game + 0x2090, 16))
        if not head or count > 20000:
            raise ValueError('invalid particle list')
        print(f'Ready: {name}; {count} native particles. Read-only access.', flush=True)
        if not args.seconds:
            return
        folder = game / 'Extract' / 'death'
        folder.mkdir(parents=True, exist_ok=True)
        output = folder / f'particles-{k.GetTickCount64()}.jsonl'
        tracked = {}; previous = {}; frames = 0; samples = 0; next_id = 0
        deadline = time.perf_counter() + args.seconds
        print(f'Recording for {args.seconds:g} seconds: {output}', flush=True)
        with output.open('x', encoding='utf8') as stream:
            stream.write(json.dumps({'format': 'cubevulkan-particles-1', 'build': '5d93ab0d', 'character': name}) + '\n')
            while time.perf_counter() < deadline:
                if u64(base + 0x551a80) != native_game or u64(native_game + 8) != world:
                    raise ValueError('world changed; recording stopped')
                at = u64(head); seen = set(); found = {}
                try:
                    while at != head and at not in seen and len(seen) < 20000:
                        seen.add(at)
                        data = read(at, 0xc8)
                        next_at, back = struct.unpack_from('<QQ', data)
                        kind, age = struct.unpack_from('<ii', data, 0x10)
                        lifetime = struct.unpack_from('<i', data, 0xc4)[0]
                        if kind == 0 and lifetime == 5000 and data[0xc0] == 1:
                            position = struct.unpack_from('<3q', data, 0x18)
                            angles = struct.unpack_from('<3f', data, 0x48)
                            color = struct.unpack_from('<4f', data, 0x54)
                            velocity = struct.unpack_from('<3f', data, 0x64)
                            spin = struct.unpack_from('<3f', data, 0x70)
                            basis = struct.unpack_from('<16f', data, 0x7c)
                            # Reject a node updated or recycled while its fields were read.
                            unchanged = read(at + 0x10, 8) == data[0x10:0x18]
                            if unchanged and all(math.isfinite(v) for v in angles + color + velocity + spin + basis) and -1000 <= age <= 5500:
                                found[at] = (age, position, velocity, angles, spin, basis, color)
                        if back == 0 or next_at == 0:
                            break
                        at = next_at
                except OSError:
                    found = {}  # Native list changed while walking; try next sample.
                now = k.GetTickCount64()
                for address, values in found.items():
                    age, position, velocity, angles, spin, basis, color = values
                    if address not in tracked or age < tracked[address][1]:
                        tracked[address] = (next_id, age)
                        next_id += 1
                        if next_id <= 8:
                            print(f'  particle {tracked[address][0]}, initial age {age} ms, speed {velocity}', flush=True)
                    ident = tracked[address][0]
                    tracked[address] = (ident, age)
                    if previous.get(address) == values:
                        continue
                    previous[address] = values
                    stream.write(json.dumps({'tick': now, 'id': ident, 'age_ms': age, 'pos': position,
                                             'velocity': velocity, 'angles': angles, 'spin': spin,
                                             'basis': basis, 'color': color, 'life_ms': 5000}, separators=(',', ':')) + '\n')
                    samples += 1
                stream.flush()
                frames += 1
                time.sleep(1 / 60)
        print(f'Saved {samples} samples in {frames} polls. Native game and saves were not changed.', flush=True)
    finally:
        k.CloseHandle(process)


if __name__ == '__main__':
    main()
