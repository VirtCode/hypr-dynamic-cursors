#include "src/debug/Log.hpp"

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#undef private

#include "src/render/Renderer.hpp"

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Shaders.hpp>

#include "shader.hpp"

void CInversionShader::compile(std::string vertex, std::string fragment) {

    program = g_pHyprOpenGL->createProgram(vertex, fragment);

    texAttrib            = glGetAttribLocation(program, "texcoord");
    posAttrib            = glGetAttribLocation(program, "pos");

    proj                 = glGetUniformLocation(program, "proj");
    screenOffset         = glGetUniformLocation(program, "screenOffset");
    backgroundTex        = glGetUniformLocation(program, "backgroundTex");
    cursorTex            = glGetUniformLocation(program, "cursorTex");
    alpha                = glGetUniformLocation(program, "alpha");
    chroma               = glGetUniformLocation(program, "chroma");
    chromaColor          = glGetUniformLocation(program, "chromaColor");
    mode                 = glGetUniformLocation(program, "mode");

    applyTint            = glGetUniformLocation(program, "applyTint");
    tint                 = glGetUniformLocation(program, "tint");
}

CInversionShader::~CInversionShader() {
    glDeleteProgram(program);
    program = 0;
}

CShaders::CShaders() {
    g_pHyprRenderer->makeEGLCurrent();

    rgba.compile(VERTEX, RGBA);
    ext.compile(VERTEX, EXT);
    rgbx.compile(VERTEX, RGBX);

    g_pHyprRenderer->unsetEGL();
}
