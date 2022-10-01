#!/usr/bin/env bash
# require: bash version >= 4
# usage example: bash remove_code.sh src 4.27
set -eEu


SUPPORTED_VERSIONS=(
    "4.26.0" "4.27.0"
    "5.0.0"
)

function usage() {
    echo "Usage: bash replace_engine_version.sh <source-directory> <engine-version>"
}

if [ $# -ne 2 ]; then
    exit 1
fi

source_dir=${1}
engine_version=${2}

supported=0
for v in "${SUPPORTED_VERSIONS[@]}"; do
    if [ "${v}" = "${engine_version}" ]; then
        supported=1
    fi
done
if [ ${supported} -eq 0 ]; then
    echo "${engine_version} is not supported."
    echo "Supported version is ${SUPPORTED_VERSIONS[*]}"
    exit 1
fi

for file in $(find "${source_dir}" -name "*.cpp" -or -name "*.h"); do
    while read line do
        if [[ "$line" =~ //\s+@remove-start\s+UE_VERSION=([0-9.,]) ]]; then
            local versions=${BASH_REMATCH[0]}
            echo versions
        fi
    done < "$file"

    echo "Replaced engine version in ${file}"
done
