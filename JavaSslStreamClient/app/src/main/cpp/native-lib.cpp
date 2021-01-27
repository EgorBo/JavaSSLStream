#include <jni.h>
#include <string>
#include "pal_jni.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_javasslstreamclient_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    std::string hello = "Hello from C++";
    return env->NewStringUTF(hello.c_str());
}