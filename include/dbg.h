#ifndef DBG_H_
#define DBG_H_

#include <stdio.h>
#include <errno.h>
#include <string.h>

#ifdef NDEBUG
#define debug(...)
#else
// #define debug(M, ...) fprintf(stderr, "DEBUG %s:%d: " M "\n", __FILE__, __LINE__, ##__VA_ARGS__)
//  __func__ <- this is another string which contains the name of the function (works in GCC and MSVC)
#define debug(...) \
	do { fprintf(stderr, "DEBUG %s:%s:%d: ", __FILE__, __func__, __LINE__); \
	fprintf(stderr, ##__VA_ARGS__); \
	fprintf(stderr, "\n"); } while (0)
#endif

#define clean_errno() (errno == 0 ? "None" : strerror(errno))

//#define log_err(M, ...) fprintf(stderr, "[ERROR] (%s:%d: errno: %s) " M "\n", __FILE__, __LINE__, clean_errno(), ##__VA_ARGS__)

#ifndef STDERR_LOGGING
#define log_err(...) \
	fprintf(stderr, "[ERROR] (%s:%s:%d: errno: %s) ", __FILE__, __func__, __LINE__, clean_errno()); \
	fprintf(stderr, ##__VA_ARGS__)
#endif

#ifdef STDERR_LOGGING
#define log_generic(L, ...) \
	fprintf(stderr, "[" L "] (%s:%s:%d: errno: %s) ", __FILE__, __func__, __LINE__, clean_errno()); \
	fprintf(stderr, ##__VA_ARGS__)

#define log_err(...) log_generic("ERROR", __VA_ARGS__)

#define log_warn(...) log_generic("WARN", __VA_ARGS__)

#define log_info(...) fprintf(stderr, "[INFO] (%s:%d) " M "\n", __FILE__, __LINE__); \
	fprintf(stderr, ##__VA_ARGS__)
#endif

#define check(A, ...) if(!(A)) { log_err(__VA_ARGS__); errno=0; goto error; }

#define sentinel(...)  { log_err(__VA_ARGS__); errno=0; goto error; }

#define check_mem(A) check((A), "%s", "Out of memory.")

#define check_debug(A, ...) if(!(A)) { debug(__VA_ARGS__); errno=0; goto error; }

#endif /* DBG_H_ */

