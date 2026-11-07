#!/usr/bin/env python3
"""viscan fork of PROJ: run a gie test file without some of its operations.

    gie_filter.py run <gie executable> <file.gie> <excludes.txt>

excludes.txt holds one operation per line, as returned by operation_key()
(e.g. "+proj=tmerc +ellps=GRS80 +approx"). A temporary copy of the gie file,
with those operations blanked out (line numbers unchanged), is run with gie.
An exclusion that matches no operation is an error (stale list).

Used by the ctest tests "<name>.filtered" (see ViscanMathOnlyFinalize.cmake)
and by update_test_exclusions.py.
"""

import re
import subprocess
import sys
import tempfile
from pathlib import Path

# gie's directives (src/apps/gie.cpp, gie_tags)
TAGS = ("<gie>", "operation", "crs_src", "crs_dst", "use_proj4_init_rules",
        "accept", "expect", "roundtrip", "banner", "verbose", "direction",
        "tolerance", "ignore", "require_grid", "echo", "skip", "</gie>",
        "<gie-strict>", "</gie-strict>")
# Directives that are kept even inside an excluded operation.
KEEP = ("use_proj4_init_rules", "verbose", "<gie>", "</gie>", "<gie-strict>",
        "</gie-strict>")


def tag_of(line):
    word = line.split(None, 1)[0] if line.strip() else ""
    return word if word in TAGS else None


def blocks(lines):
    """Operations of a gie file: list of (first, end, key), 0-based lines,
    end exclusive. A block runs from 'operation' (or 'crs_src'/'crs_dst')
    to the next one or to the end of the <gie> section."""
    result = []
    inside = False
    current = None  # [first, key_parts, collecting]
    prev_tag = None

    def close(end):
        nonlocal current
        if current:
            result.append((current[0], end, " ".join(current[1])))
            current = None

    for i, line in enumerate(lines):
        tag = tag_of(line)
        if tag in ("<gie>", "<gie-strict>"):
            inside = True
        elif tag in ("</gie>", "</gie-strict>"):
            close(i)
            inside = False
        if not inside:
            prev_tag = tag or prev_tag
            continue
        starts = tag == "operation" or tag == "crs_src" or (
            tag == "crs_dst" and prev_tag != "crs_src")
        if starts:
            close(i)
            current = [i, [], True]
        if current is not None:
            if tag is not None and tag not in ("operation", "crs_src",
                                               "crs_dst"):
                current[2] = False  # arguments of the operation are done
            elif tag is None and not line[:1].isspace():
                current[2] = False  # banner/comment line, not an argument
            elif tag is None and not line.strip():
                current[2] = False
            if current[2]:
                text = line.split("#", 1)[0].replace("\\", " ")
                current[1].extend(text.split())
        if tag:
            prev_tag = tag
    close(len(lines))
    return result


def operation_key(block):
    return block[2]


def filtered_lines(lines, excludes):
    """Return (new lines, keys in excludes that were not found)."""
    out = list(lines)
    found = set()
    for first, end, key in blocks(lines):
        if key in excludes:
            found.add(key)
            for i in range(first, end):
                if tag_of(lines[i]) not in KEEP:
                    out[i] = "\n" if lines[i].endswith("\n") else ""
    return out, set(excludes) - found


def run(gie, gie_file, excludes, cwd=None, env=None):
    """Run gie on gie_file without the excluded operations.
    Returns (returncode, output)."""
    lines = Path(gie_file).read_text(errors="replace").splitlines(True)
    new_lines, stale = filtered_lines(lines, set(excludes))
    if stale:
        return 1, "".join(f"viscan: stale exclusion, no such operation: {s}\n"
                          for s in sorted(stale))
    with tempfile.TemporaryDirectory() as tmp:
        path = Path(tmp) / Path(gie_file).name
        path.write_text("".join(new_lines))
        res = subprocess.run([gie, str(path)], cwd=cwd, env=env,
                             capture_output=True, text=True, errors="replace")
    return res.returncode, res.stdout + res.stderr


def failing_keys(gie_file, output):
    """Operations of gie_file that gie reported as failing in output."""
    lines = Path(gie_file).read_text(errors="replace").splitlines(True)
    numbers = {int(n) for n in re.findall(r"FAILURE in [^\n(]*\((\d+)\)",
                                          output)}
    numbers |= {int(n) for n in re.findall(r"line no\. (\d+)", output)}
    keys = set()
    for first, end, key in blocks(lines):
        if any(first < n <= end for n in numbers):  # gie counts from 1
            keys.add(key)
    return keys


def main():
    if len(sys.argv) != 5 or sys.argv[1] != "run":
        sys.exit(__doc__)
    _, _, gie, gie_file, excludes_file = sys.argv
    excludes = [l.strip() for l in Path(excludes_file).read_text().splitlines()
                if l.strip()]
    code, output = run(gie, gie_file, excludes)
    sys.stdout.write(output)
    if "stale exclusion" not in output:
        print(f"viscan: {len(excludes)} operation(s) of "
              f"{Path(gie_file).name} excluded "
              "(viscan/test/excluded_tests.cmake)")
    sys.exit(code)


if __name__ == "__main__":
    main()
