#!/bin/sh
# shellcheck enable=all
set -eu

ROOT="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
status=0

step() {
    printf '\n==> %s\n' "$1"
}

fail() {
    printf 'ÉCHEC: %s\n' "$1" >&2
    status=1
}

step "shellcheck (bin/*)"
if command -v shellcheck >/dev/null 2>&1; then
    # shellcheck disable=SC2086 # les scripts sont des chemins sans espaces
    if ! shellcheck --enable=all "${ROOT}"/bin/* "${ROOT}"/check.sh; then
        fail "shellcheck"
    fi
else
    fail "shellcheck introuvable"
fi

step "compilation (bin/build)"
if ! "${ROOT}/bin/build"; then
    fail "compilation"
fi

step "tests Qt offscreen (bin/test)"
if ! QT_QPA_PLATFORM=offscreen "${ROOT}/bin/test"; then
    fail "tests"
fi

printf '\n'
if [ "${status}" -eq 0 ]; then
    printf 'OK: tous les contrôles ont réussi.\n'
else
    printf 'ERREUR: au moins un contrôle a échoué.\n' >&2
fi
exit "${status}"
