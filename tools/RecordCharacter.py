"""Read-only animation capture from the owner's loaded Steam Cube World.

Windows/Python 3. Example:
  python tools/RecordCharacter.py --pid 1234 --game "D:/.../Cube World" --character mage-water --seconds 60
Omit --seconds to inspect readiness. No injection, input, native save writes or
executable dumps. Output is private game data in Extract/characters only.
"""
import argparse
import ctypes as C
from ctypes import wintypes as W
import math
import pathlib
import struct
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--pid', required=True, type=int)
    parser.add_argument('--game', required=True, type=pathlib.Path)
    parser.add_argument('--character', required=True)
    parser.add_argument('--seconds', type=float, default=0)
    args = parser.parse_args()
    if not 0 <= args.seconds <= 120:
        parser.error('capture duration must be 0-120 seconds')
    game = args.game.resolve()
    folder = (game / 'Extract' / 'characters' / args.character).resolve()
    if folder.parent != (game / 'Extract' / 'characters').resolve() or not (folder / 'snapshot.txt').is_file():
        parser.error('choose an existing character snapshot under this installation')
    snapshot = dict(line.split('=', 1) for line in (folder / 'snapshot.txt').read_text().splitlines() if '=' in line)
    k = C.WinDLL('kernel32', use_last_error=True)
    k.OpenProcess.argtypes = [W.DWORD, W.BOOL, W.DWORD]; k.OpenProcess.restype = W.HANDLE
    k.CloseHandle.argtypes = [W.HANDLE]
    k.ReadProcessMemory.argtypes = [W.HANDLE, C.c_void_p, C.c_void_p, C.c_size_t, C.POINTER(C.c_size_t)]
    k.QueryFullProcessImageNameW.argtypes = [W.HANDLE, W.DWORD, W.LPWSTR, C.POINTER(W.DWORD)]
    k.CreateToolhelp32Snapshot.argtypes = [W.DWORD, W.DWORD]; k.CreateToolhelp32Snapshot.restype = W.HANDLE
    k.GetTickCount64.restype = C.c_uint64
    process = k.OpenProcess(0x1000 | 0x10, False, args.pid)  # QUERY_LIMITED_INFORMATION | VM_READ only
    if not process:
        raise OSError(C.get_last_error(), 'cannot read Cube World')

    def read(address, count):
        data = C.create_string_buffer(count); got = C.c_size_t()
        if not k.ReadProcessMemory(process, address, data, count, C.byref(got)) or got.value != count:
            raise OSError(C.get_last_error(), 'native character is unavailable')
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
            raise OSError(C.get_last_error(), 'cannot inspect the game module')
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
        if not creature:
            raise ValueError('load a character into the Steam world first')
        # SDK offsets verified for this exact build. Animation matrices contain
        # all visual root/limb turns; the entity stores world facing in degrees.
        def sample():
            if u64(u64(base + 0x551a80) + 8) != world or u64(world + 1096) != creature:
                raise ValueError('world or selected character changed; recording stopped')
            data = read(creature, 5680); entity = data[16:16 + 2400]; anim = data[3128:]
            if (entity[316], entity[317]) != (int(snapshot['class']), int(snapshot['specialization'])):
                raise ValueError('loaded class/specialization does not match the requested recording folder')
            if read(creature + 16 + 88, 8) != entity[88:96]:
                return None  # A game update raced the sample; retry next tick.
            matrices = anim[312:312 + 33 * 64]
            root = struct.unpack_from('<16f', matrices)
            if not all(math.isfinite(v) for v in root) or abs(root[15] - 1) > .001:
                return None
            for axis in range(3):
                size = math.sqrt(sum(root[axis * 4 + row] ** 2 for row in range(3)))
                if not .005 < size < 1:
                    return None
            return entity, anim, matrices

        ready = sample()
        if not ready:
            raise ValueError('wait until the game has drawn the loaded character')
        name = ready[0][2376:2392].split(b'\0')[0].decode('utf8', errors='replace')
        print(f'Ready: {name}, class {ready[0][316]}, specialization {ready[0][317]}', flush=True)
        if not args.seconds:
            return
        manager = u64(world + 112); begin, end = struct.unpack('<QQ', read(manager + 16, 16))
        if end < begin or (end - begin) % 8 or end - begin > 20000 * 8:
            raise ValueError('invalid native sprite table')
        sprites = {pointer: i for i, pointer in enumerate(struct.unpack('<' + 'Q' * ((end - begin) // 8), read(begin, end - begin)))}
        output = folder / f'anim-{k.GetTickCount64()}.cwa'
        count = 0; previous = None; deadline = time.perf_counter() + args.seconds
        print(f'Recording for {args.seconds:g} seconds: {output}', flush=True)
        with output.open('xb') as stream:
            stream.write(b'CWA2')
            while time.perf_counter() < deadline:
                captured = sample()
                if captured:
                    entity, anim, matrices = captured
                    state, timer = struct.unpack_from('<ii', anim, 8)[::-1]
                    flags = struct.unpack_from('<I', entity, 76)[0]; ability = entity[88]
                    phase, yaw = struct.unpack_from('<f', entity, 92)[0], struct.unpack_from('<f', entity, 32)[0]
                    slots = [sprites.get(pointer, -1) for pointer in struct.unpack_from('<12Q', anim, 216)] + [-1, -1]
                    stream.write(struct.pack('<Q', k.GetTickCount64()) + entity[:24] + struct.pack('<iiIi', state, timer, flags, ability))
                    stream.write(struct.pack('<ff', phase, yaw) + entity[36:48] + struct.pack('<14i', *slots) + matrices)
                    count += 1
                    if previous is None or ability != previous[0] or phase + 1 < previous[1]:
                        print(f'  ability {ability}, phase {phase:.1f} ms, state {state}', flush=True)
                    previous = ability, phase
                time.sleep(1 / 120)
        print(f'Saved {count} frames. Native game and saves were not changed.', flush=True)
    finally:
        k.CloseHandle(process)


if __name__ == '__main__':
    main()
