#!/usr/bin/env python3
"""viscan fork of PROJ: regenerate viscan/test/excluded_tests.cmake.

Runs every upstream test of a math-only build and records the ones that
fail, so that they are not run in normal builds:

- gtest executables: the failing (or crashing) test cases, as a
  GTEST_FILTER exclusion. A suite where every case fails is written as
  "Suite.*".
- gie files: the failing operations. The original test is disabled and
  "<name>.filtered" runs the file without them (gie_filter.py). If every
  operation fails, the test is just disabled.
- other tests (scripts, ...): the whole ctest test is disabled.

Usage (after merging a new upstream release):

    cmake -S . -B build-tests -DBUILD_TESTING=ON -DVISCAN_IGNORE_TEST_EXCLUSIONS=ON
    cmake --build build-tests -j
    python3 viscan/tools/update_test_exclusions.py build-tests
    git diff viscan/test/excluded_tests.cmake

REVIEW THE DIFF. Every new exclusion must be explained by the removed
database / network / init-file support. A failure in the projection math
would also end up in the list. The viscan_* tests are never excluded.
"""

import json
import os
import re
import subprocess
import sys
import tempfile
from pathlib import Path

SCRIPT_DIR = Path(__file__).resolve().parent
sys.path.insert(0, str(SCRIPT_DIR))
import gie_filter  # noqa: E402

OUTPUT = SCRIPT_DIR.parent / "test" / "excluded_tests.cmake"
PROJ_TEST_DIR = SCRIPT_DIR.parent.parent / "test"
TIMEOUT = 1800


def ctest_tests(build_dir):
    out = subprocess.run(
        ["ctest", "--show-only=json-v1"], cwd=build_dir, check=True,
        capture_output=True, text=True).stdout
    tests = []
    for t in json.loads(out)["tests"]:
        props = {p["name"]: p["value"] for p in t.get("properties", [])}
        env = dict(os.environ)
        for item in props.get("ENVIRONMENT", []):
            key, _, value = item.partition("=")
            env[key] = value
        env.pop("GTEST_FILTER", None)
        tests.append({
            "name": t["name"],
            "command": t["command"],
            "cwd": props.get("WORKING_DIRECTORY", build_dir),
            "env": env,
        })
    return tests


def gtest_cases(test):
    """List of 'Suite.Case' names, or None if not a gtest executable."""
    if len(test["command"]) != 1:
        return None
    try:
        res = subprocess.run(
            test["command"] + ["--gtest_list_tests"], cwd=test["cwd"],
            env=test["env"], capture_output=True, text=True, timeout=120)
    except (OSError, subprocess.TimeoutExpired):
        return None
    if res.returncode != 0:
        return None
    cases, suite = [], None
    for line in res.stdout.splitlines():
        if not line.strip():
            continue
        if not line.startswith(" "):
            suite = line.split("#")[0].strip()
            if not suite.endswith("."):
                return None
        elif suite:
            cases.append(suite + line.split("#")[0].strip())
    return cases or None


def run_gtest(test, excluded):
    """Run with the given exclusions. Returns (failed cases, crashed case)."""
    with tempfile.TemporaryDirectory() as tmp:
        report = Path(tmp) / "report.json"
        env = dict(test["env"])
        if excluded:
            env["GTEST_FILTER"] = "-" + ":".join(sorted(excluded))
        res = subprocess.run(
            test["command"] + ["--gtest_output=json:" + str(report)],
            cwd=test["cwd"], env=env, capture_output=True, text=True,
            errors="replace", timeout=TIMEOUT)
        if report.exists():
            data = json.loads(report.read_text())
            failed = set()
            for suite in data.get("testsuites", []):
                for case in suite.get("testsuite", []):
                    if case.get("failures"):
                        failed.add(suite["name"] + "." + case["name"])
            return failed, None, res.returncode
        # Crashed: the last test that started but did not finish.
        running = None
        for line in res.stdout.splitlines():
            m = re.match(r"\[ RUN      \] (\S+)", line)
            if m:
                running = m.group(1)
            elif re.match(r"\[ +(OK|FAILED) +\] ", line):
                running = None
        return set(), running, res.returncode


def gtest_exclusions(test, cases):
    excluded = set()
    while True:
        failed, crashed, rc = run_gtest(test, excluded)
        if crashed:
            print(f"    crash in {crashed}")
            excluded.add(crashed)
            continue
        if failed:
            excluded |= failed
            continue
        # Still failing without any failing case: problem outside of the
        # test cases, disable the whole test.
        return excluded if rc == 0 else None


def is_gie_test(test):
    cmd = test["command"]
    return (len(cmd) == 2 and Path(cmd[0]).name.startswith("gie")
            and cmd[1].endswith(".gie"))


def gie_exclusions(test):
    """Failing operations of a gie test; None if the whole test must go."""
    gie, gie_file = test["command"]
    lines = Path(gie_file).read_text(errors="replace").splitlines(True)
    all_keys = {k for _, _, k in gie_filter.blocks(lines)}
    excluded = set()
    for _ in range(20):
        code, output = gie_filter.run(gie, gie_file, excluded,
                                      cwd=test["cwd"], env=test["env"])
        if code == 0:
            return excluded
        new = gie_filter.failing_keys(gie_file, output) - excluded
        if not new:
            return None  # fails without a failing operation: disable
        excluded |= new
        if excluded >= all_keys:
            return None
    return None


def collapse(excluded, cases):
    by_suite = {}
    for c in cases:
        by_suite.setdefault(c.split(".")[0], []).append(c)
    result = []
    for suite, suite_cases in sorted(by_suite.items()):
        if all(c in excluded for c in suite_cases):
            result.append(suite + ".*")
        else:
            result.extend(sorted(c for c in suite_cases if c in excluded))
    return result


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    build_dir = str(Path(sys.argv[1]).resolve())
    cache = Path(build_dir, "CMakeCache.txt").read_text()
    if "VISCAN_IGNORE_TEST_EXCLUSIONS:BOOL=ON" not in cache:
        sys.exit("configure the build with -DVISCAN_IGNORE_TEST_EXCLUSIONS=ON")

    disabled, filtered, gie_filtered = [], {}, {}
    for test in ctest_tests(build_dir):
        name = test["name"]
        if name.startswith("viscan_"):
            continue
        if is_gie_test(test):
            res = subprocess.run(test["command"], cwd=test["cwd"],
                                 env=test["env"], capture_output=True,
                                 timeout=TIMEOUT)
            if res.returncode == 0:
                print(f"pass  {name}")
                continue
            gie_file = Path(test["command"][1])
            n_ops = len(gie_filter.blocks(
                gie_file.read_text(errors="replace").splitlines(True)))
            excluded = gie_exclusions(test)
            if excluded is None:
                print(f"FAIL  {name}")
                disabled.append(name)
            else:
                print(f"      {name}: {len(excluded)} of {n_ops} operations "
                      "excluded")
                rel = gie_file.resolve().relative_to(PROJ_TEST_DIR.resolve())
                gie_filtered[name] = (rel.as_posix(), sorted(excluded), n_ops)
            continue
        cases = gtest_cases(test)
        if cases is None:
            res = subprocess.run(test["command"], cwd=test["cwd"],
                                 env=test["env"], capture_output=True,
                                 timeout=TIMEOUT)
            ok = res.returncode == 0
            print(f"{'pass' if ok else 'FAIL'}  {name}")
            if not ok:
                disabled.append(name)
            continue
        excluded = gtest_exclusions(test, cases)
        if excluded is None:
            print(f"FAIL  {name} (outside of test cases)")
            disabled.append(name)
        elif excluded:
            print(f"      {name}: {len(excluded)} of {len(cases)} excluded")
            filtered[name] = (collapse(excluded, cases), len(excluded),
                              len(cases))
        else:
            print(f"pass  {name} ({len(cases)} cases)")

    lines = [
        "# viscan fork of PROJ: upstream tests that fail in the math-only",
        "# build (no database, network or init files) and are not run.",
        "#",
        "# GENERATED by viscan/tools/update_test_exclusions.py. Do not edit by",
        "# hand; review the diff after regenerating.",
        "",
        "set(VISCAN_DISABLED_TESTS",
    ]
    lines += [f"  {n}" for n in sorted(disabled)]
    lines += [")", "", "set(VISCAN_GTEST_FILTERED_TESTS"]
    lines += [f"  {n}" for n in sorted(filtered)]
    lines.append(")")
    lines += ["", "set(VISCAN_GIE_FILTERED_TESTS"]
    lines += [f"  {n}" for n in sorted(gie_filtered)]
    lines.append(")")
    for name in sorted(gie_filtered):
        rel, keys, n_ops = gie_filtered[name]
        lines += ["", f"# {name}: {len(keys)} of {n_ops} operations excluded",
                  f"set(VISCAN_GIE_FILE_{name} {rel})",
                  f"set(VISCAN_GIE_EXCLUDES_{name}"]
        lines += [f"  [=[{k}]=]" for k in keys]
        lines.append(")")
    for name in sorted(filtered):
        patterns, n_excl, n_all = filtered[name]
        lines += ["", f"# {name}: {n_excl} of {n_all} test cases excluded",
                  f"set(VISCAN_GTEST_EXCLUDES_{name}"]
        lines += [f"  {p}" for p in patterns]
        lines.append(")")
    OUTPUT.write_text("\n".join(lines) + "\n")
    print(f"\nwritten {OUTPUT}")


if __name__ == "__main__":
    main()
