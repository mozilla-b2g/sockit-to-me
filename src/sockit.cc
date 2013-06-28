// Indicate we're building a node.js extension
#define BUILDING_NODE_EXTENSION

// Global includes
#include <node.h>

// Platform dependent includes.
#include <errno.h>
#include <unistd.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <netdb.h>

// Local includes
#include "sockit.h"

// So we don't spend half the time typing 'v8::'
using namespace v8;

// Clever constants
static const double SUCCESS = 1;
static const int    MAX_LOOKUP_RETRIES = 3;

Sockit::Sockit() : mSocket(0) {

}

Sockit::~Sockit() {

}

/*static*/ void
Sockit::Init(Handle<Object> aExports) {
  Local<FunctionTemplate> object = FunctionTemplate::New(New);
  // Set the classname our object will have in JavaScript land.
  object->SetClassName(String::NewSymbol("Sockit"));

  // Reserve space for our object instance.
  object->InstanceTemplate()->SetInternalFieldCount(1);

  // Add the 'Connect' function.
  object->PrototypeTemplate()->Set(
    String::NewSymbol("connect"),
    FunctionTemplate::New(Connect)->GetFunction()
  );

  // Add the 'Read' function.
  object->PrototypeTemplate()->Set(
    String::NewSymbol("read"),
    FunctionTemplate::New(Read)->GetFunction()
  );

  // Add the 'Write' function.
  object->PrototypeTemplate()->Set(
    String::NewSymbol("write"),
    FunctionTemplate::New(Write)->GetFunction()
  );

  // Add the 'Close' function.
  object->PrototypeTemplate()->Set(
    String::NewSymbol("close"),
    FunctionTemplate::New(Close)->GetFunction()
  );

  // Add the constructor.
  Persistent<Function> constructor =
    Persistent<Function>::New(object->GetFunction());
  aExports->Set(String::NewSymbol("Sockit"), constructor);
}

/*static*/ Handle<Value>
Sockit::New(const Arguments& aArgs) {
  HandleScope scope;

  Sockit* sockit = new Sockit();
  sockit->Wrap(aArgs.This());

  return aArgs.This();
}

/*static*/ Handle<Value>
Sockit::Connect(const Arguments& aArgs) {
  HandleScope scope;

  if(aArgs.Length() < 1) {
    return scope.Close(
      Exception::Error(String::New("Not enough arguments."))
    );
  }

  if(aArgs.Length() > 1) {
    return scope.Close(
      Exception::Error(String::New("Too many arguments."))
    );
  }

  if(!aArgs[0]->IsObject()) {
    return scope.Close(
      Exception::Error(String::New("Invalid argument. Must be an Object."))
    );
  }

  Local<Object> options = aArgs[0]->ToObject();

  Local<String> hostKey = String::New("host");
  Local<String> portKey = String::New("port");

  if(!options->Has(hostKey) ||
     !options->Get(hostKey)->IsString()) {
    return scope.Close(
      Exception::Error(
        String::New("Object must have 'host' field and it must be a string.")
      )
    );
  }

  if(!options->Has(portKey) ||
     !options->Get(portKey)->IsNumber()) {
    return scope.Close(
      Exception::Error(
        String::New("Object must have 'port' field and it must be a number.")
      )
    );
  }

  // XXXAus: Move the DNS piece into it's own method.

  String::Utf8Value optionsHost(options->Get(hostKey)->ToString());

  bool shouldRetry = false;
  int  retryCount = 0;
  struct hostent *hostEntry;

  do {
    hostEntry = gethostbyname(*optionsHost);

    // Getting the host address failed. Let's see why.
    if(hostEntry == NULL) {
      // Check h_errno, we might retry if the error indicates we should.
      switch(h_errno) {
        case HOST_NOT_FOUND:
          return scope.Close(
            Exception::Error(String::New("Couldn't resolve host."))
          );
        break;

        case NO_RECOVERY:
          return scope.Close(
            Exception::Error(String::New("Unexpected failure during host lookup."))
          );
        break;

        case NO_DATA:
          return scope.Close(
            Exception::Error(String::New("Host exists but has no address."))
          );
        break;

        case TRY_AGAIN:
          shouldRetry = true;
          retryCount++;
        break;
      }
    }
  }
  while(shouldRetry && retryCount <= MAX_LOOKUP_RETRIES);

  if(hostEntry == NULL) {
    return scope.Close(
      Exception::Error(String::New("Couldn't resolve host even after retries."))
    );
  }

  int socketHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(socketHandle < 0) {
    return scope.Close(
      Exception::Error(String::New("Failed to create socket."))
    );
  }

  Sockit *sockit = ObjectWrap::Unwrap<Sockit>(aArgs.This());
  sockit->mSocket = socketHandle;

  // Magic socket incantation.
  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  // Always use first address returned, it's good enough for our purposes.
  address.sin_addr.s_addr =
      inet_addr(inet_ntoa(*(struct in_addr *)hostEntry->h_addr_list[0]));
  // Using static cast here to enforce short as required by IPv4 functions.
  address.sin_port =
      htons(static_cast<short>(options->Get(portKey)->ToNumber()->Uint32Value()));

  if(connect(
       sockit->mSocket, (sockaddr *) &address, sizeof(struct sockaddr)
     ) < 0) {
    // Save error number.
    int connectError = errno;
    // We failed to connect. Close to socket we created.
    close(sockit->mSocket);
    // In case there's another connect attempt, clean up the socket data.
    sockit->mSocket = 0;
    // Report failure to connect.
    return scope.Close(Exception::Error(String::New("Failed to connect.")));
  }

  return aArgs.This();
}

/*static*/ Handle<Value>
Sockit::Read(const Arguments& aArgs) {
  HandleScope scope;

  if(aArgs.Length() != 1) {
    return scope.Close(
      Exception::Error(
        String::New("Not enough arguments, read takes the number of bytes to be read from the socket.")
      )
    );
  }

  if(!aArgs[0]->IsNumber()) {
    return scope.Close(
      Exception::Error(String::New("Argument must be a Number."))
    );
  }

  Sockit *sockit = ObjectWrap::Unwrap<Sockit>(aArgs.This());

  // Make sure we're connected to something.
  if(sockit->mSocket == 0) {
    return scope.Close(
      Exception::Error(
        String::New("Not connected. To read data you must call connect first.")
      )
    );
  }

  // Figure out how many bytes the user wants us to read from the socket.
  unsigned int bytesToRead = aArgs[0]->ToNumber()->Uint32Value();
  // Allocate a shiny buffer.
  char *buffer = new char[bytesToRead];
  // Read it.
  int bytesRead = read(sockit->mSocket, &buffer[0], bytesToRead);
  // Sadly, we have to copy the data, we can't just assign this memory to
  // the string object as it may not be allocated by the same allocator.
  Local<String> data = String::New(buffer, bytesRead);
  // Done with our temporary buffer.
  delete buffer;

  return scope.Close(data);
}

/*static*/ Handle<Value>
Sockit::Write(const Arguments& aArgs) {
  HandleScope scope;

  if(aArgs.Length() < 1) {
    return scope.Close(
      Exception::Error(String::New("Not enough arguments."))
    );
  }

  if(aArgs.Length() > 1) {
    return scope.Close(
      Exception::Error(String::New("Too many arguments."))
    );
  }

  // XXXAus: Not quite right, should allow for ArrayBuffer Objects also.
  if(!aArgs[0]->IsString()) {
    return scope.Close(
      Exception::Error(String::New("Invalid argument, must be a String."))
    );
  }

  Sockit *sockit = ObjectWrap::Unwrap<Sockit>(aArgs.This());

  // Make sure we're connected to something.
  if(sockit->mSocket == 0) {
    return scope.Close(
      Exception::Error(
        String::New("Not connected. To read data you must call connect first.")
      )
    );
  }

  char *data = "GET / HTTP/1.0\n\n";

  int bytesToWrite = strlen(data);
  int bytesWritten = write(sockit->mSocket, data, bytesToWrite);

  return aArgs.This();
}

/*static*/ Handle<Value>
Sockit::Close(const Arguments& aArgs) {
  HandleScope scope;

  Sockit *sockit = ObjectWrap::Unwrap<Sockit>(aArgs.This());
  if(sockit->mSocket != 0) {
    // If close fails we have bigger problems on our hands than a stale
    // socket that isn't closed.
    close(sockit->mSocket);
  }

  sockit->mSocket = 0;

  return aArgs.This();
}
