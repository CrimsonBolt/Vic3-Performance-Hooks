import struct, sys

# Portable vic3hook target resolver: re-derives the 12 hook run-body RVAs and
# 16-byte prologues from ANY victoria3.exe via the RTTI walk. Pure file reads.
#   python resolve_targets.py "D:\...\Victoria 3\binaries\victoria3.exe"
# Output feeds hooks.cpp (rva + P_* prologue rows) when the game updates.

path = sys.argv[1] if len(sys.argv) > 1 else r'C:\Users\CrimsonBolt\Desktop\Projects\vic3-re\binary\victoria3.exe'
F = open(path, 'rb').read()
PE = struct.unpack_from('<I', F, 0x3c)[0]
NSEC = struct.unpack_from('<H', F, PE + 6)[0]
OPT = struct.unpack_from('<H', F, PE + 20)[0]
SECS = []
for i in range(NSEC):
    off = PE + 24 + OPT + i * 40
    name = F[off:off + 8].rstrip(b'\0').decode()
    vsz, va, rsz, ra = struct.unpack_from('<IIII', F, off + 8)
    SECS.append((name, va, vsz, ra))

BASE = 0x140000000          # preferred image base (1.14.5)
SHARED_PREPARE = 0x1407d76b0  # CTickTaskWithExecute::Prepare shared by 100+ tasks
TEXT_END = BASE + max(sva + vsz for n, sva, vsz, ra in SECS if n == '.text')

def va2off(va):
    r = va - BASE
    for name, sva, vsz, ra in SECS:
        if sva <= r < sva + vsz:
            return ra + (r - sva)
    return None

def off2va(o):
    for name, sva, vsz, ra in SECS:
        if ra <= o < ra + vsz:
            return BASE + sva + (o - ra)
    return None

def rd_q(va):
    o = va2off(va)
    return struct.unpack_from('<Q', F, o)[0] if o is not None else None

def find_all(needle):
    out, i = [], 0
    while True:
        i = F.find(needle, i + 1)
        if i < 0:
            break
        out.append(i)
    return out

def is_code(va):
    return va is not None and BASE <= va < TEXT_END

def resolve(name):
    results = []
    pats = [('.?AVTickTask_' + name + '@').encode(),
            ('.?AVCTickTask_' + name + '@').encode(),
            ('.?AV?$CSimplifiedTickTask@VCTickTask_' + name + '@').encode()]
    seen = set()
    for pat in pats:
        for h in find_all(pat):
            name_va = off2va(h)
            if name_va is None:
                continue
            td_rva = name_va - 0x10 - BASE
            for c in find_all(struct.pack('<I', td_rva)):
                col_off = c - 0xC  # x64 COL: pTypeDescriptor at +0xC
                if col_off < 0:
                    continue
                col_va = off2va(col_off)
                if col_va is None or struct.unpack_from('<I', F, col_off)[0] != 1:
                    continue
                for v in find_all(struct.pack('<Q', col_va)):
                    v_va = off2va(v)
                    if v_va is None:
                        continue
                    v_va += 8  # qword found IS the COL pointer at vftable-8
                    if v_va in seen:
                        continue
                    seen.add(v_va)
                    slot6 = rd_q(v_va + 48)
                    slot7 = rd_q(v_va + 56)
                    results.append((v_va, slot6, slot7))
    return results

NAMES = ['PrepareNavyAIData', 'UpdateBuildingSpendingPriorities_Stage2', 'UpdateAI',
         'HandleAutonomousInvestment', 'UpdateSpendingVariables', 'RecalculateModifierNodes',
         'UpdatePopGrowth', 'ProcessEconomicMigration', 'UpdateAICountryGoalsAdjustments',
         'UpdateStatePopNeedCache', 'UpdateStateTrades', 'UpdateWorldMarket']

print(f'resolving {len(NAMES)} targets in {path}')
ok = 0
for n in NAMES:
    got = None
    for v_va, s6, s7 in resolve(n)[:4]:
        if s6 == SHARED_PREPARE:
            run = s7                      # Group A: slot7 = Execute
        elif not is_code(s7):
            run = s6                      # Group B: short vtable, slot6 = run
        else:
            continue
        if is_code(run):
            got = run
            break
    if got is None:
        print(f'{n:42s} UNRESOLVED (name/RTTI changed?)')
        continue
    ok += 1
    o = va2off(got)
    pro = F[o:o + 16].hex()
    b = ','.join(f'0x{pro[i:i+2]}' for i in range(0, 32, 2))
    print(f'{n:42s} rva 0x{got-BASE:07x}  {{ {b} }}')
print(f'{ok}/{len(NAMES)} resolved')
