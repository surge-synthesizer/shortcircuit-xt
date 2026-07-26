#!/usr/bin/env python3
"""
Regenerates sf2_range_intersect.sf2, the fixture for the
"Import SF2 intersects preset and instrument ranges" case in
tests/importer_tests.cpp (see #1930).

An SF2 preset zone constrains the key/velocity range of the instrument zones it
selects rather than replacing it, so each imported zone is the intersection of
the two and an empty intersection produces no zone. This fixture deliberately
misaligns the two levels so those cases are distinguishable:

  preset zones:      key 0-59         key 60-127
  instrument zones:  key 0-71 (lo)    key 72-127 (hi)

Intersecting gives exactly three zones:

  (0,59)->lo    (60,71)->lo    (72,127)->hi

The (0,59) x (72,127) pair is empty and must produce nothing. Importing the
cross product instead gives four, including a bogus 72-127 duplicate of the low
sample. The two samples carry different originalPitch values (60 and 84) so the
test can tell which one landed on which span.

Run with no arguments to overwrite the fixture in place; output is
deterministic, so a no-op run leaves the file byte-identical.
"""

import math
import os
import struct
import sys

# --- generator opcodes we emit
GEN_INSTRUMENT = 41
GEN_KEYRANGE = 43
GEN_SAMPLEID = 53

NFRAMES = 512
PAD = 46  # spec-mandated zero padding after each sample
SAMPLE_RATE = 44100


def chunk(cid, data):
    out = cid.encode("latin1") + struct.pack("<I", len(data)) + data
    if len(data) & 1:
        out += b"\0"  # RIFF chunks are word aligned
    return out


def listchunk(ltype, data):
    return chunk("LIST", ltype.encode("latin1") + data)


def zstr(s, n):
    """Fixed-width NUL-padded string field."""
    b = s.encode("latin1")[: n - 1]
    return b + b"\0" * (n - len(b))


def sine(nframes, cycles):
    return b"".join(
        struct.pack("<h", int(20000 * math.sin(2 * math.pi * cycles * i / nframes)))
        for i in range(nframes)
    )


def keyrange(lo, hi):
    return struct.pack("<HH", GEN_KEYRANGE, (hi << 8) | lo)


def build():
    # ---- sdta: two distinguishable tones, each followed by the zero pad
    smpl = sine(NFRAMES, 8) + b"\0" * 2 * PAD + sine(NFRAMES, 16) + b"\0" * 2 * PAD
    lo_start, lo_end = 0, NFRAMES
    hi_start = NFRAMES + PAD
    hi_end = hi_start + NFRAMES

    info = b""
    info += chunk("ifil", struct.pack("<HH", 2, 1))
    info += chunk("isng", zstr("EMU8000", 8))
    info += chunk("INAM", zstr("SCXT Range Intersect Test", 26))

    sdta = chunk("smpl", smpl)

    # phdr: name[20], preset, bank, bagNdx, library, genre, morphology.
    # Every pdta list ends with a terminal record.
    phdr = zstr("Isect Test", 20) + struct.pack("<HHHIII", 0, 0, 0, 0, 0, 0)
    phdr += zstr("EOP", 20) + struct.pack("<HHHIII", 0, 0, 2, 0, 0, 0)

    # pbag/ibag: genNdx, modNdx
    pbag = struct.pack("<HH", 0, 0) + struct.pack("<HH", 2, 0) + struct.pack("<HH", 4, 0)

    # A preset zone's generator list must terminate with the instrument gen.
    pgen = keyrange(0, 59) + struct.pack("<HH", GEN_INSTRUMENT, 0)
    pgen += keyrange(60, 127) + struct.pack("<HH", GEN_INSTRUMENT, 0)
    pgen += struct.pack("<HH", 0, 0)

    pmod = struct.pack("<HHhHH", 0, 0, 0, 0, 0)

    # inst: name[20], bagNdx
    inst = zstr("Isect Inst", 20) + struct.pack("<H", 0)
    inst += zstr("EOI", 20) + struct.pack("<H", 2)

    ibag = struct.pack("<HH", 0, 0) + struct.pack("<HH", 2, 0) + struct.pack("<HH", 4, 0)

    # An instrument zone's generator list must terminate with the sampleID gen.
    igen = keyrange(0, 71) + struct.pack("<HH", GEN_SAMPLEID, 0)
    igen += keyrange(72, 127) + struct.pack("<HH", GEN_SAMPLEID, 1)
    igen += struct.pack("<HH", 0, 0)

    imod = struct.pack("<HHhHH", 0, 0, 0, 0, 0)

    # shdr: name[20], start, end, startloop, endloop, sampleRate,
    #       originalPitch, pitchCorrection, sampleLink, sampleType (1 = mono)
    shdr = zstr("isect_lo", 20) + struct.pack(
        "<IIIIIBbHH", lo_start, lo_end, lo_start + 16, lo_end - 16, SAMPLE_RATE, 60, 0, 0, 1
    )
    shdr += zstr("isect_hi", 20) + struct.pack(
        "<IIIIIBbHH", hi_start, hi_end, hi_start + 16, hi_end - 16, SAMPLE_RATE, 84, 0, 0, 1
    )
    shdr += zstr("EOS", 20) + struct.pack("<IIIIIBbHH", 0, 0, 0, 0, 0, 0, 0, 0, 0)

    pdta = b""
    for cid, data in (
        ("phdr", phdr),
        ("pbag", pbag),
        ("pmod", pmod),
        ("pgen", pgen),
        ("inst", inst),
        ("ibag", ibag),
        ("imod", imod),
        ("igen", igen),
        ("shdr", shdr),
    ):
        pdta += chunk(cid, data)

    body = b"sfbk" + listchunk("INFO", info) + listchunk("sdta", sdta) + listchunk("pdta", pdta)
    return b"RIFF" + struct.pack("<I", len(body)) + body


if __name__ == "__main__":
    default = os.path.join(os.path.dirname(os.path.abspath(__file__)), "sf2_range_intersect.sf2")
    path = sys.argv[1] if len(sys.argv) > 1 else default
    data = build()
    with open(path, "wb") as f:
        f.write(data)
    print(f"wrote {path} ({len(data)} bytes)")
