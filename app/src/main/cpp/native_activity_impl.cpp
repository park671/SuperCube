//
// Created by youngpark on 2023/4/2.
//
#include "android_app_glue/android_native_app_glue.h"
#include <android/log.h>
#include <EGL/egl.h>
#include "native_gl.h"
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <unistd.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", __VA_ARGS__))

/*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
const EGLint surface_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_NONE
};

EGLint context_attrib_list[] = {EGL_CONTEXT_CLIENT_VERSION, getGLVersion(),
                     EGL_NONE };

EGLint format;
EGLint height, width;
EGLint major, minor;

EGLSurface surface;
EGLContext context;
EGLDisplay eglDisplay;

void initEGL(android_app *app){
    //初始化EGL
    EGLint configCount;
    eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    eglInitialize(eglDisplay, &major, &minor);
    eglChooseConfig(eglDisplay, surface_attribs, nullptr, 0, &configCount);
    if(configCount == 0) {
        return;
    }
    EGLConfig *config = (EGLConfig *)malloc(configCount * sizeof (EGLConfig));
    eglChooseConfig(eglDisplay, surface_attribs, config, configCount, &configCount);

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(eglDisplay, config[0], EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(eglDisplay, config[0], app->window, nullptr);
    context = eglCreateContext(eglDisplay, config[0], nullptr, context_attrib_list);
    eglMakeCurrent(eglDisplay, surface, surface, context);

    eglQuerySurface(eglDisplay, surface, EGL_WIDTH, &width);
    eglQuerySurface(eglDisplay, surface, EGL_HEIGHT, &height);

}


void handle_app_cmd(struct android_app* app, int32_t cmd) {
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            initEGL(app);
            init();
            onChange(width, height);
            onDraw();
            eglSwapBuffers(eglDisplay, surface);
            break;
    }
}

int lastX, lastY;

int32_t handle_input_event(struct android_app* app, AInputEvent* event){
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        int x = AMotionEvent_getX(event, 0);
        int y = AMotionEvent_getY(event, 0);
        switch (AMotionEvent_getAction(event)) {
            case AMOTION_EVENT_ACTION_DOWN:
                x = lastX;
                y = lastY;
                break;
            case AMOTION_EVENT_ACTION_MOVE:
                rotate(x - lastX, y - lastY);
                lastX = x;
                lastY = y;
                break;
            case AMOTION_EVENT_ACTION_UP:
                break;
        }
        return 1;
    }
    return 0;
}

void android_main(struct android_app* app) {
    LOGI("native_activity", "main start");
    app->onAppCmd = handle_app_cmd;
    app->onInputEvent = handle_input_event;
    int events;
    android_poll_source *source;
    while(true) {
        int indent = ALooper_pollAll(-1, nullptr, &events, (void **)(&source));
        if(source != nullptr) {
            source->process(app, source);
        }
        if(indent == LOOPER_ID_USER) {

        }
        if(app->destroyRequested) {
            return;
        }
        onDraw();
        eglSwapBuffers(eglDisplay, surface);
    }
}