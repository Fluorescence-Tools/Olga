#!/bin/sh
BUILDDIR=$1
DESTDIR=$2


if [ -z "$BUILDDIR" ]
then
	#BUILDDIR="./build-ubuntu"
	echo ERROR: BUILDDIR is not set
	exit 1
fi
if [ -z "$DESTDIR" ]
then
      DESTDIR="./Olga-ubuntu"
fi

mkdir -p "${DESTDIR}"

cat << EOF > "${DESTDIR}"/Olga.sh
#!/bin/bash
#Depends on boost and qt5. On ubuntu dependencies can be installed via:
#sudo apt-get install libboost-system1.71 libboost-date-time1.71 libgomp1 qt5-default
SCRPTDIR="\$( cd "\$( dirname "\${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
export LD_LIBRARY_PATH=\$LD_LIBRARY_PATH:\$SCRPTDIR
"\$SCRPTDIR/Olga" "\$@"
EOF

chmod +x "${DESTDIR}"/Olga.sh

cp "${BUILDDIR}/src/gui/Olga" "${DESTDIR}"/
cp "${BUILDDIR}/src/irmsd/libirmsd.so.1.0.0" "${DESTDIR}"/libirmsd.so.1

cp /usr/lib/libtng_io.so "${DESTDIR}"/
cp /usr/lib/libtng_io.so.1 "${DESTDIR}"/
cp /usr/lib/libasync++.so "${DESTDIR}"/
cp /usr/lib/libpteros.so "${DESTDIR}"/

cp src/*.json "${DESTDIR}"/
cp src/weighting_function.csv "${DESTDIR}"/
cp -r doc "${DESTDIR}"/
