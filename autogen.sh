#!/bin/sh
# you can either set the environment variables AUTOCONF, AUTOHEADER, AUTOMAKE,
# ACLOCAL, AUTOPOINT and/or LIBTOOLIZE to the right versions, or leave them
# unset and get the defaults

autoreconf --verbose --force --install --make || {
 echo 'autogen.sh failed';
 exit 1;
}

if test -n "$NOCONFIGURE"; then
 echo 'Not running configure due to NOCONFIGURE env var.'
else
 ./configure || {
  echo 'configure failed';
  exit 1;
 }
fi

echo
echo "Now type 'make' to compile this module."
echo
