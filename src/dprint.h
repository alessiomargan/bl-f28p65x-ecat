#ifndef __DPRINT_H__
#define __DPRINT_H__

#ifdef ESC_DEBUG
	extern int printf(const char *pcString, ...);
	#define DPRINT(...) printf ("esc_dbg: "__VA_ARGS__) /* TODO */
#else
	#define DPRINT(...)
#endif  /* DEBUG */


#endif
