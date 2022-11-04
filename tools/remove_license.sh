#!/usr/bin/env bash
# require: bash version >= 4
# usage example: bash remove_license.sh
set -eEu


function usage() {
    echo "Usage: bash remove_license.sh <source-directory>"
}

if [ $# -ne 1 ]; then
    exit 1
fi

SOURCE_DIR=${1}

for file in $(find "${SOURCE_DIR}" -name "*.cpp" -or -name "*.h" -or -name "*.cs"); do
    echo "${file}"
    sed -i -z -e "s/\s*\\*\s*This software is released under the MIT License.*\s*\\*\s*https:\\/\\/opensource.org\\/licenses\\/MIT//g" ${file}
    echo "Removed license in ${file}"
done
