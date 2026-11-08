# COFF の .obj から関数の機械語を取り出す（再配置が無いことを確かめる）
#   python tools/extract_obj.py kinsoku/kinsoku.obj _kinsoku 出力.lua用のバイト列ファイル
import sys, struct

obj, sym, out = sys.argv[1], sys.argv[2], sys.argv[3]
d = open(obj, 'rb').read()
mach, nsec, ts, symptr, nsym, opt, ch = struct.unpack_from('<HHIIIHH', d, 0)
secs = []
for i in range(nsec):
    o = 20 + opt + 40 * i
    name = d[o:o + 8].rstrip(b'\0')
    vsize, vaddr, rsize, rptr, relptr, lnptr, nrel, nln, flags = struct.unpack_from('<IIIIIIHHI', d, o + 8)
    secs.append((name, rsize, rptr, nrel))
strtab = symptr + 18 * nsym
target = None
i = 0
while i < nsym:
    o = symptr + 18 * i
    raw = d[o:o + 8]
    if raw[:4] == b'\0\0\0\0':
        off = struct.unpack_from('<I', raw, 4)[0]
        nm = d[strtab + off:d.index(b'\0', strtab + off)]
    else:
        nm = raw.rstrip(b'\0')
    value, secno, typ, cls, naux = struct.unpack_from('<IhHBB', d, o + 8)
    if nm.decode() == sym:
        target = (value, secno)
    i += 1 + naux
if not target:
    sys.exit('symbol not found')
value, secno = target
name, rsize, rptr, nrel = secs[secno - 1]
print('section', name, 'size', rsize, 'relocs', nrel, 'offset', value)
if nrel:
    sys.exit('relocations present: code is not position independent')
code = d[rptr + value: rptr + rsize]
open(out, 'w').write(' '.join('%02X' % b for b in code) + '\n')
print('bytes', len(code))
