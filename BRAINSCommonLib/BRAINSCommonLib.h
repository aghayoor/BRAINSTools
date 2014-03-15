/*
 * Here is where system computed values get stored.
 * These values should only change when the target compile platform changes.
 */
#ifndef __BRAINSCommonLib_h
#define __BRAINSCommonLib_h

#include <itkConfigure.h>

#if defined(WIN32) && !defined(BRAINSCommonLib_STATIC)
#pragma warning ( disable : 4275 )
#endif

/* #undef BUILD_SHARED_LIBS */

#ifndef BUILD_SHARED_LIBS
#define BRAINSCommonLib_STATIC
#endif

/* #undef BRAINS_DEBUG_IMAGE_WRITE */

/* #undef USE_DebugImageViewer */

/* #undef USE_ANTS */

/* #undef ITK_HAS_MGHIO */
extern void BRAINSRegisterAlternateIO(void);

#endif // __BRAINSCommonLib_h
