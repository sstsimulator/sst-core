#!/usr/bin/env bash

# run-as-ci-user.sh: Run GitHub Actions step scripts as an unprivileged user
# inside a CI container, where steps otherwise run as root.
#
# Usage:
#   run-as-ci-user.sh --setup   create the user and give it the workspace
#   run-as-ci-user.sh SCRIPT    run SCRIPT as the user; use as a step's
#                               `shell: bash <this script> {0}`
#
# Call --setup only inside a container.  Without root or without the user
# (macOS, self-hosted runners), SCRIPT runs as the current user.

set -euo pipefail

ci_user="sst-ci"
# GitHub sets HOME to this directory inside containers.  runuser resets HOME
# from passwd, so use the same directory to keep any config that earlier
# steps wrote there.
ci_home="/github/home"

if [[ "$#" -ne 1 ]]; then
    echo "usage: ${0} --setup | SCRIPT" >&2
    exit 2
fi

if [[ "${1}" == "--setup" ]]; then
    useradd --home-dir "${ci_home}" --no-create-home --user-group "${ci_user}"
    chown --recursive "${ci_user}:${ci_user}" "${GITHUB_WORKSPACE}" "${ci_home}"
elif [[ "$(id -u)" -eq 0 ]] && id "${ci_user}" > /dev/null 2>&1; then
    exec runuser --user "${ci_user}" -- bash --noprofile --norc -eo pipefail "${1}"
else
    exec bash --noprofile --norc -eo pipefail "${1}"
fi
