import struct
import sys
from collections import Counter

# Portable vic3hook target resolver: re-derives the 12 hook run-body RVAs and
# 16-byte prologues from ANY victoria3.exe via the RTTI walk. Pure file reads.
#   python resolve_targets.py "D:\...\Victoria 3\binaries\victoria3.exe"
# Output feeds src/hooks.cpp (rva + P_* prologue rows) when the game updates.
#
# Build-independent by construction:
#   - the image base is read from the exe's own PE header (no constant);
#   - the shared CTickTaskWithExecute::Prepare (the Group A discriminator) is
#     found self-calibrating: the slot-6 value shared by the most tick-task
#     vtables (was 0x1407d76b0 on 1.14.5 — never hardcode it again);
#   - adjustor thunks are followed: a vtable slot-7 whose first bytes end in a
#     jmp rel32 is reported together with its jump target; the shipped table
#     hooks the jump target (e.g. UpdateSpendingVariables on 1.14.5: vtable
#     slot 7 = 0x1575af0, a thunk tail-jumping to the shipped 0x157aef0 —
#     equivalent placements, since all call traffic passes through the thunk).

if len(sys.argv) < 2:
    sys.exit('usage: python resolve_targets.py <path-to-victoria3.exe>')
path = sys.argv[1]

F = open(path, 'rb').read()
PE = struct.unpack_from('<I', F, 0x3c)[0]
NSEC = struct.unpack_from('<H', F, PE + 6)[0]
OPT = struct.unpack_from('<H', F, PE + 20)[0]
SECS = []
for i in range(NSEC):
    off = PE + 24 + OPT + i * 40
    name = F[off:off + 8].rstrip(b'\0').decode()
    vsz, va, rsz, ra = struct.unpack_from('<IIII', F, off + 8)
    SECS.append((name, va, vsz, rsz, ra))

MAGIC = struct.unpack_from('<H', F, PE + 24)[0]
if MAGIC == 0x20B:  # PE32+
    BASE = struct.unpack_from('<Q', F, PE + 24 + 24)[0]
elif MAGIC == 0x10B:  # PE32
    BASE = struct.unpack_from('<I', F, PE + 24 + 28)[0]
else:
    sys.exit('not a PE image')

TEXT = next((s for s in SECS if s[0] == '.text'), None)
if TEXT is None:
    sys.exit('no .text section')
TEXT_LO = BASE + TEXT[1]
TEXT_HI = BASE + TEXT[1] + TEXT[2]


def va2off(va):
    # only RVAs backed by raw section data map to file bytes (a virtual-only
    # tail like .bss has none); using raw size for the bound is the point
    r = va - BASE
    for name, sva, vsz, rsz, ra in SECS:
        if sva <= r < sva + rsz:
            return ra + (r - sva)
    return None


def off2va(o):
    for name, sva, vsz, rsz, ra in SECS:
        if ra <= o < ra + rsz:
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
    return va is not None and TEXT_LO <= va < TEXT_HI


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
                    v_off = v
                    v_va = off2va(v_off)
                    if v_va is None:
                        continue
                    v_va += 8  # the qword found IS the COL pointer at vftable-8
                    if v_va in seen:
                        continue
                    seen.add(v_va)
                    slot6 = rd_q(v_va + 48)
                    slot7 = rd_q(v_va + 56)
                    results.append((v_va, slot6, slot7))
    return results


def tail_jmp_target(va):
    """If the function at va is a short adjustor thunk ending in `jmp rel32`
    within its first 32 bytes, return the jump target (else None)."""
    o = va2off(va)
    if o is None:
        return None
    for p in range(8, 27):  # a real body's first bytes are a prologue; thunks jmp early
        if F[o + p] != 0xE9:
            continue
        rel = struct.unpack_from('<i', F, o + p + 1)[0]
        target = va + p + 5 + rel
        if is_code(target) and target != va:
            return target
    return None


def prologue_hex(va):
    o = va2off(va)
    if o is None:
        return '(unbacked)'
    b = F[o:o + 16]
    return '{ ' + ','.join('0x%02x' % x for x in b) + ' }'


NAMES = ['PrepareNavyAIData', 'UpdateBuildingSpendingPriorities_Stage2', 'UpdateAI',
         'HandleAutonomousInvestment', 'UpdateSpendingVariables', 'RecalculateModifierNodes',
         'UpdatePopGrowth', 'ProcessEconomicMigration', 'UpdateAICountryGoalsAdjustments',
         'UpdateStatePopNeedCache', 'UpdateStateTrades', 'UpdateWorldMarket']

print(f'resolving {len(NAMES)} targets in {path}')
print(f'image base 0x{BASE:x}  .text 0x{TEXT_LO:x}-0x{TEXT_HI:x}')

candidates = {n: resolve(n) for n in NAMES}

# self-calibrating Group A discriminator: the slot-6 address most tick-task
# vtables share is the common CTickTaskWithExecute::Prepare
slot6_counts = Counter()
for n in NAMES:
    for v_va, s6, s7 in candidates[n]:
        if is_code(s6):
            slot6_counts[s6] += 1
if not slot6_counts:
    sys.exit('no RTTI candidates found - are you pointing this at victoria3.exe?')
SHARED_PREPARE = slot6_counts.most_common(1)[0][0]
print(f'shared Prepare (modal slot-6, held by {slot6_counts[SHARED_PREPARE]} vtables): 0x{SHARED_PREPARE:x}')
print()

ok = 0
for n in NAMES:
    got = None
    for v_va, s6, s7 in candidates[n][:4]:
        if not is_code(s7):
            run, kind = s6, 'B'      # short CSimplifiedTickTask vtable: slot6 is the run body
        elif s6 == SHARED_PREPARE:
            run, kind = s7, 'A'      # Group A: slot7 Execute is the run body
        else:
            continue
        if is_code(run):
            got = (v_va, run, kind)
            break
    if got is None:
        print(f'{n:42s} UNRESOLVED (name/RTTI changed?)')
        continue
    ok += 1
    v_va, run, kind = got
    thunk = tail_jmp_target(run)
    print(f'{n:42s} group {kind}  run rva 0x{run - BASE:07x}  {prologue_hex(run)}')
    if thunk is not None:
        print(f'{"":42s}   vtable slot is an ADJUSTOR THUNK -> 0x{thunk - BASE:07x}')
        print(f'{"":42s}   thunk target prologue {prologue_hex(thunk)}')
        print(f'{"":42s}   (shipped table hooks the thunk target - keep that convention when updating)')
print()
print(f'{ok}/{len(NAMES)} resolved')
