// compile hint

/*
 * MATLAB
 * mex -lhiredis -I/usr/include/hiredis/ CFLAGS='-fPIC -std=c99 -O4 -pedantic -g' redis_.c
 *
 * GNU OCTAVE
 * gcc -fPIC -I /usr/include/octave-3.8.2/octave/ -lm -I /usr/include/hiredis/ -lhiredis -std=c99 -shared redis_.c -o redis_.mex
 */

// C specific includes
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

// get it from here
// https://github.com/redis/hiredis/
#include <hiredis.h>

// Matlab/GNU Octave specific includes
#include <mex.h>

// declarate some stuff
char* redisReturn;
char *hostname, *command, *password;
int port, database;
char redisChar[19]; // afaik long enough for long long int
mxArray *cell_array_ptr;


// matlab/octave mex function
void mexFunction (int nlhs, mxArray *plhs[],
                  int nrhs, const mxArray *prhs[])
{
  // --- input checks
  // currently we need at least more than one input and one ouput!!
  // improve me!!
  if(nrhs < 1 && nlhs != 1) {
    mexErrMsgIdAndTxt( "MATLAB:redis_:invalidNumInputs",
                      "One or more inputs are required.");
  } 
  
  // default stuff
  hostname  = "127.0.0.1";
  port      = 6379;  
  database  = 0;
  password  = "";
  
  if ( nrhs == 1) {
    // one input (command), use default host and port
    if ( mxIsChar(prhs[0]) ) {
      // default hostname and port
      // get command
      command = (char *) mxCalloc(mxGetN(prhs[0])+1, sizeof(char));
      mxGetString(prhs[0], command, mxGetN(prhs[0])+1);
    } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:nrhs", "Command Input must be a string.");
    }
  }
  
  if ( nrhs == 2  ) {
    // two inputs (0. host, 1. command) - both strings
    if ( mxIsChar(prhs[0])  &&  mxIsChar(prhs[1]) ) {
     
      hostname = (char *) mxCalloc(mxGetN(prhs[0])+1, sizeof(char));
      mxGetString(prhs[0], hostname, mxGetN(prhs[0])+1);

      command = (char *) mxCalloc(mxGetN(prhs[1])+1, sizeof(char));
      mxGetString(prhs[1], command, mxGetN(prhs[1])+1);
    } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:nrhs", "Command and Hostname Input must be a string.");
    }
  }
  
  if ( nrhs == 3 ) {
      // three inputs (0. host, 1. port, 2. command)
      if ( mxIsChar(prhs[0])  &&  mxIsDouble(prhs[1]) && mxIsChar(prhs[2]) ) {
          
          hostname = (char *) mxCalloc(mxGetN(prhs[0])+1, sizeof(char));
          mxGetString(prhs[0], hostname, mxGetN(prhs[0])+1);
          
          // convert double to integer :: PORT
          double* data = mxGetPr(prhs[1]);
          port = (int)floor(data[0]);
          
          command = (char *) mxCalloc(mxGetN(prhs[2])+1, sizeof(char));
          mxGetString(prhs[2], command, mxGetN(prhs[2])+1);
      } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:nrhs", "Command and Hostname Input must be a string and Port must be double.");
      }
  }
  
  if ( nrhs == 4 ) {
      // four inputs (0. host, 1. port, 2. database number, 3. command,)
      if ( mxIsChar(prhs[0])  &&  mxIsDouble(prhs[1]) && mxIsDouble(prhs[2]) && mxIsChar(prhs[3]) ) {
          
          hostname = (char *) mxCalloc(mxGetN(prhs[0])+1, sizeof(char));
          mxGetString(prhs[0], hostname, mxGetN(prhs[0])+1);
          
          // convert double to integer :: PORT
          double* data = mxGetPr(prhs[1]);
          port = (int)floor(data[0]);
          
          // convert double to integer :: DATABASE NUMBER
          double* databasedata = mxGetPr(prhs[2]);
          database = (int)floor(databasedata[0]);
          
          command = (char *) mxCalloc(mxGetN(prhs[3])+1, sizeof(char));
          mxGetString(prhs[3], command, mxGetN(prhs[3])+1);
      } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:nrhs", "Command and Hostname Input must be a string and Port and database must be double.");
      }
  }
  
  if ( nrhs == 5 ) {
      // five inputs (0. host, 1. port, 2. database number, 3. password, 4. command )
      if ( mxIsChar(prhs[0])  &&  mxIsDouble(prhs[1]) && mxIsDouble(prhs[2]) && mxIsChar(prhs[3]) && mxIsChar(prhs[4]) ) {
          
          hostname = (char *) mxCalloc(mxGetN(prhs[0])+1, sizeof(char));
          mxGetString(prhs[0], hostname, mxGetN(prhs[0])+1);
          
          // convert double to integer :: PORT
          double* data = mxGetPr(prhs[1]);
          port = (int)floor(data[0]);
          
          // convert double to integer :: DATABASE NUMBER
          double* databasedata = mxGetPr(prhs[2]);
          database = (int)floor(databasedata[0]);
          
          password = (char *) mxCalloc(mxGetN(prhs[3])+1, sizeof(char));
          mxGetString(prhs[3], password, mxGetN(prhs[3])+1);
          
          command = (char *) mxCalloc(mxGetN(prhs[4])+1, sizeof(char));
          mxGetString(prhs[4], command, mxGetN(prhs[4])+1);
      } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:nrhs", "Command and Hostname Input must be a string and Port and database must be double.");
      }
  }
  
  // hiredis declaration
  redisContext *c;
  redisReply *reply;

  // time out and make redis connection
  struct timeval timeout = { 1, 500000 }; // 1.5 seconds
  c = redisConnectWithTimeout(hostname, port, timeout);

  // error when connection failed
  if (c == NULL || c->err) {
    if (c) {
      mexErrMsgIdAndTxt("MATLAB:redis_:connectionError","Connection error: %s\n", c->errstr);
      redisFree(c);
    } else {
      mexErrMsgIdAndTxt("MATLAB:redis_:connectionError", "Connection error: can't allocate redis context.");
    }
  }

  // 1) optional auth
  if (strlen(password) > 0){
      reply= redisCommand(c, "AUTH %s", password);
      if (reply->type == REDIS_REPLY_ERROR) {
          /* Authentication failed */
          mexErrMsgIdAndTxt("MATLAB:redis_:AuthenticationFailed", "Authentication failed.");
      }
  }

  // 2) optional change database
  // !Improve me in case of error
  if (database != 0) {
      reply = redisCommand(c, "SELECT %d", database);
      if (reply->type == REDIS_REPLY_ERROR) {
          /* Select database failed */
          mexErrMsgIdAndTxt("MATLAB:redis_:SelectFailed", "Select Database %d failed.", database);
      }
  }
  
  // call redis
  reply = redisCommand(c, command);

  // check the output
  if (reply->type == REDIS_REPLY_STRING) {
      plhs[0] = mxCreateString(reply->str);
  } else if (reply->type == REDIS_REPLY_ARRAY) {
    // get number of elements
    int n = (int)floor(reply->elements);
    // outout will be a cell array matlab-sided
    cell_array_ptr = mxCreateCellMatrix(n,1);

    for (unsigned int j = 0; j < reply->elements; j++) {
        //mexPrintf("%u) %s\n", j, reply->element[j]->str);
        mxSetCell(cell_array_ptr,j, mxCreateString(reply->element[j]->str));
    }

    // free hiredis
    freeReplyObject(reply);
    redisFree(c);
    // this will return warnings ... but seems to work fine
    plhs[0] = cell_array_ptr;
    
  } else if (reply->type == REDIS_REPLY_INTEGER) {
    // IMPROVE ME! 
    // currently numbers are convert and returned as a string
    long long int t = reply->integer;
    // save copy
    //snprintf(redisChar, 19, "%lld",t);
    //plhs[0] = mxCreateString(redisChar);

    plhs[0] = mxCreateNumericMatrix(1, 1, mxDOUBLE_CLASS, mxREAL);
    double *out;
    out = mxGetPr(plhs[0]);
    out[0] = t;
        
    // free redis
    freeReplyObject(reply);
    redisFree(c);

    

  } else {
    // Laugh in the face of danger
    plhs[0] = mxCreateString(reply->str);
  }

}


