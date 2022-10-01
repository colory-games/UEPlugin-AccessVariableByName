#!/usr/bin/env bash
# require: bash version >= 4
# usage example: bash remove_code.sh src 4.27 output
set -eEu


SUPPORTED_VERSIONS=(
    "4.26.0" "4.27.0"
    "5.0.0"
)

function usage() {
    echo "Usage: bash replace_engine_version.sh <source-directory> <engine-version> <output-directory>"
}

if [ $# -ne 2 ]; then
    exit 1
fi

source_dir=${1}
engine_version=${2}
output_dir=${3}

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

mkdir -p "${output_dir}"

remove_start_regex="[^\S]*//[^\S]*@remove-start[^\S]+UE_VERSION=([0-9.,]+)"
remove_end_regex="[^\S]*//[^\S]*@remove-end"
enable_delete=0
for file in $(find "${source_dir}" -name "*.cpp" -or -name "*.h"); do
    out_file_path="${output_directory}/${file}"

    while IFS= read -r line; do
        if [[ "$line" =~ $remove_start_regex ]]; then
            versions=${BASH_REMATCH[1]}
	    versions=(${versions//,/ })

            for version in ${versions[@]}; do
                if [[ ${version} = ${engine_version} ]]; then
                    enable_delete=1
		    break
                fi
            done
        fi

	if [[ $enable_delete -eq 0 ]]; then
            echo "${line}" >> ${out_file_path}
        fi

        if [[ "$line" =~ $remove_end_regex ]]; then
            enable_delete=0
        fi

    done < "${file}"

    echo "Remove code in ${file}"
done
