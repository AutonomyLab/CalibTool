#include <unistd.h>
#include <iostream>
#include <time.h>
#include <iostream> 
#include <sstream>
#include <stdlib.h>
#include <string.h>
#include <hiredis/hiredis.h>

using namespace std;

int main(int argc, char** argv) 
{
  // todo - make these command line options
  const char* redisHost = "127.0.0.1";
  const int redisPort = 6379;  
  redisContext *redisc = NULL;
  
  redisc = redisConnect( redisHost, redisPort );
  if( redisc->err )
    {
      printf( "Redis connection error: %s\n", redisc->errstr);
      exit(-1);
    }
  
  std::stringstream key;
  key << "camera" << argv[1];
  
  while( 1 )
    {
      void* reply = redisCommand( redisc, "GET %s", key.str().c_str() );
      
      if( reply == NULL )
	printf( "Redis error on SET: %s\n", redisc->errstr );
      else
	{
	  const redisReply* r = (redisReply*)reply;
	  
	  if(  r->type == REDIS_REPLY_STRING )
	    {
	      if( strlen(r->str) > 1 )
		cout << r->str << endl;
	    }
	  else
	    cout << "unexpected reply type " << r->type << endl;
	}

      usleep(3e5);
    }
}
