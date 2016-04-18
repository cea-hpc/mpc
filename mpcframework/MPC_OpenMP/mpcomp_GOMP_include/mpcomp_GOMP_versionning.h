#ifndef __MPCOMP_GOMP_VERSIONNING_H__
#define __MPCOMP_GOMP_VERSIONNING_H__

#define MPCOMP_VERSION_SYMBOLS

#ifdef MPCOMP_VERSION_SYMBOLS

    #define str(x) #x
    //#define versionify(gomp_api_name, version_str, mpcomp_api_name)
    #define versionify(mpcomp_api_name, gomp_api_name, version_str) \
	__asm__(".symver " str(mpcomp_api_name) "," str(gomp_api_name) "@@" version_str);

#else // MPCOMP_USE_VERSION_SYMBOLS

    #define str(x)
    #define versionify(api_name, version_num, version_str) /* Nothing */

#endif // MPCOMP_USE_VERSION_SYMBOLS

#endif /* __MPCOMP_GOMP_VERSIONNING_H__ */
