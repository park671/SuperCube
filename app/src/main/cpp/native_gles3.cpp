//
// Created by youngpark on 2023/4/1.
//

#include "native_gl.h"
#include "matrix.h"
#include "matrix_mul_neo.h"
#include <GLES3/gl3.h>
#include <android/log.h>

int getGLVersion() {
    return 3;
}

int degreeX, degreeY;

void rotate(int x, int y) {
    degreeX = (degreeX + x) % 360;
    degreeY = (degreeY + y) % 360;
}

unsigned int mProgramId, mPositionId, mColorId;

const char *VERTEX_SHADER = "#version 300 es\n"
                            "layout (location = 0) in vec4 vPosition;\n"
                            "layout (location = 1) in vec4 aColor;\n"
                            "uniform mat4 mvpMatrix;\n"
                            "out vec4 vColor;\n"
                            "void main() {\n"
                            "     gl_Position  = mvpMatrix * vPosition;\n"
                            "     vColor = aColor;\n"
                            "}\0";

const char *FRAGMENT_SHADER = "#version 300 es\n"
                              "precision mediump float;\n"
                              "in vec4 vColor;\n"
                              "out vec4 fragColor;\n"
                              "void main() {\n"
                              "     fragColor = vColor;\n"
                              "}\0";

unsigned int loadShader(const char *shaderCode, GLenum type) {
    int compiled;
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderCode, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        __android_log_print(ANDROID_LOG_ERROR, "native_GL", "compile err");
        return 0;
    }
    return shader;
}

unsigned int loadProgram(const char *VShaderCode, const char *FShaderCode) {
    unsigned int iVshader, iFShader, iProgramId;
    int link;
    iVshader = loadShader(VShaderCode, GL_VERTEX_SHADER);
    iFShader = loadShader(FShaderCode, GL_FRAGMENT_SHADER);
    if (!(iVshader && iFShader)) {
        __android_log_print(ANDROID_LOG_ERROR, "native_GL", "shader err");
        return 0;
    }
    iProgramId = glCreateProgram();
    glAttachShader(iProgramId, iVshader);
    glAttachShader(iProgramId, iFShader);
    glLinkProgram(iProgramId);
    glGetProgramiv(iProgramId, GL_LINK_STATUS, &link);
    if (!link) {
        __android_log_print(ANDROID_LOG_ERROR, "native_GL", "link err");
        return 0;
    }
    glDeleteShader(iVshader);
    glDeleteShader(iFShader);
    return iProgramId;
}

void init() {
    __android_log_print(ANDROID_LOG_DEBUG, "native_GL", "init");
    glClearColor(0.1f, 0.2f, 0.3f, 0.4f);
    glEnable(GL_DEPTH_TEST);
    mProgramId = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    glUseProgram(mProgramId);
}

int mRotateAgree = 0;

float mViewMatrix[16];
float mModelMatrix[16];
float mProjectMatrix[16];
float mMVPMatrix[16];

float mRatio;

float r = 1.0f;
const float vertex[] = {
        r, r, r, //0
        -r, r, r, //1
        -r, -r, r, //2
        r, -r, r, //3
        r, r, -r, //4
        -r, r, -r, //5
        -r, -r, -r, //6
        r, -r, -r //7
};
const unsigned char index[] = {
        0, 2, 1, 0, 2, 3, //前面
        0, 5, 1, 0, 5, 4, //上面
        0, 7, 3, 0, 7, 4, //右面
        6, 4, 5, 6, 4, 7, //后面
        6, 3, 2, 6, 3, 7, //下面
        6, 1, 2, 6, 1, 5 //左面
};
float c = 1.0f;
const float color[] = {
        c, c, c, 1,
        0, c, c, 1,
        0, 0, c, 1,
        c, 0, c, 1,
        c, c, 0, 1,
        0, c, 0, 1,
        0, 0, 0, 1,
        c, 0, 0, 1
};

float line_len = 3;

// vertices of lines
GLfloat lineXVertices[6] = {
        0.0f, 0.0f, 0.0f,
        line_len, 0.0f, 0.0f
};

GLfloat lineYVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, line_len, 0.0f
};

GLfloat lineZVertices[] = {
        0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, line_len
};

float mBaseMatrix[] = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
};


void transform() {
    float rotateMarix[16],rotateMarixY[16], scaleMatrix[16], translateMatrix[16];
    setRotateM(rotateMarix, 0, degreeY, 1, 0, 0);
    setRotateM(rotateMarixY, 0, degreeX, 0, 1, 0);
    matrix_multiply_4x4_neon_asm(rotateMarix, rotateMarixY, rotateMarix);
    scaleM(scaleMatrix, 0, mBaseMatrix, 0, 1.f, 1.f, 1.f);
    translateM(translateMatrix, 0, mBaseMatrix, 0, -0.5f, -0.5f, 0.0f);
    matrix_multiply_4x4_neon_asm(rotateMarix, scaleMatrix, mModelMatrix);
    matrix_multiply_4x4_neon_asm(mModelMatrix, translateMatrix, mModelMatrix);

    //设置透视投影
    frustumM(mProjectMatrix, 0, -mRatio, mRatio, -1, 1, 2, 300);
    //设置相机位置
    setLookAtM(mViewMatrix, 0, 0.0f, -0.0f, 10.0f, 0.f, 0.f, 0.f, 0.f, 1.0f, 0.0f);
    //计算变换矩阵
    matrix_multiply_4x4_neon_asm(mProjectMatrix, mViewMatrix, mMVPMatrix);
    matrix_multiply_4x4_neon_asm(mMVPMatrix, mModelMatrix, mMVPMatrix);
}

void onChange(int width, int height) {
    __android_log_print(ANDROID_LOG_DEBUG, "native_GL", "onChange");
    glViewport(0, 0, width, height);
    mRatio = (1.0f * width) / (1.f * height);
}

void onDraw() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    mRotateAgree = (mRotateAgree + 2) % 360;
    transform(); //计算MVP变换矩阵
    int mvpMatrixHandle = glGetUniformLocation(mProgramId, "mvpMatrix");
    glUniformMatrix4fv(mvpMatrixHandle, 1, false, mMVPMatrix);
    //启用顶点的数组句柄
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    //准备顶点坐标和颜色数据
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertex);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, color);
    //绘制正方体的表面（6个面，每面2个三角形，每个三角形3个顶点）
    glDrawElements(GL_TRIANGLES, 6 * 2 * 3, GL_UNSIGNED_BYTE, index);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, lineXVertices);
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, lineYVertices);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 0, lineZVertices);
    glDrawArrays(GL_LINES, 0, 2);

    //禁止顶点数组句柄
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
}