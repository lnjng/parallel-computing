#!/usr/bin/bash
#
# DO NOT EDIT THIS FILE
#

function get_heatsim() {
    local project_dir="$1"
    local build_dir

    mkdir -p "${project_dir}/build"
    build_dir=$(mktemp -d -p "${project_dir}/build/")
    cd "${build_dir}" || exit 1

    if command -v cmake3 &> /dev/null; then
        cmake3 "${project_dir}" -DCMAKE_BUILD_TYPE=Release 1>&2
    elif command -v cmake &> /dev/null; then
        cmake "${project_dir}" -DCMAKE_BUILD_TYPE=Release 1>&2
    else
        echo "cmake not found, exiting..."
        exit 1
    fi

    make 1>&2
    readlink -e heatsim
}

function get_results_directory() {
    local project_dir="$1"

    mkdir -p "${project_dir}/results"

    local results_dir
    results_dir=$(readlink -f "${project_dir}/results")

    if [[ ! -f "${results_dir}/data.csv" ]]; then
        echo -e "job-id\tnode-count\tcpu-count\tdim-x\tdim-y\titerations\timage\ttime" > "${results_dir}/data.csv"
    fi

    echo "${results_dir}"
}

function get_output_image_name() {
    local dim_x="$1"
    local dim_y="$2"
    local job_id="$3"
    local iterations="$4"
    local image="$5"

    echo "$(basename "${image}")-${dim_x}-${dim_y}-${iterations}-(${job_id}).png"
}

function run_heatsim_single() {
    local heatsim_bin="$1"
    local dim_x="$2"
    local dim_y="$3"
    local job_id="$4"
    local iterations="$5"
    local input_image="$6"
    local output_image="$7"
    local results_time="$8"

    local hostfile
    if [[ "${job_id}" == "none" ]]; then
        hostfile=(--use-hwthread-cpus)
    else
        hostfile=()
    fi

    local cpu_count
    cpu_count=$(echo "${dim_x} * ${dim_y}" | bc)

    /usr/bin/time -a -o "${results_time}" -f "%e" \
        mpirun "${hostfile[@]}" -np "${cpu_count}" "${heatsim_bin}" \
            --dim-x "${dim_x}" \
            --dim-y "${dim_y}" \
            --iterations "${iterations}" \
            --input "${input_image}" \
            --output "${output_image}"
}

function save_data() {
    local dim_x="$1"
    local dim_y="$2"
    local job_id="$3"
    local node_count="$4"
    local iterations="$5"
    local image="$6"
    local results_time="$7"
    local results="$8"

    local cpu_count
    cpu_count=$(echo "${dim_x} * ${dim_y}" | bc)

    local average_time
    average_time=$(awk '{s+=$1}END{print "",s/NR}' RS=" " "${results_time}")

    local temp_file
    temp_file=$(mktemp)

    printf "%s\t%s\t%d\t%d\t%d\t%d\t%s\t%4.3f\n" "${job_id}" "${node_count}" "${cpu_count}" \
        "${dim_x}" "${dim_y}" "${iterations}" "$(basename "${image}")" "${average_time}" > "${temp_file}"
    flock -w 10 "${results}" -c "cat ${temp_file} >> ${results}"

    rm -v "${temp_file}"
}

function run_heatsim() {
    local dim_x="$1"
    local dim_y="$2"
    local job_id="$3"
    local node_count="$4"
    local project_dir="$5"

    local all_iterations=(1 2000)
    local all_images=(
        "${project_dir}/images/earth-small.png"
        "${project_dir}/images/earth-medium.png"
        "${project_dir}/images/earth-large.png"
        "${project_dir}/images/earth-xlarge.png"
    )
    local repeat=3

    local heatsim_bin
    heatsim_bin=$(get_heatsim "${project_dir}")

    local results_dir
    results_dir=$(get_results_directory "${project_dir}")

    for iterations in "${all_iterations[@]}"; do
        for input_image in "${all_images[@]}"; do
            local output_image
            output_image=$(get_output_image_name "${dim_x}" "${dim_y}" "${job_id}" "${iterations}" "${input_image}")
            output_image="${results_dir}/${output_image}"

            local results_temp
            results_temp=$(mktemp)

            for (( i=0; i <= repeat; i++)); do
                run_heatsim_single "${heatsim_bin}" "${dim_x}" "${dim_y}" "${job_id}" \
                    "${iterations}" "${input_image}" "${output_image}" "${results_temp}"
            done

            save_data "${dim_x}" "${dim_y}" "${job_id}" "${node_count}" "${iterations}" \
                "${input_image}" "${results_temp}" "${results_dir}/data.csv"

            rm -v "${results_temp}"
        done
    done
}

function run() {
    local dim_x=1
    local dim_y=1
    local job_id="none"
    local node_count="unknown"
    local project_dir

    project_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." &> /dev/null && pwd)

    while [[ $# -gt 0 ]]; do
        case "$1" in
        --dim-x)
            dim_x="$2"
            shift
            ;;
        --dim-y)
            dim_y="$2"
            shift
            ;;
        --job-id)
            job_id="$2"
            shift
            ;;
        --node-count)
            node_count="$2"
            shift
            ;;
        *)
            echo "Unknown argument: $1"
            exit 1
        esac
        shift
    done

    run_heatsim "${dim_x}" "${dim_y}" "${job_id}" ${node_count} "${project_dir}"
}

run "$@"