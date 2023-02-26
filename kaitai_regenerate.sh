#!/bin/bash

UTARGET=$(uname)

# Determine platform
if [ "$UTARGET" = "Darwin" ]; then
  export UPLATFORM="mac"
  export UFIND="find -E"
  export UFINDOPT=""
  export USEDOPT="''"
elif [ "$UTARGET" = "Linux" ]; then
  export UPLATFORM="linux_$(uname -m)"
  export UFIND="find"
  export UFINDOPT="-regextype posix-extended"
  export USEDOPT=""
else
  export UPLATFORM="$UTARGET"
  echo "Please run this script on Linux or macOS"
fi

# Generate
echo "Attempting to to generate parsers from Kaitai KSY files on ${UPLATFORM}..."
kaitai-struct-compiler --target cpp_stl --cpp-standard 11 --outdir common/generated common/ksy/* || exit 1

# Show generated files
${UFIND} common/generated ${UFINDOPT} \
 -regex '.*\.(cpp|h)' \
 -print || exit 1

# Replace global includes for kaitai with local ones (<> -> "")
${UFIND} common/generated ${UFINDOPT} \
 -regex '.*\.(cpp|h)' \
 -exec sed -i ${USEDOPT} '/^#include <kaitai/s/[<>]/\"/g' {} + || exit 1

# Add .. to the include path for kaitai includes
${UFIND} common/generated ${UFINDOPT} \
 -regex '.*\.(cpp|h)' \
 -exec sed -i ${USEDOPT} '/^#include \"kaitai\//s/kaitai\//..\/kaitai\//g' {} + || exit 1

# Suppress "p__root - unused parameter" warning
${UFIND} common/generated ${UFINDOPT} \
 -regex '.*\.(cpp)' \
 -exec sed -i ${USEDOPT} '/^    m__root = this;/s/;/; (void)p__root;/g' {} + || exit 1

# Add uint64_t to enum structure_ids_t
${UFIND} common/generated ${UFINDOPT} \
 -regex '.*\.(h)' \
 -exec sed -i ${USEDOPT} '/^    enum structure_ids_t {/s/{/: uint64_t {/g' {} + || exit 1

# Suppress type downcast warning in ami_nvar.cpp
${UFIND} common/generated ${UFINDOPT} \
 -name 'ami_nvar.cpp' \
 -exec sed -i ${USEDOPT} 's/_offset = _io()->pos();/_offset = (int32_t)_io()->pos();/g' {} + || exit 1

exit 0
