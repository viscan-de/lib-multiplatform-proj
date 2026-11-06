#!/usr/bin/env bash
# viscan fork of PROJ: merge an upstream PROJ release.
#
#   viscan/tools/merge_upstream.sh 9.10.0
#
# Run on a fresh branch. The merge is left uncommitted so it can be checked
# first. Upstream CI workflows are dropped (the fork has its own), which also
# resolves their conflicts. See FORK_CHANGES.md, "Merging a new PROJ release".
set -euo pipefail

if [ $# -ne 1 ]; then
  echo "usage: $0 <upstream PROJ tag, e.g. 9.10.0>" >&2
  exit 2
fi
tag="$1"
cd "$(git rev-parse --show-toplevel)"

git remote get-url upstream >/dev/null 2>&1 ||
  git remote add upstream https://github.com/OSGeo/PROJ.git
git fetch upstream --tags --no-recurse-submodules

git merge --no-ff --no-commit "$tag" || true

keep="build-multiplatform.yml build-ubuntu.yml release-multiplatform.yml"
{ git ls-files .github/workflows; git diff --name-only --diff-filter=U -- .github/workflows; } |
  sort -u |
  while read -r f; do
    if [ "$(dirname "$f")" = ".github/workflows" ] &&
       [[ " $keep " == *" $(basename "$f") "* ]]; then
      continue
    fi
    git rm -q -f --ignore-unmatch -- "$f"
  done

conflicts=$(git diff --name-only --diff-filter=U)
if [ -n "$conflicts" ]; then
  echo
  echo "Resolve these conflicts, then continue with the steps below:"
  echo "$conflicts"
fi

cat <<EOF

Next steps:
  cmake -S . -B build-tests -DBUILD_TESTING=ON -DVISCAN_IGNORE_TEST_EXCLUSIONS=ON
  cmake --build build-tests -j
      (undefined symbols? add stubs in viscan/src, see the file comments)
  ctest --test-dir build-tests -R viscan_math_only --output-on-failure
  python3 viscan/tools/update_test_exclusions.py build-tests
  git diff viscan/test/excluded_tests.cmake      # review every new exclusion
  git commit
EOF
