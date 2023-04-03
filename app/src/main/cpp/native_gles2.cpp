//
// Created by youngpark on 2023/4/1.
//

#include "native_gl.h"
#include <GLES2/gl2.h>
#include <android/log.h>
#include <cmath>
#include "matrix.h"
#include "matrix_mul_neo.h"

int getGLVersion() {
    return 2;
}

const char *VERTEX_SHADER = "attribute vec4 vPosition;"
                            "uniform mat4 vMatrix;"
                            "varying  vec4 vColor;"
                            "attribute vec4 aColor;"
                            "void main() {"
                            "  gl_Position = vMatrix*vPosition;"
                            "  vColor=aColor;"
                            "}\0";

const char *FRAGMENT_SHADER = "precision mediump float;"
                              "varying vec4 vColor;"
                              "void main() {"
                              "  gl_FragColor = vColor;"
                              "}\0";

int COORDS_PER_VERTEX = 3;
const float cubePositions[] = {
        -1.0f, 1.0f, 1.0f,    //正面左上0
        -1.0f, -1.0f, 1.0f,   //正面左下1
        1.0f, -1.0f, 1.0f,    //正面右下2
        1.0f, 1.0f, 1.0f,     //正面右上3
        -1.0f, 1.0f, -1.0f,    //反面左上4
        -1.0f, -1.0f, -1.0f,   //反面左下5
        1.0f, -1.0f, -1.0f,    //反面右下6
        1.0f, 1.0f, -1.0f,     //反面右上7
};
const unsigned char index[] = {
        6, 7, 4, 6, 4, 5,    //后面
        6, 3, 7, 6, 2, 3,    //右面
        6, 5, 1, 6, 1, 2,    //下面
        0, 3, 2, 0, 2, 1,    //正面
        0, 1, 5, 0, 5, 4,    //左面
        0, 7, 3, 0, 4, 7,    //上面
};

const float color[] = {
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        0.f, 1.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
        1.f, 0.f, 0.f, 1.f,
};

int mPositionHandle;
int mColorHandle;

float mViewMatrix[16];
float mModelMatrix[16];
float mProjectMatrix[16];
float mMVPMatrix[16];

int mMatrixHandler;

int mProgram;
int degreeX, degreeY;

void rotate(int x, int y) {
    degreeX = (degreeX + x) % 360;
    degreeY = (degreeY + y) % 360;
}

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
    mProgram = loadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
    glEnable(GL_DEPTH_TEST);
}

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

int rotate_degree = 0;
float ratio = 0;

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
    frustumM(mProjectMatrix, 0, -ratio, ratio, -1, 1, 2, 300);
    //设置相机位置
    setLookAtM(mViewMatrix, 0, 0.0f, -0.0f, 10.0f, 0.f, 0.f, 0.f, 0.f, 1.0f, 0.0f);
    //计算变换矩阵
    matrix_multiply_4x4_neon_asm(mProjectMatrix, mViewMatrix, mMVPMatrix);
    matrix_multiply_4x4_neon_asm(mMVPMatrix, mModelMatrix, mMVPMatrix);
}

void onChange(int width, int height) {
    __android_log_print(ANDROID_LOG_DEBUG, "native_GL", "onChange");
    ratio = (float) width / height;
    transform();
}

void onDraw() {
    glClear(GL_DEPTH_BUFFER_BIT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    //将程序加入到OpenGLES2.0环境
    glUseProgram(mProgram);
    rotate_degree = (rotate_degree + 2) % 360;
    transform();
    //获取变换矩阵vMatrix成员句柄
    mMatrixHandler = glGetUniformLocation(mProgram, "vMatrix");
    //指定vMatrix的值
    glUniformMatrix4fv(mMatrixHandler, 1, false, mMVPMatrix);
    //获取顶点着色器的vPosition成员句柄
    mPositionHandle = glGetAttribLocation(mProgram, "vPosition");
    //启用三角形顶点的句柄
    glEnableVertexAttribArray(mPositionHandle);
    //准备三角形的坐标数据
    glVertexAttribPointer(mPositionHandle, 3,
                          GL_FLOAT, false,
                          0, cubePositions);
    //获取片元着色器的vColor成员的句柄
    mColorHandle = glGetAttribLocation(mProgram, "aColor");
    //设置绘制三角形的颜色
//        glUniform4fv(mColorHandle, 2, color, 0);
    glEnableVertexAttribArray(mColorHandle);
    glVertexAttribPointer(mColorHandle, 4,
                          GL_FLOAT, false,
                          0, color);
    //索引法绘制正方体
    glDrawElements(GL_TRIANGLES, 6 * 3 * 2, GL_UNSIGNED_BYTE, index);
    //禁止顶点数组的句柄
    glDisableVertexAttribArray(mPositionHandle);
}