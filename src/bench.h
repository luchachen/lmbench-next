/*
 * $Id$
 */
#ifndef _BENCH_H
#define _BENCH_H

#ifdef WIN32
#include <windows.h>
typedef unsigned char bool_t;
#endif

#include	<assert.h>
#include        <ctype.h>
#include        <stdio.h>
#ifndef WIN32
#include        <unistd.h>
#endif
#include        <stdlib.h>
#include        <fcntl.h>
#include        <signal.h>
#include        <errno.h>
#ifndef WIN32
#include        <strings.h>
#endif
#include        <sys/types.h>
#ifndef WIN32
#include        <sys/mman.h>
#endif
#include        <sys/stat.h>
#ifndef WIN32
#include        <sys/wait.h>
#include        <sys/time.h>
#include        <sys/socket.h>
#include        <sys/un.h>
#include        <sys/resource.h>
#include	<rpc/rpc.h>
#endif
#include	"stats.h"
#include	"timing.h"
#include	"lib_tcp.h"
#include	"lib_udp.h"
#include	"lib_unix.h"


#ifdef	DEBUG
#	define		debug(x) fprintf x
#else
#	define		debug(x)
#endif
#ifdef	NO_PORTMAPPER	/* not recently tested */
#define	TCP_XACT	-3962
#define	TCP_CONTROL	-3963
#define	TCP_DATA	-3964
#define	TCP_CONNECT	-3965
#else
#define	TCP_XACT	(u_long)404039	/* XXX - unregistered */
#define	TCP_CONTROL	(u_long)404040	/* XXX - unregistered */
#define	TCP_DATA	(u_long)404041	/* XXX - unregistered */
#define	TCP_CONNECT	(u_long)404042	/* XXX - unregistered */
#define	UDP_XACT 	(u_long)404032	/* XXX - unregistered */
#define	UDP_DATA 	(u_long)404033	/* XXX - unregistered */
#define	VERS		(u_long)1
#endif

#define	UNIX_CONTROL	"/tmp/lmbench.ctl"
#define	UNIX_DATA	"/tmp/lmbench.data"
#define	UNIX_LAT	"/tmp/lmbench.lat"

/*
 * socket send/recv buffer optimizations
 */
#define	SOCKOPT_READ	0x0001
#define	SOCKOPT_WRITE	0x0002
#define	SOCKOPT_RDWR	0x0003
#define	SOCKOPT_PID	0x0004
#define	SOCKOPT_REUSE	0x0008
#define	SOCKOPT_NONE	0

#ifndef SOCKBUF
#define	SOCKBUF		(1024*1024)
#endif

#ifndef	XFERSIZE
#define	XFERSIZE	(64*1024)	/* all bandwidth I/O should use this */
#endif

#if defined(SYS5) || defined(WIN32)
#define	bzero(b, len)	memset(b, 0, len)
#define	bcopy(s, d, l)	memcpy(d, s, l)
#define	rindex(s, c)	strrchr(s, c)
#endif
#define	gettime		usecs_spent
#define	streq		!strcmp
#define	ulong		unsigned long

#ifdef WIN32
#include <process.h>
#define getpid _getpid
#endif

#define	SMALLEST_LINE	32		/* smallest cache line size */
#define	TIME_OPEN2CLOSE

#define	GO_AWAY	signal(SIGALRM, exit); alarm(60 * 60);
#define	REAL_SHORT	   50000
#define	SHORT	 	 1000000
#define	MEDIUM	 	 2000000
#define	LONGER		 7500000	/* for networking data transfers */
#define	ENOUGH		REAL_SHORT

#define	TRIES		11

typedef struct {
	uint64 u;
	uint64 n;
} value_t;

typedef struct {
	int	N;
	value_t	v[TRIES];
} result_t;
void    insertinit(result_t *r);
void    insertsort(uint64, uint64, result_t *);
void	save_median();
void	save_minimum();
void	save_results(result_t *r);
void	get_results(result_t *r);


#define	BENCHO(loop_body, overhead_body, enough) { 			\
	int 		__i, __N;					\
	double 		__oh;						\
	result_t 	__overhead, __r;				\
	insertinit(&__overhead); insertinit(&__r);			\
	__N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;\
	if (enough < LONGER) {loop_body;} /* warm the cache */		\
	for (__i = 0; __i < __N; ++__i) {				\
		BENCH1(overhead_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__overhead);	\
		BENCH1(loop_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__r);		\
	}								\
	for (__i = 0; __i < __r.N; ++__i) {				\
		__oh = __overhead.v[__i].u / (double)__overhead.v[__i].n; \
		__r.v[__i].u -= (uint64)((double)__r.v[__i].n * __oh);	\
	}								\
	save_results(&__r);						\
}

#define	BENCH(loop_body, enough) { 					\
	long		__i, __N;					\
	result_t 	__r;						\
	insertinit(&__r);						\
	__N = (enough == 0 || get_enough(enough) <= 100000) ? TRIES : 1;\
	if (enough < LONGER) {loop_body;} /* warm the cache */		\
	for (__i = 0; __i < __N; ++__i) {				\
		BENCH1(loop_body, enough);				\
		if (gettime() > 0) 					\
			insertsort(gettime(), get_n(), &__r);		\
	}								\
	save_results(&__r);						\
}

#define	BENCH1(loop_body, enough) { 					\
	double		__usecs;					\
	BENCH_INNER(loop_body, enough);  				\
	__usecs = gettime();						\
	__usecs -= t_overhead() + get_n() * l_overhead();		\
	settime(__usecs >= 0. ? (uint64)__usecs : 0.);			\
}
	
#define	BENCH_INNER(loop_body, enough) { 				\
	static uint64	__iterations = 1;				\
	int		__enough = get_enough(enough);			\
	uint64		__n;						\
	double		__result = 0.;					\
									\
	while(__result < 0.95 * __enough) {				\
		start(0);						\
		for (__n = __iterations; __n > 0; __n--) {		\
			loop_body;					\
		}							\
		__result = stop(0,0);					\
		if (__result < 0.99 * __enough 				\
		    || __result > 1.2 * __enough) {			\
			if (__result > 150.) {				\
				double	tmp = __iterations / __result;	\
				tmp *= 1.1 * __enough;			\
				__iterations = (uint64)(tmp + 1);	\
			} else {					\
				if (__iterations > (uint64)1<<40) {	\
					__result = 0.;			\
					break;				\
				}					\
				__iterations <<= 3;			\
			}						\
		}							\
	} /* while */							\
	save_n((uint64)__iterations); settime((uint64)__result);	\
}

/* getopt stuff */
#define getopt	mygetopt
#define optind	myoptind
#define optarg	myoptarg
#define	opterr	myopterr
#define	optopt	myoptopt
extern	int	optind;
extern	int	opterr;
extern	int	optopt;
extern	char	*optarg;
int	getopt(int ac, char **av, char *opts);

typedef void (*bench_f)(uint64 iterations, void* cookie);
typedef void (*support_f)(void* cookie);

#define NOINIT NULL
#define NOCLEANUP NULL

extern void benchmp(support_f initialize, 
		bench_f benchmark,
		support_f cleanup,
		int enough, 
		int parallel,
		void* cookie);


/*
 * Generated from msg.x which is included here:

	program XACT_PROG {
	    version XACT_VERS {
		char
		RPC_XACT(char) = 1;
    	} = 1;
	} = 3970;

 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/types.h>

#define XACT_PROG ((u_long)404040)
#define XACT_VERS ((u_long)1)
#define RPC_XACT ((u_long)1)
#define RPC_EXIT ((u_long)2)
extern char *rpc_xact_1();
extern char *client_rpc_xact_1();

#endif /* _BENCH_H */
