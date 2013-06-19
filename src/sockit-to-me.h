#ifndef __SOCKIT_TO_ME_H__
#define __SOCKIT_TO_ME_H__

#include <node.h>

class SockitToMe {
  public:
    static void Init(v8::Handle<v8::Object>);

  private:
    SockitToMe();
    ~SockitToMe();

    static v8::Handle<v8::Value> New(const v8::Arguments&);

    static v8::Handle<v8::Value> Connect();

    static v8::Handle<v8::Value> Read();

    static v8::Handle<v8::Value> Write();

    static v8::Handle<v8::Value> Close();
};

#endif
