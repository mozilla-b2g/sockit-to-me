// Indicate we're building a node.js extension
#define BUILDING_NODE_EXTENSION

// Global includes
#include <node.h>

// Local includes
#include "sockit-to-me.h"

// So we don't spend half the time typing 'v8::'
using namespace v8;

// Initialize
void InitAll(Handle<Object> aExports) {
  SockitToMe::Init(aExports);
}

NODE_MODULE(addon, InitAll)
