#include <string>
#include <hyprland/src/render/Shader.hpp>

// we need our own shader class as we have two textures
class CInversionShader {
  public:
    GLuint  program           = 0;

    GLint   posAttrib         = -1;
    GLint   texAttrib         = -1;

    GLint   proj              = -1;
    GLint   cursorTex         = -1;
    GLint   backgroundTex     = -1;
    GLint   alpha             = -1;
    GLint   mode              = -1;
    GLint   chroma            = -1;
    GLint   chromaColor       = -1;

    GLint   applyTint = -1;
    GLint   tint      = -1;

    void compile(std::string vertex, std::string fragment);
    ~CInversionShader();
};

class CShaders {
  public:
    CInversionShader rgba;
    CInversionShader rgbx;
    CInversionShader ext;

    CShaders();
};

inline std::unique_ptr<CShaders> g_pShaders;

inline const std::string VERTEX = R"#(
    uniform mat3 proj;

    attribute vec2 pos;
    attribute vec2 texcoord;

    varying vec2 v_texcoord;
    varying vec2 v_screencord;

    void main() {
        gl_Position = vec4(proj * vec3(pos, 1.0), 1.0);

        v_screencord = gl_Position.xy / 2.0 + vec2(0.5, 0.5); // transform to texture coords
        v_texcoord = texcoord;
    }
)#";

inline const std::string RGBA = R"#(
    precision highp float;

    varying vec2 v_texcoord; // is in 0-1
    varying vec2 v_screencord; // is in 0-1

    uniform sampler2D cursorTex;
    uniform sampler2D backgroundTex;
    uniform float alpha;

    uniform int applyTint;
    uniform vec3 tint;

    uniform int mode;
    uniform int chroma;
    uniform vec4 chromaColor;

    void main() {
        vec4 cursor = texture2D(cursorTex, v_texcoord);
        vec4 background = texture2D(backgroundTex, v_screencord);

        // invert based on mode
        if (mode == 0 || mode == 1) // invert
            background.rgb = vec3(1.0, 1.0, 1.0) - background.rgb;
        if (mode == 1 || mode == 2) // hueshift
            background.rgb = -background.rgb + dot(vec3(0.26312, 0.5283, 0.10488), background.rgb) * 2.0;

        background *= cursor[3]; // premultiplied alpha
        vec4 pixColor = background;

        // this is a very crude "chroma algorithm", feel free to contribute a better one
        if (chroma == 1) {
            float diff = (abs(chromaColor.x - cursor.x) + abs(chromaColor.y - cursor.y) + abs(chromaColor.z - cursor.z) + abs(chromaColor.w - cursor.w)) / 4.0;
            pixColor = background * (1.0 - diff) + cursor * diff;
        }

        if (applyTint == 1) {
    	    pixColor[0] = pixColor[0] * tint[0];
    	    pixColor[1] = pixColor[1] * tint[1];
    	    pixColor[2] = pixColor[2] * tint[2];
        }

        gl_FragColor = pixColor * alpha;
    }
)#";

inline const std::string RGBX = R"#(
    precision highp float;
    varying vec2 v_texcoord;
    uniform sampler2D cursorTex;
    uniform sampler2D backgroundTex;
    uniform float alpha;

    uniform int applyTint;
    uniform vec3 tint;

    void main() {


        vec4 pixColor = vec4(texture2D(cursorTex, v_texcoord).rgb, 1.0);

        if (applyTint == 1) {
       	    pixColor[0] = pixColor[0] * tint[0];
           	pixColor[1] = pixColor[1] * tint[1];
           	pixColor[2] = pixColor[2] * tint[2];
        }

        pixColor.r = 1.0;

        gl_FragColor = pixColor * alpha;
    }
)#";

inline const std::string EXT = R"#(
    #extension GL_OES_EGL_image_external : require

    precision highp float;
    varying vec2 v_texcoord;
    uniform samplerExternalOES texture0;
    uniform samplerExternalOES texture1;
    uniform float alpha;

    uniform int applyTint;
    uniform vec3 tint;

    void main() {

        vec4 pixColor = texture2D(texture0, v_texcoord);

        if (applyTint == 1) {
           	pixColor[0] = pixColor[0] * tint[0];
           	pixColor[1] = pixColor[1] * tint[1];
           	pixColor[2] = pixColor[2] * tint[2];
        }

        pixColor.r = 1.0;

        gl_FragColor = pixColor * alpha;
    }
)#";
