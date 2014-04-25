#ifdef _MSC_VER
#pragma once
#endif

#ifndef __DNT_CORE_XSYSTEM_DETECTION_H__
#define __DNT_CORE_XSYSTEM_DETECTION_H__

// The operating system, must be one of: (X_OS_x)
//      DARWIN          - Darwin OS (synonym for R_OS_MAC)
//      MAC             - OS X or iOS (synonym for R_OS_DARWIN)
//      MACX            - OS X
//      IOS             - iOS
//      MSDOS           - MS-DOS and Windows
//      OS2             - OS/2
//      OS2EMX          - XFree86 on OS/2 (not PM)
//      WIN32           - Win32 (Windows 2000/XP/Vista/7 and Windows Server 2003/2008)
//      WINCE           - WinCE (Windows CE 5.0)
//      CYGWIN          - Cygwin
//      SOLARIS         - Sun Solaris
//      HPUX            - HP-UX
//      ULTRIX          - DEC Ultrix
//      LINUX           - Linux
//      FREEBSD         - FreeBSD
//      NETBSD          - NetBSD
//      OPENBSD         - OpenBSD
//      BSDI            - BSD/OS
//      IRIX            - SGI Irix
//      OSF             - HP Tru64 UNIX
//      SCO             - SCO OpenServer 5
//      UNIXWARE        - UnixWare 7, Open UNIX 8
//      AIX             - AIX
//      HURD            - GNU Hurd
//      DGUX            - DG/UX
//      RELIANT         - Reliant UNIX
//      DYNIX           - DYNIX/ptx
//      QNX             - QNX
//      QNX6            - QNX RTP 6.1
//      LYNX            - LynxOS
//      BSD4            - Any BSD 4.4 system
//      UNIX            - Any UNIX/BSD/SYSVsystem
//      ANDROID         - Android platform

#if defined(__APPLE__) && (defined(__GNUC__) || defined(__xlC__) || defined(__xlc__))
#define X_OS_DARWIN
#define X_OS_BSD4
#ifdef __LP64__
#define X_OS_DARWIN64
#else
#define X_OS_DARWIN32
#endif
#elif defined(ANDROID)
#define X_OS_ANDROID
#define X_OS_LINUX
#elif defined(__CYGWIN__)
#define X_OS_CYGWIN
#elif !defined(SAG_COM) && (defined(WIN64) || defined(_WIN64) || defined(__WIN64__))
#define X_OS_WIN32
#define X_OS_WIN64
#elif !defined(SAG_COM) && (defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__))
#if defined(WINCE) || defined(_WIN32_WCE)
#define X_OS_WINCE
#else
#define X_OS_WIN32
#endif
#elif defined(__sun) || defined(sun)
#define X_OS_SOLARIS
#elif defined(hpux) || defined(__hpux)
#define X_OS_HPUX
#elif defined(__ultrix) || defined(ultrix)
#define X_OS_ULTRIX
#elif defined(sinix)
#define X_OS_RELIANT
#elif defined(__native_client__)
#define X_OS_NACL
#elif defined(__linux__) || defined(__linux)
#define X_OS_LINUX
#elif defined(__FreeBSD__) || defined(__DragonFly__)
#define X_OS_FREEBSD
#define X_OS_BSD4
#elif defined(__NetBSD__)
#define X_OS_NETBSD
#define X_OS_BSD4
#elif defined(__OpenBSD__)
#define X_OS_OPENBSD
#define X_OS_BSD4
#elif defined(__bsdi__)
#define X_OS_BSDI
#define X_OS_BSD4
#elif defined(__sgi)
#define X_OS_IRIX
#elif defined(__osf__)
#define X_OS_OSF
#elif defined(__AIX)
#define X_OS_AIX
#elif defined(__Lynx__)
#define X_OS_LYNX
#elif defined(__GNUC__)
#define X_OS_HURD
#elif defined(__QNXNTO__)
#define X_OS_QNX
#elif defined(_SEQUENT_)
#define X_OS_DYNIX
#elif defined(_SCO_DS) // SCO OpenServer 5 + GCC
#define X_OS_SCO
#elif defined(__USLC__) // all SCO platforms + UDK or OUDK
#define X_OS_UNIXWARE
#elif defined(__svr4__) && defined(i386) // Open UNIX 8 + GCC
#define X_OS_UNIXWARE
#elif defined(__INTEGRITY)
#define X_OS_INTEGRITY
#elif defined(VXWORKS) // there is no "real" VxWorks define - this has to be set in the DEFINE!
#define X_OS_VXWORKS
#elif defined(__MAKEDEPEND__)
#else
#error "DNT has not been ported to this OS"
#endif

#if defined(X_OS_WIN32) || defined(X_OS_WIN64) || defined(X_OS_WINCE)
#define X_OS_WIN
#endif

#if defined(X_OS_DARWIN)
#define X_OS_MAC
#if defined(X_OS_DARWIN64)
#define X_OS_MAC64
#elif defined(X_OS_DARWIN32)
#define X_OS_MAC32
#endif
#include <TargetConditional.h>
#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
#define X_OS_IOS
#else
#define X_OS_MACX
#endif
#endif

#if defined(X_OS_WIN)
#undef X_OS_UNIX
#elif !defined(X_OS_UNIX)
#define X_OS_UNIX
#endif

#endif // __DNT_CORE_XSYSTEM_DETECTION_H__
