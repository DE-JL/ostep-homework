#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#define NS_PER_SEC 1000000000

static inline uint64_t ns( void )
{
  struct timespec ts;
  clock_gettime( CLOCK_MONOTONIC_RAW, &ts );
  return (uint64_t)ts.tv_sec * NS_PER_SEC + ts.tv_nsec;
}

void time_open( int n )
{
  uint64_t start_ns = ns();

  for ( int i = 0; i < n; ++i )
  {
    int fd = open( "/etc/passwd", O_RDONLY );
    close( fd );
  }

  uint64_t end_ns = ns();

  uint64_t avg = ( end_ns - start_ns ) / n;
  printf( "Average open( ... ) + close( ... ) time: %llu ns\n", avg );
}

void time_context_switch( int n )
{
  int p_fds[ 2 ] = { 0 }; // { p_read, p_write }
  int c_fds[ 2 ] = { 0 }; // { c_read, c_write }

  if ( pipe( p_fds ) || pipe( c_fds ) )
  {
    fprintf( stderr, "Failed to create pipe!\n" );
    exit( 1 );
  }

  char buf = 0;
  pid_t pid = fork();

  // Child process.
  if ( pid == 0 )
  {
    for ( int i = 0; i < n; ++i )
    {
      read( c_fds[ 0 ], &buf, 1 );
      write( p_fds[ 1 ], &buf, 1 );
    }
    return;
  }

  // Parent benchmarks two context switches per iteration.
  uint64_t start_ns = ns();

  for ( int i = 0; i < n; ++i )
  {
    write( c_fds[ 1 ], &buf, 1 );
    read( p_fds[ 0 ], &buf, 1 );
  }

  uint64_t end_ns = ns();

  wait( NULL );

  uint64_t avg = ( end_ns - start_ns ) / n;
  printf( "Average context switch time: %llu ns\n", avg / 2 );
}

int main( void )
{
  // 1000 iterations per experiment.
  time_open( 1000 );
  time_context_switch( 1000 );
}
