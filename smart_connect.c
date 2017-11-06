/******************************************************************************

 © Copyright 2006-2011, CERTARETE. All rights reserved.

SMART CONNECT

This is a utility function aimed at stream-oriented Internet client
applications including Web browsers, Video players and interactive
games.  It is intended to replace the typical sequence of calls an
application is required to make in order to connect to a network
server at a certain TCP port. The calls replaced by this function
include name resolution (e.g. gethostbyname(), an optional port
number resolution e.g. getservicebyname(), creation of a socket
with socket() and the initiation of a connection using the connect()
system call.

It does not merely replace these calls to simplify a client application
programming, but is doing so with increased sophistication to improve
functionality and performance of the client. By replacing the
traditional call sequence with this function the client will be
able to:

 ✓	Make the application compatible with both IPv4, 
	and IPv6 and virtually any dual stack setup;
 ✓	Automatically select the best server address available
	between IPv6 and IPv4;
 ✓	Given a choice of multiple equivalent geographically distributed
	servers, the client will select the closest one based on response time;
 ✓	Will cache the result of DNS name lookup and server selection locally
	to make the cost of creating new or additional connections the lowest,
	and gravitate towards the same server when possible.

It should be noted that the algorithm described in additional details
below is designed to comply with the stateful mechanism described
in the EFT "Happy Eye Balls" draft

THE ALGORITHM

At the heart of this module is the POSIX 1003.1 compliant  getaddrinfo()
call utilized to resolve the sever domain name into one or more
server IP addresses of either protocol family. Server domain names
are mapped to several IP addresses either for the purpose of
supporting both IPv4 and IPv6 protocols, or to load-balance the
clients across a number of "equivalent" servers, or all of the above
resins combined. The returned addresses are either all lead to the
same physical server, or to a collection of servers that host
identical content, hence termed equivalent.

Once the DNS resolution is complete, the module proceeds to initiate
a TCP connection to each and every address returned concurrently,
and proceeds to wait for any of the initiated connection to be
established. It is the premise of this code that the server that
is closest to the client will usually be the first to respond and
establish a connection. Mostly proximity will be in terms of
geographical distance, but at times when portions of the network
greatly vary in throughput, the first server to respond can still
be shown as the best choice for achieving the best client experience.
In a dual-stack situation the choice of protocol family will also
follow the same rule, with the aim of making the choice theta will
result in the best experience. It will seamlessly mitigate any IPv6
breakage issues when these exist. It is expectant that given the
choice of both IPv6 and IPv4 route to the same server, an IPv6
connection will be established sooner because the lack of Network
Address Translation gateways on the route should lead to a lower
round-trip delay, and as long as IPv6 is being phased-in, it is
without a doubt expected to be much less congested, and thus a
slight but highly desirable bias in favor of IPv6 does exist in
this implementation, but without sacrificing the quality of client
experience.

Once at least one connection is established, the remaining pending
connections will be aborted, and a socked descriptor to the established
connection is returned to the application, just like the result of
connectI(), and that returned connection is set up in blocking mode,
just lie that returned by connect().

The successful connection is also stored in a local cache, along
with the name of the server and the service requested, so that any
subsequent repeated connection request will employ a shortcut, skip
the DNS resolution and concurrent connection steps, and establish
a connection with the same server as was found to be preferable
just a short while earlier. The preference of the same address for
repeated connection request is not unconditional though: when the
choice of address is saved in the local cache, it is saved along
with the time it took to establish a connection. If the cached
address does not establish a connection within twice the anticipated
time, it may be a sign of a change in the client network configuration
or a change in the network topology or congestion climate, in which
case the algorithm will commence a new DNS resolution and concurrent
connection process, while continuing to wait for a connection to
be established with the cached address.

The locally cached entries are also aged out after e.g. 10 minutes
(per IETF draft), or when the cache fills up, oldest cache entries
will be displaced to make room for newest ones. Although the
concurrent connection method of selecting the preferred address and
protocol may seem overly aggressive, the use of local cache assures
that these processes are not repeated too often, so that the
additional traffic generated by the concurrent connection selection
is kept below the nuisance level.

FUNCTIONS

The module provides two callable functions:

 ⁃      smart_connect() which takes the domain name of the desired
 server and the name of the service (i.e. TCP port), and returns a
 socket descriptor. As with connect() it may block up to the
 system-wide default TCP connection timeout, which is typically set
 at 2 minutes.

 ⁃      smart_connect_with_timeout() is similar, but adds an upper
 bound on the time the function is allowed to block, so that if
 none of the servers responds within the allotted timeout, the
 function will return with an error.

ERROR REPORTING

If either function fails to establish a connection,, it will return
-1 with <errno> set to the code appropriate for the error condition
encountered. Note that DNS name resolution has a separate error
code name space, but this module performs a mapping of the DNS error
codes to one of the standard <errno> codes. Most notably when the
domain server name is not found or does not contain any IPv4 or
IPv6 address information,<errno> will be set to  EADDRNOTAVAIL.

COMPILE OPTIONS

The following compile-time options are available:

_UNITEST
	- enable a unit-test "main()" function
_DEBUG
	- enable diagnostic "printf()" messages
PTHREAD
	- should be defined when the module is used in threaded application
SMART_CONN_CACHE_SIZE
	- the size of the local cache. (default is 16)
	If set to 0, local cache and all its associated code will be disabled.
SMART_CONN_CACHE_TIME
	- the time in seconds before each cache entry is aged out,
	and should be set to 600 seconds for IETF draft compliance.
SMART_CONN_MIN_TIMEOUT
	- a lower bound on internal time out values in milliseconds, 
	provides a margin of variance to server response times.

AUTHOR
	Leonid Rosenboim <Leonid@Certarete.com>

LICENSE
 © Copyright 2006-2011, CERTARETE. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

+ Redistributions of source code must retain the above copyright
notice, this list of conditions and the following disclaimer.

+ Redistributions in binary form must reproduce the above copyright
notice, this list of conditions and the following disclaimer in the
documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

*/

/* Optional defines from Makefile */
/* #define	_UNITEST */
/* #define	_DEBUG  */
/* #define	PTHREAD */

#ifndef	SMART_CONN_CACHE_SIZE
#define	SMART_CONN_CACHE_SIZE	16
#endif

/* How long should a local cache entry be maintained */
#define	SMART_CONN_CACHE_TIME	(10*60) /* seconds */

/* Lower bound on timeout, to avoid timeout values unreasonable short */
#define	SMART_CONN_MIN_TIMEOUT	50	/* milliseconds */

#define	_POSIX_C_SOURCE 2	/* Needed for Linux */
#define	_BSD_SOURCE		/* Needed for Linux */
#define	_DARWIN_C_SOURCE	/* Needed for MacOS */

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#include <sys/errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <netdb.h>

#ifdef	PTHREAD
#include <pthread.h>
#endif

#include "smart_connect.h"

/* Maximum concurrent connections limitatipn */
#ifndef	SMART_CONN_MAX_CONN
#define	SMART_CONN_MAX_CONN	64
#endif

/* Make sure we have enough space for either protocol family */
typedef struct {
  union {
  struct sockaddr sa;
  struct sockaddr_in sin;
  struct sockaddr_in6 sin6 ;
  } a;
  unsigned short sa_size ;
} sockaddr_t;

static unsigned smart_conn_max_conn = SMART_CONN_MAX_CONN ;

#if	SMART_CONN_CACHE_SIZE > 0
struct smart_conn_cache_s {
	const char *	server_name,
		   *	service_name;
	time_t		time_learned;
	unsigned int	timeout;
	sockaddr_t	sa;
} ;

static struct smart_conn_cache_s smart_cache[ SMART_CONN_CACHE_SIZE ] ;

#ifdef	PTHREAD
pthread_mutex_t smart_conn_cache_mutex = PTHREAD_MUTEX_INITIALIZER ;
#endif

#ifdef	_DEBUG
/******************************************************************************
*
* Print out IPv4/IPv6 address
*
*/
static void 
smart_conn_sa_print( const char * server_name, const sockaddr_t * sa )
  {
  switch(sa->a.sa.sa_family) 
      {
      char str[64];
	case AF_INET:
	  inet_ntop(AF_INET, &(sa->a.sin.sin_addr), 
		    str, sizeof(str)); 
	  printf("%s: IPv4: %s\n", server_name, str );
	  break;

	case AF_INET6:
	    inet_ntop(AF_INET6, &(sa->a.sin6.sin6_addr),
		    str, sizeof(str)); 
	  printf("%s: IPv6: %s\n", server_name, str );
	  break;
        default:
	  printf("%s: invalid family: %d\n", server_name, sa->a.sa.sa_family );
      }
  }
#endif

/******************************************************************************
*
* Connect using cached address
*
*/
static int
smart_conn_cache_connect( 
    const char * server_name, 
    const char * service_name,
    int * sock_fd,
    sockaddr_t * sa,
    int * connect_time
    )
  {
  struct timeval t1, t2;
  struct smart_conn_cache_s * pCache ;
  struct pollfd poll_fds;
  unsigned index;
  int error, opt;
  int poll_timeout ;
  time_t now;

  time( &now );

  /* Lookup the socket address in cache, retire outdated entries */
  for(index = 0; index < SMART_CONN_CACHE_SIZE; index ++)
    {
    pCache = & smart_cache[ index ];
    if(pCache->server_name == NULL )
      continue;
    if(pCache->service_name == NULL )
      continue;
    if( (now - pCache->time_learned) >= SMART_CONN_CACHE_TIME )
      {
      free( (void *) pCache->server_name );
      free( (void *) pCache->service_name );
      pCache->server_name = pCache->service_name = NULL;
      continue;
      }
    if( (0 == strcmp( pCache->server_name, server_name)) &&
        (0 == strcmp( pCache->service_name, service_name)) )
      {
      break;
      }
    }

  /* No cached entry ? */
  if(index >= SMART_CONN_CACHE_SIZE )
    {
    errno = EAGAIN;
    return -1;
    }

#ifdef	_DEBUG
  printf("cache %d: ", index );
  smart_conn_sa_print( server_name, & pCache->sa );
#endif

  /* Remember the cached address */
  * sa = pCache->sa ;

  /* Connect again with cached entry */
  * sock_fd = socket( pCache->sa.a.sa.sa_family, SOCK_STREAM, 0 );
  if( * sock_fd < 0 )
    {
    printf("socket error: %s\n", strerror(errno));
    return -1;
    }

  /* Mark this socket non-blocking */
  error = opt =  fcntl( * sock_fd, F_GETFL, 0 );
  if( error < 0 || fcntl( * sock_fd, F_SETFL, opt | O_NONBLOCK )  < 0 )
    {
    printf("fcntl error: %s\n", strerror(errno));
    close (* sock_fd);
    * sock_fd = -1;
    return -1;
    }

  /* Initiate connection attempt */
  error = connect(* sock_fd, & pCache->sa.a.sa, pCache->sa.sa_size);

  if( error < 0 && errno != EINPROGRESS )
    {
    printf("connect %d error: %s\n", *sock_fd, strerror(errno));
    /* Connection not initiated, address family no longer works */
    pCache->time_learned = 0;
    close(* sock_fd);
    * sock_fd = -1;
    return -1;
    }

  gettimeofday( &t1, NULL );

  /* Use timeout from the cache entry if set */
  poll_timeout = pCache->timeout;
  if( poll_timeout == 0 )
    poll_timeout = -1;

  /* Prep poll data */
  poll_fds.fd = * sock_fd;
  poll_fds.events = POLLOUT;
  poll_fds.revents = 0;

  /* Use poll() tp wait for connect to succeed */
  error = poll( & poll_fds, 1, poll_timeout );

  gettimeofday( &t2, NULL );
  timersub( &t2, &t1, &t2 );
  * connect_time = 1000 * t2.tv_sec + t2.tv_usec / 1000 ;

  /* NOTE: if interrupted by a signal, will revert to concurrent connection */

  if( error < 0 )
    {
    /* Poll returned an error, cached addtress is wrong ? */
    pCache->time_learned = 0;
    close(* sock_fd);
    * sock_fd = -1;
    return -1;
    }
  else if( error == 0 )
    {
    /* Time out, maybe cached timeout was not enough, maybe network change */
    /* Forget this cache entry, it will be re-learned anyway */
    pCache->time_learned = 0;
    /* The caller will try new search and  continue to wait on this socket */
    return 1;
    }
  else
    {
    /* Soccess !, the caller will revert to blocking mode */
    return 0;;
    }
  }

/******************************************************************************
*
* Store learned address in smart_cache
*
* For a given server, its address along with its anticipated tineout
* are stired in a local cache to avoid repeating name lookup and 
* concurrent connection to identify the closest server of a number
* of entries returned by DNS.
*
* RETURNS: N/A
*/
static void
smart_conn_cache_store(  const char * server_name, const char * service_name,
		int sock_fd, const sockaddr_t *sa, unsigned int timeout )
  {
  volatile struct smart_conn_cache_s * pCache ;
  time_t now, t;
  unsigned index, index_free;

  index_free = SMART_CONN_CACHE_SIZE ;
  time( & now );
  t = now;

  /* Find a vacant cache entry */
  for(index = 0; index < SMART_CONN_CACHE_SIZE; index ++)
    {
    pCache = & smart_cache[ index ];
    /* Found an emplty slot, done */
    if(pCache->server_name == NULL )
      {
      index_free = index ;
      break;
      }

    if( 0 == strcmp( server_name, pCache->server_name))
      {
      index_free = index ;
      break;
      }

    /* Found an outdated slot, done */
    if( (now - pCache->time_learned) >= SMART_CONN_CACHE_TIME )
      {
      index_free = index ;
      break;
      }
    /* Find the oldest slot if all are busy and not outdated */
    if( t > pCache->time_learned )
      {
      index_free = index ;
      t = pCache->time_learned ;
      }
    }

  /* when all cache entries are new (within 1 second) then there is no room */
  if( index_free >= SMART_CONN_CACHE_SIZE )
    {
    return;
    }

  /* Mutex lock */
#ifdef	PTHREAD
  pthread_mutex_lock( & smart_conn_cache_mutex );
#endif

  pCache = & smart_cache[ index_free ];

  /* Make sure the entry found is indeed free */
  if( pCache->server_name != NULL )
    {
    free( (void *) pCache->server_name );
    pCache->server_name =  NULL;
    }
  if( pCache->service_name != NULL )
    {
    free( (void *) pCache->service_name );
    pCache->service_name = NULL;
    }

  /* Double check entry is still free, and not taken up with another thread */
  if( pCache->server_name == NULL && pCache->service_name == NULL )
    {
    /* Fill in the chosen cache entry */
    pCache->server_name = strdup( server_name );
    pCache->service_name = strdup( service_name );
    pCache->sa = * sa ;
    pCache->time_learned = now;
    pCache->timeout = timeout;
#ifdef	_DEBUG
    printf("%s: cached at %u\n", server_name, index_free );
#endif
    }

  /* Mutex un-lock */
#ifdef	PTHREAD
  pthread_mutex_unlock( & smart_conn_cache_mutex );
#endif
  }

#endif	/* SMART_CONN_CACHE_SIZE */

/******************************************************************************
*
* Map error codes returned by getaddrinfo() into the closest <errno.h> alias
*
* This mapping is specifically tailored at smart_connect() below, hence
* errors that do not apply are all mapped to a "catch all" general error.
*
* RETURNS: the closest <errno.h> equivalent code.
*/
static int
smart_connect_map_error(int gai_errno)
  {
  switch( gai_errno )
    {
    case EAI_SYSTEM:    /* system error returned in errno */
	return errno ;
    case EAI_MEMORY:    /* memory allocation failure */
	return ENOMEM ;
    case EAI_OVERFLOW:  /* argument buffer overflow */
	return ENOBUFS ;
    case EAI_AGAIN:     /* temporary failure in name resolution */
	return EAGAIN ;
#ifdef	EAI_PROTOCOL
    case EAI_PROTOCOL:  /* resolved protocol is unknown */
#endif
    case EAI_SERVICE:   /* servname not supported for ai_socktype */
    case EAI_FAMILY:    /* ai_family not supported */
	return EPROTONOSUPPORT;
    case EAI_FAIL:      /* non-recoverable failure in name resolution */
    case EAI_NONAME:    /* hostname or servname not provided, or not known */
	return EADDRNOTAVAIL ;
    case EAI_SOCKTYPE:  /* ai_socktype not supported */
    case EAI_BADFLAGS:  /* invalid value for ai_flags */
#ifdef	EAI_BADHINTS
    case EAI_BADHINTS:  /* invalid value for hints */
#endif
    default:
	return EINVAL ;
    }
  }

/******************************************************************************
*
* smart_connect_with_timeout - make a TCP connection to one of several servers
*
* PARAMETERS
* <server_name> could be a numeric IP address or more appropriately a
* DNS name of a server, or a group of equivalent servers.
* <service_name> is either a numberic TCP port number or a mnemonic of
* a TCP service.
* <timeout> connection timeout in milliseconds.
*
* RETURNS
* The function returns the file descriptor of the socket for the resulting
* connection or -1 in case of an error.
*
* NOTES
* This funcion will block until a connection is established or an error
* occurs. The socked returned is in its default blocking mode.
*/
int
smart_connect_with_timeout( 
	const char * server_name, 
	const char * service_name, 
	unsigned timeout
	)
  {
  struct addrinfo hints, *res, *res0;
  struct pollfd * poll_fds;
  sockaddr_t * sas, s0 ;
  int poll_timeout, conn_time;
  unsigned conn_count ;
  unsigned index;
  unsigned pend_count ;
  int error;
  int sock_fd, opt;
  struct timeval t1, t2;
#ifdef	_DEBUG
  unsigned addr_count, ipv6_count ;
#endif

  poll_fds = NULL ;
  sas = NULL ;
  sock_fd = -1;
  conn_count = 0 ;
  conn_time = 0;
  memset( &s0, 0, sizeof(s0));

#if	SMART_CONN_CACHE_SIZE > 0
  error = smart_conn_cache_connect( server_name, service_name, 
  	&sock_fd, & s0, & conn_time );

#ifdef	_DEBUG
  printf("%s: cache returned %d in %u msec\n", server_name, error, conn_time);
#endif

  if( error == 0 )
    {
    /* Cached connection succeeded, just revert socket to blocking and return */
    opt = fcntl( sock_fd, F_GETFL, 0 );
    (void) fcntl( sock_fd, F_SETFL, opt & ~O_NONBLOCK );
    return sock_fd ;
    /* Avoid re-learning when re-using cache, let the cache entry age out */
    }
  else if( error == 1 )
    {
    /* Proceed to concurrent connect, along with initiated connection */
    conn_count = 1;
    }
  else
    {
    /* Proceed to concurrent connect, nothing in cache */
    }
#endif

  /* Prepare an array to store addresses */
  sas = calloc( smart_conn_max_conn, sizeof( * sas ));
  
  /* Prepare the file descriptor set to poll on */
  poll_fds = calloc( smart_conn_max_conn , sizeof(* poll_fds ) );
  if( poll_fds == NULL || sas == NULL )
    {
    goto _error;
    }
  for(index = 0; index < smart_conn_max_conn; index ++ )
    {
    poll_fds[ index ].fd = -1;
    poll_fds[ index ].events = 0;
    }

  index = 0;

#if	SMART_CONN_CACHE_SIZE > 0
  if( conn_count > 0 )
    {
    poll_fds[index].fd = sock_fd;
    poll_fds[index].events = POLLOUT;
    sas[ index ] = s0;
    index ++;
    }
#endif

  /* Initialize the addrinfo hints structure */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = PF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

#ifdef	_DEBUG
  addr_count = ipv6_count = 0;
#endif

  /* Perform DNS resolution, get list of socket addresses */
  error = getaddrinfo(server_name, service_name, &hints, &res0);

  /* Name resulution failed, map error code to <errno> */
  if (error)
    {
    errno = smart_connect_map_error( error );
    goto _error;
    }

  /* Start connection time measurement */
  gettimeofday( &t1, NULL );

  /* Traverse list of candidate addresses, initiate connection to each */
  for (res = res0; res; res = res->ai_next) 
    {
#ifdef	_DEBUG
    addr_count ++ ;
    if( res->ai_family != AF_INET )
      ipv6_count ++;
#endif

    /* Create a socket for each address */
    sock_fd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sock_fd < 0)
      {
      continue;
      }

    /* Mark this socket non-blocking */
    error = opt =  fcntl( sock_fd, F_GETFL, 0 );
    if( error < 0 || fcntl( sock_fd, F_SETFL, opt | O_NONBLOCK )  < 0 )
      {
      close (sock_fd);
      sock_fd = -1;
      continue ;
      }

    /* Make sure our buffer is indeed big enough */
    assert( sizeof( sas[0] ) >= res->ai_addrlen );

    /* Initiate connection attempt */
    error = connect(sock_fd, res->ai_addr, res->ai_addrlen) ;

    if( error < 0 && errno != EINPROGRESS )
      {
      close(sock_fd);
      sock_fd = -1;
      continue;
      }

    /* Save pending connection socket for later polling */
    poll_fds[ index ].events = POLLOUT ;
    poll_fds[ index ].fd = sock_fd ;
    memcpy( & sas[ index ].a.sa, res->ai_addr, res->ai_addrlen );
    sas[ index ].sa_size = res->ai_addrlen ;

    /* Record the number of initiated connections */
    conn_count ++; index ++ ;

    /* Limit the number of outstanding conections */
    if( index >= smart_conn_max_conn )
      break;

    /* Should a server be faster than the client, need no more connections */
    if( error == 0 )
      break ;
    } /* for res */


#ifdef	_DEBUG
  printf("%s: %d connections started, of %u addresses, %u are ipv6\n", 
  	server_name, conn_count,
	addr_count, ipv6_count );
#endif

  /* Free the address resolution result data memory */
  freeaddrinfo(res0);

  /* If there was no success in initiating a conenction, report an error */
  /* <errno> will hold the reason for the last attempt to fail */
  if (sock_fd < 0 && conn_count == 0 )
    {
    goto _error;
    }

  /* Decide on actual timeout value */
  if( timeout < 1 )
    {
    poll_timeout = -1;
    }
  else
    {
    poll_timeout = timeout - conn_time ;

    if( poll_timeout < SMART_CONN_MIN_TIMEOUT )
      poll_timeout = SMART_CONN_MIN_TIMEOUT ;
    }

  /* Wait for as long as at least one socket is still pending connection */
  for( pend_count = conn_count ; pend_count > 0; )
    {

    /* Poll for first ready socket */
    error = poll( poll_fds, smart_conn_max_conn, poll_timeout );

#ifdef	_DEBUG
    printf("poll returned %d, errno %d\n", error, errno );
#endif

    if( error == 0 )
      {
      errno = ETIMEDOUT ;
      goto _error;
      }

    if( error < 0 && errno == EINTR )
      continue ;

    if( error < 0 )
      {
      goto _error;
      }

    /* Inspect the results from poll() */
    for( pend_count = index = 0; index < conn_count; index ++ )
      {
#ifdef	_DEBUG
	printf("poll fd %d, revents %#x\n", 
	  poll_fds[index].fd, poll_fds[index].revents );
#endif

      if( poll_fds[ index ].revents == POLLOUT )
	{
	/* Live connection found */
	sock_fd = poll_fds[ index ].fd;
	goto _success ;
	}
      else if( poll_fds[ index ].revents == 0 )
	{
	/* Pending connection, count */
        pend_count ++;
	}
      else
	{
	/* There was an error, ignore this <fd> for now, will close later */
        poll_fds[ index ].events = 0;
	}
      } /* for index */
    } /* for  pend_count */

  /* If all initiated attempts have resulted in poll() error */
  errno = ENOTCONN ;

_error:

  if( sas != NULL )
    free( sas );

  if( poll_fds != NULL )
    {
    /* Close all sockets */
    for( index = 0; index < conn_count ; index ++ )
      {
      sock_fd = poll_fds[ index ].fd ;
      if(sock_fd < 0)
	continue;
      (void) close( sock_fd );
      }

    free(poll_fds);
    }
  return -1 ;
  
_success:
  /* Measure connection time */
  gettimeofday( &t2, NULL );
  timersub( &t2, &t1, &t1 );
  conn_time += t1.tv_sec * 1000 + t1.tv_usec / 1000;

#ifdef	_DEBUG
  printf("%s: connected in %d msec\n",
  	server_name, conn_time );
#endif

  if( poll_fds != NULL )
    {
    unsigned i ;

    /* <sock_fd> connected, close all other pending sockets */
    for( i = 0; i < conn_count; i ++ )
      {
      if( sock_fd != poll_fds[ i ].fd && poll_fds[ i].fd >= 0)
	{
	(void) close( poll_fds[ i].fd );
	}
      }

    free(poll_fds);
    }

  /* Revert the resulting connection to blocking mode */
  opt = fcntl( sock_fd, F_GETFL, 0 );
  (void) fcntl( sock_fd, F_SETFL, opt & ~O_NONBLOCK );

#if	SMART_CONN_CACHE_SIZE > 0
#ifdef	_DEBUG
    printf("storing index %d: ", index );
    smart_conn_sa_print( server_name, & sas [ index ] );
#endif
  smart_conn_cache_store( server_name, service_name, 
  	sock_fd, & sas[ index ],
  	(conn_time << 1) + SMART_CONN_MIN_TIMEOUT );
#endif

  if( sas != NULL )
    free( sas );

  /* Return the socket file descriptor, just like "connect()" */
  return sock_fd ;
  }

/******************************************************************************
*
* smart_connect - Smart connect without a time out
*
* This function is similar to smart_connect_with_timeout(), but does not
* specify a maximum time to wait for a server to respond.
*
* When timeout is unspecified, the code will rely on default TCP timeout
* which is typically 2 minutes, to find the fastest responding server
* and return a socket file descritor connected with it.
*/
int
smart_connect( const char * server_name, const char * service_name )
  {
  return (
    smart_connect_with_timeout(
      server_name,
      service_name,
      0)
      );
  }

#ifdef	_UNITEST
/******************************************************************************
*
* A simple unit test to demonstrate the workings of smart_connect().
*
* Program expects a single argument, a numeric address or a domain
* name of an HTTP server, that translates to multiple IP addresses.
* The benefits of smart_connect() are best demonstrated when the
* server name translates into several addresses, some of which
* lead to non-existent servers or networks, that would cause the
* client to delay for a couple of minutes on engaging a defunct server
* with the usual blocking connect() call.
*
*/
int
test_connect( const char * server, int verbose )
  {
  int s, rc ;
  char buf[ BUFSIZ ];
  struct timeval t1, t2, t3 ;

  gettimeofday( &t1, NULL );

  s = smart_connect( server, "http"  );
  if( s < 0 )
    {
    printf("%s: smart_connect error: %s\n", server, strerror(errno));
    return -1;
    }

  gettimeofday( &t2, NULL );

  rc = snprintf(buf, sizeof(buf)-1,
  		"HEAD / HTTP/1.1\r\n"
  		"Host: %s\r\n"
		"Connection: close\r\n"
		"\r\n",
		server);
  rc = write( s, buf, rc );
  if( rc < 0 )
    {
    perror("write");
    return -2;
    }

  memset(buf, 0, sizeof(buf));

  rc = read(s, buf, sizeof(buf)-1 );
  if( rc < 0 )
    {
    perror("read");
    return -3;
    }

  gettimeofday( &t3, NULL );

  if( verbose &&  rc > 0 )
    printf("Response:\n%s", buf );

  close(s);

  timersub( &t2, &t1, &t2 );
  timersub( &t3, &t1, &t3 );
  printf("%s: connected in %ld.%06u, responded in %ld.%06u  sec\n", 
	server,
  	t2.tv_sec, (unsigned) t2.tv_usec,
  	t3.tv_sec, (unsigned) t3.tv_usec );
  return 0;
  }

int
main( int argc, char ** argv )
  {
  int rc;
  FILE * fp;

  if( argc <= 1 )
    {
    fprintf(stderr, "Usage: %s domain|file\n", argv[0] );
    return -1;
    }

  /* Check if argv1 is a file name */
  fp = fopen( argv[1], "r");
  if( fp != NULL )
    {
    static char namebuf[4192];
    static char * names[32];
    char * p;
    unsigned count, i;

    printf("reading file %s ...\n", argv[1] );

    rc = fread( namebuf, sizeof(namebuf), 1, fp );

    for(p = namebuf, count = 0; p && count < 32; )
      {
      names[count++] = p ;
      p = strchr(p, '\n');
      if( p ) *p++ = '\0';
      }

    for( i = 0; i < 100; i ++ )
      {
      unsigned i ;
      i = random() % count ;
      if( names[i] == NULL ) continue;
      if( strlen(names[i]) < 3 ) continue;
      rc = test_connect( names[i], 0 );
      if( rc < 0 ) printf("%s: error !!\n", names[i]);
      sleep(1);
      }
    return 0;
    }

  rc = test_connect( argv[1], 1 );
  if( rc < 0 ) exit(rc);

  return 0;
  }

#endif	/* _UNITEST */
