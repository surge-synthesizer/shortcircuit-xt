#!/bin/bash
# usage: make_deb.sh <product dir> <source dir> <output .deb> <version>

set -e

INDIR=$1
SOURCEDIR=$2
OUTPUT_DEB=$3
VERSION=$4

PRODUCT_NAME="Shortcircuit XT"
PACKAGE_NAME=shortcircuit-xt

# a missing git hash leaves a trailing hyphen, which dpkg rejects
VERSION=${VERSION%-}

# releases are X.Y.Z; nightlies sort above the newest release tag and below the next release
TAG_PREFIX=v
if [[ ${VERSION} =~ ^(${TAG_PREFIX})?([0-9]+\.[0-9]+\.[0-9]+)$ ]]; then
    DEB_VERSION=${BASH_REMATCH[2]}
else
    BASE=${DEB_BASE_VERSION:-0.0.0}
    if ! [[ ${BASE} =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        echo "ERROR: DEB_BASE_VERSION '${BASE}' is not X.Y.Z"
        exit 1
    fi
    LAST_REL=$(git -C "${SOURCEDIR}" tag --list "${TAG_PREFIX}*" \
        | sed -nE "s/^${TAG_PREFIX}([0-9]+\.[0-9]+\.[0-9]+)$/\1/p" | sort -V | tail -n 1)
    COUNT=$(git -C "${SOURCEDIR}" rev-list --count HEAD || echo 0)
    DATE=$(TZ=UTC git -C "${SOURCEDIR}" log -1 --format=%cd --date=format-local:%Y%m%d || date -u +%Y%m%d)
    HASH=$(git -C "${SOURCEDIR}" rev-parse --short HEAD || echo nohash)
    if [[ $(git -C "${SOURCEDIR}" rev-parse --is-shallow-repository) == true ]]; then
        echo "WARNING: shallow clone; nightly deb versions will not order correctly"
    fi
    if [[ -z ${LAST_REL} ]] || dpkg --compare-versions "${BASE}" gt "${LAST_REL}"; then
        DEB_VERSION="${BASE}~nightly.${DATE}.${COUNT}.${HASH}"
    else
        DEB_VERSION="${LAST_REL}+nightly.${DATE}.${COUNT}.${HASH}"
    fi
fi
if ! [[ ${DEB_VERSION} =~ ^[0-9][A-Za-z0-9.+~]*$ ]]; then
    echo "ERROR: '${DEB_VERSION}' is not a valid deb version"
    exit 1
fi
echo "DEB VERSION is ${DEB_VERSION}"

WORK_DIR=$(mktemp -d)
trap 'rm -rf "${WORK_DIR}"' EXIT
# dpkg-shlibdeps wants the tree at debian/<package> next to a debian/control
PKG="${WORK_DIR}/debian/${PACKAGE_NAME}"
DOC_DIR="${PKG}/usr/share/doc/${PACKAGE_NAME}"

mkdir -p "${PKG}/DEBIAN" "${PKG}/usr/bin" "${PKG}/usr/lib/clap" "${PKG}/usr/lib/vst3" "${DOC_DIR}"
touch "${WORK_DIR}/debian/control"

if [[ -f "${INDIR}/${PRODUCT_NAME}.clap" ]]; then
    cp "${INDIR}/${PRODUCT_NAME}.clap" "${PKG}/usr/lib/clap/"
fi
if [[ -d "${INDIR}/${PRODUCT_NAME}.vst3" ]]; then
    cp -r "${INDIR}/${PRODUCT_NAME}.vst3" "${PKG}/usr/lib/vst3/"
fi
if [[ -f "${INDIR}/${PRODUCT_NAME}" ]]; then
    cp "${INDIR}/${PRODUCT_NAME}" "${PKG}/usr/bin/"
fi

cp "${SOURCEDIR}/resources/LICENSE.agpl3" "${DOC_DIR}/copyright"

DATE=$(date --rfc-email)
MSG=$(git -C "${SOURCEDIR}" log -n 1 --pretty="%s (git hash %H)" || echo "unknown commit")
cat <<EOT > "${DOC_DIR}/changelog.Debian"
${PACKAGE_NAME} (${DEB_VERSION}) stable; urgency=medium

  * ${MSG}
  * For more details, see: https://github.com/surge-synthesizer/shortcircuit-xt

 -- Surge Synthesizer Team <noreply@github.com>  ${DATE}
EOT
gzip -9 -n "${DOC_DIR}/changelog.Debian"

find "${PKG}" -type d -exec chmod 0755 {} +
find "${PKG}/usr" -type f -exec chmod 0644 {} +
if [[ -f "${PKG}/usr/bin/${PRODUCT_NAME}" ]]; then
    chmod 0755 "${PKG}/usr/bin/${PRODUCT_NAME}"
fi

# linked libs, with version floors, read from the binaries themselves
SHLIBS=""
ELF_FILES=()
while IFS= read -r -d '' f; do
    if [[ "$(od -An -tx1 -N4 "$f" | tr -d ' \n')" == 7f454c46 ]]; then
        ELF_FILES+=(-e"$f")
    fi
done < <(find "${PKG}/usr" -type f -print0)
if command -v dpkg-shlibdeps > /dev/null && [[ ${#ELF_FILES[@]} -gt 0 ]]; then
    SHLIBS=$(cd "${WORK_DIR}" && dpkg-shlibdeps -O "${ELF_FILES[@]}" | sed -n 's/^shlibs:Depends=//p') || SHLIBS=""
fi
if [[ -z "${SHLIBS}" ]]; then
    echo "WARNING: no dpkg-shlibdeps result; using a static Depends floor"
    SHLIBS="libc6 (>= 2.27), libstdc++6, libgcc-s1, libfontconfig1, libfreetype6, libx11-6"
fi

# juce dlopens these for the ui and fails without any of them
DEPENDS="${SHLIBS}"
for p in libx11-6 libxext6 libxcursor1 libxinerama1 libxrandr2; do
    if ! [[ ", ${DEPENDS}," =~ ,\ ${p}[\ ,] ]]; then
        DEPENDS="${DEPENDS}, ${p}"
    fi
done

# the standalone dlopens jack, its only audio backend in ci, and alsa for midi
cat <<EOT > "${PKG}/DEBIAN/control"
Source: ${PACKAGE_NAME}
Package: ${PACKAGE_NAME}
Version: ${DEB_VERSION}
Architecture: $(dpkg --print-architecture)
Maintainer: surgeteam <noreply@github.com>
Depends: ${DEPENDS}
Recommends: libjack-jackd2-0 | libjack0, libasound2, xdg-utils, libxss1, zenity | kdialog
Provides: vst-plugin
Section: sound
Priority: optional
Homepage: https://github.com/surge-synthesizer/shortcircuit-xt
Sndstation-Kind: clap-plugin
Description: Sampler virtual instrument
 Shortcircuit XT includes CLAP and VST3 instrument formats for use in
 compatible hosts and a standalone executable.
EOT

cat "${PKG}/DEBIAN/control"
echo "----- PACKAGE CONTENTS -----"
find "${PKG}/usr" -print

# xz so older dpkg without zstd can still read it
dpkg-deb --verbose --root-owner-group -Zxz --build "${PKG}" "${OUTPUT_DEB}"
