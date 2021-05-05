#!/bin/sh
BUILDDIR=$1
SYSLIBPREFIX=$2
DESTDIR=$3


if [ -z "$BUILDDIR" ]
then
	#BUILDDIR="/build-win64"
	echo ERROR: BUILDDIR is not set
	exit 1
fi
if [ -z "$SYSLIBPREFIX" ]
then
      SYSLIBPREFIX="/usr/x86_64-w64-mingw32"
fi
if [ -z "$DESTDIR" ]
then
      DESTDIR="./Olga"
fi

mkdir -p "${DESTDIR}"/platforms/
mkdir -p "${DESTDIR}"/imageformats/
mkdir -p "${DESTDIR}"/iconengines/
cp "${BUILDDIR}"/src/*/release/*.exe "${DESTDIR}"/
cp "${BUILDDIR}"/src/irmsd/release/irmsd.dll "${DESTDIR}"/
cp "${SYSLIBPREFIX}"/bin/lib{async++,bz2-1,freetype-6,gcc_s_seh-1,gcc_s_sjlj-1,glib-2.0-0,harfbuzz-0,iconv-2,intl-8,pcre2-16-0,fmt,pcre-1,pcre16-0,png16-16,pteros,tng_io,stdc++-6,winpthread-1,boost_filesystem,graphite2,gomp-1,ssp-0,spdlog}.dll "${DESTDIR}"/
cp "${SYSLIBPREFIX}"/bin/{Qt5Core,Qt5Gui,Qt5Multimedia,Qt5Network,Qt5Svg,Qt5Widgets,zlib1}.dll "${DESTDIR}"/
cp "${SYSLIBPREFIX}"/lib/qt/plugins/platforms/qwindows.dll "${DESTDIR}"/platforms/
cp "${SYSLIBPREFIX}"/lib/qt/plugins/imageformats/{qsvg,qico,qtiff,qjpeg}.dll "${DESTDIR}"/imageformats/
cp "${SYSLIBPREFIX}"/lib/qt/plugins/iconengines/qsvgicon.dll "${DESTDIR}"/iconengines/
cp src/*.json "${DESTDIR}"/
cp src/weighting_function.csv "${DESTDIR}"/
cp -r doc "${DESTDIR}"/
