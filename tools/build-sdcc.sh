#!/usr/bin/env bash
set -euo pipefail

version=4.6.0
archive="sdcc-src-${version}.tar.bz2"
url="https://downloads.sourceforge.net/project/sdcc/sdcc/${version}/${archive}"
sha256="5fd6a93e5997ce01756868fe35e441095cfb637894a80c262514a634094973b6"
boost_version=1.89.0
boost_dir_version=1_89_0
boost_archive="boost_${boost_dir_version}.tar.bz2"
boost_url="https://archives.boost.io/release/${boost_version}/source/${boost_archive}"
boost_sha256="85a33fa22621b4f314f8e85e1a5e2a9363d22e4f4992925d4bb3bc631b5a0c7a"
prefix="${SDCC_PREFIX:-$HOME}"
work="${SDCC_BUILD_DIR:-${TMPDIR:-/tmp}/sdcc-${version}-build}"
cache="${XDG_CACHE_HOME:-$HOME/.cache}/kiss-tnc2-toolchain"

if test -x "$prefix/bin/sdcc" &&
   "$prefix/bin/sdcc" --version 2>/dev/null | grep -q " ${version} "; then
    echo "SDCC ${version} is already installed in $prefix/bin."
    exit 0
fi

for tool in make sha256sum tar; do
    command -v "$tool" >/dev/null 2>&1 || {
        echo "Required build tool '$tool' is missing." >&2
        exit 2
    }
done

rm -rf "$work"
mkdir -p "$work" "$cache"
if command -v curl >/dev/null 2>&1; then
    curl --fail --location --retry 3 --continue-at - \
        --output "$cache/$archive" "$url"
elif command -v wget >/dev/null 2>&1; then
    wget --continue --output-document="$cache/$archive" "$url"
else
    echo "Either curl or wget is required to download SDCC." >&2
    exit 2
fi

printf '%s  %s\n' "$sha256" "$cache/$archive" | sha256sum --check -
tar -xjf "$cache/$archive" -C "$work"
src="$work/sdcc-${version}"

# SDCC's Z80 register allocator uses the header-only Boost Graph library.
# AlmaLinux does not provide the development package on all installations,
# so use a pinned source archive when the system headers are unavailable.
boost_cppflags=
if ! printf '#include <boost/graph/adjacency_list.hpp>\n' |
     "${CXX:-c++}" -x c++ -E - >/dev/null 2>&1; then
    if command -v curl >/dev/null 2>&1; then
        curl --fail --location --retry 3 --continue-at - \
            --output "$cache/$boost_archive" "$boost_url"
    else
        wget --continue --output-document="$cache/$boost_archive" "$boost_url"
    fi
    printf '%s  %s\n' "$boost_sha256" "$cache/$boost_archive" |
        sha256sum --check -
    tar -xjf "$cache/$boost_archive" -C "$work"
    boost_cppflags="-I$work/boost_${boost_dir_version}"
fi

cd "$src"
CPPFLAGS="$boost_cppflags ${CPPFLAGS:-}" ./configure \
    --prefix="$prefix" \
    --disable-doc \
    --disable-non-free \
    --disable-device-lib \
    --disable-ucsim \
    --disable-ds390-port \
    --disable-ds400-port \
    --disable-hc08-port \
    --disable-s08-port \
    --disable-mcs51-port \
    --disable-pic14-port \
    --disable-pic16-port \
    --disable-stm8-port \
    --disable-pdk13-port \
    --disable-pdk14-port \
    --disable-pdk15-port \
    --disable-pdk16-port \
    --disable-mos6502-port \
    --disable-mos65c02-port \
    --disable-r2k-port \
    --disable-r2ka-port \
    --disable-r3ka-port \
    --disable-r4k-port \
    --disable-r5k-port \
    --disable-r6k-port \
    --disable-sm83-port \
    --disable-tlcs90-port \
    --disable-ez80_z80-port \
    --disable-z80n-port \
    --disable-r800-port \
    --without-ccache

make -j"${JOBS:-$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 2)}"
make install

echo "Installed SDCC ${version} in $prefix/bin."
echo "Add it to this shell with: export PATH=\"$prefix/bin:\$PATH\""
