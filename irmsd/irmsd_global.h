#ifndef IRMSD_GLOBAL_H
#define IRMSD_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(IRMSD_LIBRARY)
#  define IRMSDSHARED_EXPORT Q_DECL_EXPORT
#else
#  define IRMSDSHARED_EXPORT Q_DECL_IMPORT
#endif

#endif // IRMSD_GLOBAL_H
