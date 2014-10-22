/*begin hand.c 
 *This file define some function to hand the program`s signal and daemon
 *
 */

#include <syscall.h>
#include <syslog.h>
#include "http_conn.h"
#include "cpu_time.h"

#define	MAXFD	64
void addsig( int sig, void( handler )(int), bool restart)
{
	struct sigaction sa;
	memset( &sa, '\0', sizeof( sa ) );
	sa.sa_handler = handler;
	if( restart ){
		sa.sa_flags |= SA_RESTART;
	}
	sigfillset( &sa.sa_mask );
	if(sigaction( sig, &sa, NULL ) <= -1){
		perror("sigaction");
	}
}

/*Here we could deal with some signal handler*/
void hand_sig()
{
	addsig( SIGPIPE, SIG_IGN ,false);
	addsig(SIGINT,cpu_time,false);
}

//extern int	daemon_proc;	/* defined in error.c */

//void daemon_init(const char *pname, int facility)
void daemon_init(  )
{
	int		i;
	pid_t	pid;

	if ( (pid = Fork()) != 0)
		exit(0);			/* parent terminates */
	/* 1st child continues */
	/* become session leader */
	setsid();

	hand_sig();
	Signal(SIGHUP, SIG_IGN);
	if ( (pid = Fork()) != 0)
		exit(0);			/* 1st child terminates */
	/* 2nd child continues */

//	daemon_proc = 1;		/* for our err_XXX() functions */

	/* change working directory */
	chdir("/");

	/* clear our file mode creation mask */
	umask(0);

	for (i = 0; i < MAXFD; i++)
		close(i);

	//openlog(pname, LOG_PID, facility);
}
