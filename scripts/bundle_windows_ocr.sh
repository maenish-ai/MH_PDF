#!/usr/bin/env bash
set -euo pipefail

dist=${1:?Windows staging directory required}
prefix=${MINGW_PREFIX:?Run under the MSYS2 UCRT64 shell}
test "${MSYSTEM}" = UCRT64
test -f "$dist/MaenPDF.exe"

# Follow the native PE imports recursively so each provider runs outside MSYS2.
packages=$(mktemp)
trap 'rm -f "$packages"' EXIT
bundle_executable() {
    local program=$1 destination=$2 file dependency imports
    local -a queue=("$prefix/bin/$program.exe")
    mkdir -p "$destination"
    while (("${#queue[@]}")); do
        file=${queue[0]}
        queue=("${queue[@]:1}")
        [[ ! -e "$destination/${file##*/}" ]] || continue
        cp "$file" "$destination/"
        pacman -Qqo "$file" >> "$packages"
        imports=$("$prefix/bin/objdump.exe" -p "$file")
        while IFS= read -r dependency; do
            # Windows supplies its own system DLLs. UCRT64 dependencies are
            # copied from the signed package installation.
            [[ ! -f "$prefix/bin/$dependency" ]] || queue+=("$prefix/bin/$dependency")
        done < <(awk '/DLL Name:/ {print $3}' <<< "$imports")
    done
}

bundle_executable tesseract "$dist/tools/tesseract"
bundle_executable qpdf "$dist/tools/qpdf"

data="$dist/tools/tesseract/tessdata"
mkdir -p "$data/configs"
# Tesseract embeds its searchable-PDF font. The MSYS2 build installs the
# PDF renderer configuration and the language data as separate files.
for file in ara.traineddata eng.traineddata osd.traineddata configs/pdf; do
    source="$prefix/share/tessdata/$file"
    cp "$source" "$data/$file"
    pacman -Qqo "$source" >> "$packages"
done

notices="$dist/tools/ocr-notices"
mkdir -p "$notices"
while IFS= read -r package; do
    pacman -Q "$package" >> "$notices/packages.txt"
    while IFS= read -r notice; do
        [[ -f "$notice" ]] || continue
        relative=${notice#"$prefix/share/licenses/"}
        mkdir -p "$notices/licenses/$(dirname "$relative")"
        cp "$notice" "$notices/licenses/$relative"
    done < <(pacman -Qlq "$package" | awk -v root="$prefix/share/licenses/" 'index($0, root) == 1')
done < <(sort -u "$packages")
