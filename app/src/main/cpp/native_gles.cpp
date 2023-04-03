//
// Created by youngpark on 2023/4/1.
//

#include "native_gl.h"
#include <GLES/gl.h>
#include <android/log.h>
#include <cmath>

int getGLVersion() {
    return 1;
}

float vertices[] = {
        -1.0f, 1.0f, 1.f, // top left
        -1.0f, -1.0f, 1.f, // bottom left
        1.0f, -1.0f, 1.f,  //top right
        -1.0f, 1.0f, 1.f, //bottom left
        1.0f, -1.0f, 1.f, //bottom right
        1.0f, 1.0f, 1.f,    //top right	//前面

        1.0f, 1.0f, 1.f,
        1.0f, -1.0f, 1.f,
        1.0f, -1.0f, -1.f,
        1.0f, 1.0f, 1.f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.f,        //右面

        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,  //左面

        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,   //后面

        -1.0f, 1.0f, -1.0f,    // top left
        -1.0f, 1.0f, 1.0f,    //bottom left
        1.0f, 1.0f, -1.0f,    //top right
        -1.0f, 1.0f, 1.0f,    //bottom left
        1.0f, 1.0f, 1.0f,        //top right
        1.0f, 1.0f, -1.0f,    // -top right上面

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,    //下面
};
//立方体的顶点颜色
float colors[] = {
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,

        1.f, 0.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,
        1.f, 0.f, 1.f, 1.f,

        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,

        0.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,
        0.f, 0.f, 1.f, 1.f,

        0.5f, 0.f, 1.f, 1.f,
        0.5f, 0.f, 1.f, 1.f,
        0.5f, 0.f, 1.f, 1.f,
        0.5f, 0.f, 1.f, 1.f,
        0.5f, 0.f, 1.f, 1.f,
        0.5f, 0.f, 1.f, 1.f,

        1.f, 0.f, 0.5f, 1.f,
        1.f, 0.f, 0.5f, 1.f,
        1.f, 0.f, 0.5f, 1.f,
        1.f, 0.f, 0.5f, 1.f,
        1.f, 0.f, 0.5f, 1.f,
        1.f, 0.f, 0.5f, 1.f,
};

int mRatio = 0;

void init() {
    __android_log_print(ANDROID_LOG_DEBUG, "native_GL", "init");
    glEnable(GL_DEPTH_TEST);
    // 所做深度测试的类型
    glDepthFunc(GL_DITHER);
    //黑色背景
    glClearColor(0.f, 0.f, 0.f, 0.5f);
    //启用阴影平滑
    glShadeModel(GL_SMOOTH);
    //清除深度缓存
    glClearDepthf(1.0f);

    glEnable(GL_TEXTURE_2D);
    //告诉系统对透视进行修正
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void onChange(int width, int height) {
    __android_log_print(ANDROID_LOG_DEBUG, "native_GL", "onChange");
    glViewport(0, 0, width, height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float ratio = (float) width / (float) height;
    glFrustumf(-ratio, ratio, -1, 1, 2, 300);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void onDraw() {
    glClearColor(0.f, 0.f, 0.f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);  //设置矩阵模式
    glFrontFace(GL_CCW);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    //开启顶点和纹理缓冲
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, 0, vertices);
    glColorPointer(4, GL_FLOAT, 0, colors);
    glLoadIdentity();
    glTranslatef(0, 0, -10);
    glRotatef(mRatio, 1.f, 0.f, 0.f);        //往上面倾斜(x轴)倾斜,根据每次得到的角度

    glDrawArrays(GL_TRIANGLES, 0, 107);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisable(GL_CULL_FACE);
    mRatio = (mRatio + 2) % 360;            //旋转角度减1
}