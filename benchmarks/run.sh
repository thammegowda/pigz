#!/usr/bin/env bash
MYDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
MYDIR="$( dirname "${BASH_SOURCE[0]}" )"
RUNSCRIPT="$MYDIR/run.py"

ROOT=$MYDIR

# pick one file from https://paracrawl.eu/
DATA_URL="https://web-language-models.s3.amazonaws.com/paracrawl/release9/en-bg/en-bg.txt.gz"
DATA_COMP="$ROOT/sample.txt.gz"
DATA_DEC="$ROOT/sample.txt"

TOOLS=(
    wget
    hyperfine   # for benchmarking,  https://github.com/sharkdp/hyperfine
)

log(){
    echo -e "$(date -Is) - $@" >&2
}

check_tools(){
    for tool in "${TOOLS[@]}"; do
        if ! command -v $tool &> /dev/null; then
            log "Error: $tool is not installed or missing in PATH"
            if [[ "$tool" == "hyperfine" ]]; then
                log "https://github.com/sharkdp/hyperfine. On ubuntu, you may try 'sudo apt install hyperfine'"
            fi
            exit 1
        fi
    done
    [[ -f $RUNSCRIPT ]] || { log "Error: $RUNSCRIPT not found"; exit 1; }
}

get_data(){
    set -eu
    [[ -s $DATA_COMP ]] || {
    wget  $DATA_URL -O $DATA_COMP.tmp && mv $DATA_COMP{.tmp,}
    }
    [[ -s $DATA_DEC ]] || {
        gzip -dc $DATA_COMP > $DATA_DEC.tmp && mv $DATA_DEC{.tmp,}
    }
}


main(){
    set -eu
    check_tools
    get_data
    log "Running benchmarks..."
    #hf_args="--runs 2 --show-output -L impl pigz,gzip,pigz_subproc"
    hf_args="--runs 2 --show-output -L impl pigz_subproc,pigz,gzip"
    hyperfine $hf_args --export-csv results.comp.csv  "python $RUNSCRIPT {impl} compress $DATA_DEC /dev/null"
    hyperfine $hf_args --export-csv results.decomp.csv "python $RUNSCRIPT {impl} decompress $DATA_COMP /dev/null"
}
main "$@"