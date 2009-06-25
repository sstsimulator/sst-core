#!/bin/sh

echo "Generating configure files..."

autoreconf --install --symlink --warnings=gnu,obsolete,override,portability,no-obsolete && \
  exit 0

echo "It appears that configure file generation failed.  Sorry :(."
exit 1
