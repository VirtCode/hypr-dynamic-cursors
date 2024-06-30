#include "src/debug/Log.hpp"

#define private public
#include <hyprland/src/render/OpenGL.hpp>
#undef private

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/render/Shaders.hpp>

#include "shader.hpp"

void CInversionShader::compile(std::string vertex, std::string fragment) {

    program = g_pHyprOpenGL->createProgram(vertex, fragment);

    texAttrib            = glGetAttribLocation(program, "texcoord");
    posAttrib            = glGetAttribLocation(program, "pos");

    proj                 = glGetUniformLocation(program, "proj");
    backgroundTex        = glGetUniformLocation(program, "backgroundTex");
    cursorTex            = glGetUniformLocation(program, "cursorTex");
    alpha                = glGetUniformLocation(program, "alpha");
    applyTint            = glGetUniformLocation(program, "applyTint");
    tint                 = glGetUniformLocation(program, "tint");
}

CInversionShader::~CInversionShader() {
    glDeleteProgram(program);
    program = 0;
}

CShaders::CShaders() {
    RASSERT(eglMakeCurrent(wlr_egl_get_display(g_pCompositor->m_sWLREGL), EGL_NO_SURFACE, EGL_NO_SURFACE, wlr_egl_get_context(g_pCompositor->m_sWLREGL)),
                "Couldn't set current EGL!");

    rgba.compile(VERTEX, RGBA);
    ext.compile(VERTEX, EXT);
    rgbx.compile(VERTEX, RGBX);

    RASSERT(eglMakeCurrent(wlr_egl_get_display(g_pCompositor->m_sWLREGL), EGL_NO_SURFACE, EGL_NO_SURFACE, wlr_egl_get_context(g_pCompositor->m_sWLREGL)),
            "Couldn't set current EGL!");
}
