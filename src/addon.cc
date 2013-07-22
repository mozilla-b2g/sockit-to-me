// Global includes
#include <node.h>

// Local includes
#include "sockit.h"

// So we don't spend half the time typing 'v8::'
using namespace v8;

// Initialize
void InitAll(Handle<Object> aExports, Handle<Object> module) {
  Sockit::Init(aExports, module);
}

NODE_MODULE(sockit, InitAll)
