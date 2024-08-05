#!/bin/bash
# argv[1] - resourcepack dir
# argv[2] - voxelius font name

export LANG=C.UTF-8
export LANGUAGE=C.UTF-8
export LC_ALL=C.UTF-8

cd $(dirname $(realpath $(dirname ${0})))

cp ${1}/assets/minecraft/font/glyph_sizes.bin ${PWD}/assets/fonts/${2}.glyph_sizes.bin

for i in $(seq 0 255); do
    lc=$(printf "%02x" ${i})
    uc=$(printf "%02X" ${i})
    printf "copying page 0x%s\n" ${uc}
    cp "${1}/assets/minecraft/textures/font/unicode_page_${lc}.png" "${PWD}/assets/fonts/${2}.page.${uc}.png"
done
