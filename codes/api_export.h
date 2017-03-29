_Pragma("once");

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define API_EXPORT __attribute__ ((dllexport))
#else
#define API_EXPORT __declspec(dllexport) // Note: actually gcc seems to also supports this syntax.
#endif
#ifdef __GNUC__
#define API_IMPORT __attribute__ ((dllimport))
#else
#define API_IMPORT __declspec(dllimport) // Note: actually gcc seems to also supports this syntax.
#endif 
#define API_LOCAL
#else
#if __GNUC__ >= 4
#define API_EXPORT __attribute__ ((visibility ("default")))
#define API_IMPORT __attribute__ ((visibility ("default")))
#define API_LOCAL  __attribute__ ((visibility ("hidden")))
#else
#define API_EXPORT
#define API_IMPORT
#define API_LOCAL
#endif                                                                                                                                                                                                        
#endif
