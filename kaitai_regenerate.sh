#!/bin/bash

UTARGET=$(uname)

# Determine platform
if [ "$UTARGET" = "Darwin" ]; then
  export UPLATFORM="mac"
elif [ "$UTARGET" = "Linux" ]; then
  export UPLATFORM="linux_$(uname -m)"
elif [ "${UTARGET/MINGW32/}" != "$UTARGET" ]; then
  export UPLATFORM="win32"
else
  # Fallback to something...
  export UPLATFORM="$UTARGET"
fi

# Generate
echo "Attempting to to generate parsers from Kaitai KSY files on ${UPLATFORM}..."
kaitai-struct-compiler --target cpp_stl --outdir common/generated common/ksy/* || exit 1

# Show generated files
find -E common/generated \
 -regex '.*\.(cpp|h)' \
 -print || exit 1

# Replace global includes for kaitai with local ones (<> -> "")
find -E common/generated \
 -regex '.*\.(cpp|h)' \
 -exec sed -i '' '/^#include <kaitai/s/[<>]/\"/g' {} + || exit 1

# Add .. to the include path for kaitai includes
find -E common/generated \
 -regex '.*\.(cpp|h)' \
 -exec sed -i '' '/^#include \"kaitai\//s/kaitai\//..\/kaitai\//g' {} + || exit 1

# Suppress "p__root - unused parameter" warning
find -E common/generated \
 -regex '.*\.(cpp)' \
 -exec sed -i '' '/^    m__root = this;/s/;/; (void)p__root;/g' {} + || exit 1

exit 0
