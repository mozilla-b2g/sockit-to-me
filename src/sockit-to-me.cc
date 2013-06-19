// Indicate we're building a node.js extension
#define BUILDING_NODE_EXTENSION

// Global includes
#include <node.h>

// Local includes
#include "sockit-to-me.h"

// So we don't spend half the time typing 'v8::'
using namespace v8;

SockitToMe::SockitToMe() {

}

SockitToMe::~SockitToMe() {

}

/*static*/ void SockitToMe::Init(Handle<Object>) {

}

/*static*/ Handle<Value> SockitToMe::New(const v8::Arguments&) {

}

/*static*/ Handle<Value> SockitToMe::Connect() {

}

/*static*/ Handle<Value> SockitToMe::Read() {

}

/*static*/ Handle<Value> SockitToMe::Write() {

}

/*static*/ Handle<Value> SockitToMe::Close() {

}
