// Indicate we're building a node.js extension
#define BUILDING_NODE_EXTENSION

// Global includes
#include <node.h>

// Platform dependent includes.
#include <errno.h>
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netdb.h>

// Local includes
#include "sockit.h"

// So we don't spend half the time typing 'v8::'
using namespace v8;

// Clever constants
static const double SUCCESS = 1;
static const int    MAX_LOOKUP_RETRIES = 3;

Sockit::Sockit() {

}

Sockit::~Sockit() {

}

/*static*/ void Sockit::Init(Handle<Object> aExports) {
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

/*static*/ Handle<Value> Sockit::New(const Arguments& aArgs) {
  HandleScope scope;

  Sockit* sockit = new Sockit();
  sockit->Wrap(aArgs.This());

  return aArgs.This();
}

/*static*/ Handle<Value> Sockit::Connect(const Arguments& aArgs) {
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

  Local<String> optionsHost = options->Get(hostKey)->ToString();
  char *host = new char[optionsHost->Utf8Length()];
  optionsHost->WriteUtf8(host);


  bool shouldRetry = false;
  int  retryCount = 0;
  struct hostent *hostEntry;

  do {
    hostEntry = gethostbyname(host);

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

  int socketHandle = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(socketHandle < 0) {
    return scope.Close(Number::New(errno));
  }

  Sockit *sockit = ObjectWrap::Unwrap<Sockit>(aArgs.This());
  sockit->mSocket = socketHandle;

  struct sockaddr_in address;
  memset(&address, 0, sizeof(address));
  address.sin_family = AF_INET;
  address.sin_addr.s_addr =
      inet_addr(inet_ntoa(*(struct in_addr *)hostEntry->h_addr_list[0]));
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
    return scope.Close(Number::New(connectError));
  }

  return scope.Close(Number::New(SUCCESS));
}

/*static*/ Handle<Value> Sockit::Read(const Arguments& aArgs) {

}

/*static*/ Handle<Value> Sockit::Write(const Arguments& aArgs) {

}

/*static*/ Handle<Value> Sockit::Close(const Arguments& aArgs) {

}
