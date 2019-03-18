/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <kodi/addon-instance/Visualization.h>
#if defined(HAS_GLES)
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <assert.h>
#define TO_STRING(...) #__VA_ARGS__
#else
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif
#endif
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <streambuf>
#include <ctime>
#include <p8-platform/util/timeutils.h>
#include <math.h>
#include <complex.h>
#include <limits.h>
#include <fstream>
#include <sstream>

#include "kiss_fft.h"
#include "lodepng.h"

#include "jsmn/jsmn.h"
#include <stdlib.h>       // strtol

static void launch(int preset);

using namespace std;

string g_pathPresets;

struct Preset {
  std::string name;
  std::string file;
  int channel[4];
};

int testingPresets = 0; // just to not select TEST vizs when randomly selecting presets

std::vector<Preset> g_presets; // filled in ADDON_Create() from json file

// json file under "./resources/" that contains the list of presets (g_presets)
#if defined(HAS_GLES)
  char const *presetsFile = "presets_GLES.json";
#else
  char const *presetsFile = "presets.json";
#endif

int g_currentPreset = 0;

const char *g_fileTextures[] = {
  "tex00.png",
  "tex01.png",
  "tex02.png",
  "tex03.png",
  "tex04.png",
  "tex05.png",
  "tex06.png",
  "tex07.png",
  "tex08.png",
  "tex09.png",
  "tex10.png",
  "tex11.png",
  "tex12.png",
  "tex15.png",
  "tex16.png",
  "tex14.png",
};

#if defined(HAS_GLES)
struct
{
  GLuint vertex_buffer;
  GLuint attr_vertex_e;
  GLuint attr_vertex_r, uTexture;
  GLuint effect_fb;
  GLuint framebuffer_texture;
  GLuint render_program;
  GLuint uScale;
  int fbwidth, fbheight;
} state_g, *state = &state_g;
#endif

int g_numberTextures = 17;
GLint g_textures[17] = { };

void LogProps(AddonProps_Visualization *props) {
  cout << "Props = {" << endl
       << "\t x: " << props->x << endl
       << "\t y: " << props->y << endl
       << "\t width: " << props->width << endl
       << "\t height: " << props->height << endl
       << "\t pixelRatio: " << props->pixelRatio << endl
       << "\t name: " << props->name << endl
       << "\t presets: " << props->presets << endl
       << "\t profile: " << props->profile << endl
//       << "\t submodule: " << props->submodule << endl // Causes problems? Is it initialized?
       << "}" << endl;
}

void LogTrack(VisTrack *track) {
  cout << "Track = {" << endl
       << "\t title: " << track->title << endl
       << "\t artist: " << track->artist << endl
       << "\t album: " << track->album << endl
       << "\t albumArtist: " << track->albumArtist << endl
       << "\t genre: " << track->genre << endl
       << "\t comment: " << track->comment << endl
       << "\t lyrics: " << track->lyrics << endl
       << "\t trackNumber: " << track->trackNumber << endl
       << "\t discNumber: " << track->discNumber << endl
       << "\t duration: " << track->duration << endl
       << "\t year: " << track->year << endl
       << "\t rating: " << track->rating << endl
       << "}" << endl;
}

void LogAction(const char *message) {
  cout << "Action " << message << endl;
}

void LogActionString(const char *message, const char *param) {
  cout << "Action " << message << " " << param << endl;
}

float blackmanWindow(float in, size_t i, size_t length) {
  double alpha = 0.16;
  double a0 = 0.5 * (1.0 - alpha);
  double a1 = 0.5;
  double a2 = 0.5 * alpha;

  float x = (float)i / (float)length;
  return in * (a0 - a1 * cos(2.0 * M_PI * x) + a2 * cos(4.0 * M_PI * x));
}

void smoothingOverTime(float *outputBuffer, float *lastOutputBuffer, kiss_fft_cpx *inputBuffer, size_t length, float smoothingTimeConstant, unsigned int fftSize) {
  for (size_t i = 0; i < length; i++) {
    kiss_fft_cpx c = inputBuffer[i];
    float magnitude = sqrt(c.r * c.r + c.i * c.i) / (float)fftSize;
    outputBuffer[i] = smoothingTimeConstant * lastOutputBuffer[i] + (1.0 - smoothingTimeConstant) * magnitude;
  }
}

float linearToDecibels(float linear) {
  if (!linear)
    return -1000;
  return 20 * log10f(linear);
}

#define SMOOTHING_TIME_CONSTANT (0.8)
#define MIN_DECIBELS (-100.0)
#define MAX_DECIBELS (-30.0)

#define AUDIO_BUFFER (1024)
#define NUM_BANDS (AUDIO_BUFFER / 2)

GLuint createTexture(GLint format, unsigned int w, unsigned int h, const GLvoid * data) {
  GLuint texture = 0;
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D,  GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
  return texture;
}

GLuint createTexture(const GLvoid *data, GLint format, unsigned int w, unsigned int h, GLint internalFormat, GLint scaling, GLint repeat) {
  GLuint texture = 0;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, scaling);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, scaling);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, repeat);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, repeat);

#if defined(HAS_GLES)
  glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
#else
  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);
#endif
  glBindTexture(GL_TEXTURE_2D, 0);

  return texture;
}

GLuint createTexture(const char *file, GLint internalFormat, GLint scaling, GLint repeat) {
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

  cout << "creating texture " << fullPath << endl;


  unsigned error;
  unsigned char* image;
  unsigned width, height;

  error = lodepng_decode32_file(&image, &width, &height, fullPath.c_str());
  if (error) {
    printf("error %u: %s\n", error, lodepng_error_text(error));
    return 0;
  }

  GLuint texture = createTexture(image, GL_RGBA, width, height, internalFormat, scaling, repeat);
  free(image);
  return texture;
}

GLuint compileShader(GLenum shaderType, const char *shader) {
  GLuint s = glCreateShader(shaderType);
  if (s == 0) {
    cerr << "Failed to create shader from\n====" << endl;
    cerr << shader << endl;
    cerr << "===" << endl;

    return 0;
  }

  glShaderSource(s, 1, &shader, NULL);
  glCompileShader(s);

  GLint param;
  glGetShaderiv(s, GL_COMPILE_STATUS, &param);
  if (param != GL_TRUE) {
    cerr << "Failed to compile shader source\n====" << endl;
    cerr << shader << endl;
    cerr << "===" << endl;

    int infologLength = 0;
    char *infoLog;

    glGetShaderiv(s, GL_INFO_LOG_LENGTH, &infologLength);

    if (infologLength > 0) {
      infoLog = new char[infologLength];
      glGetShaderInfoLog(s, infologLength, NULL, infoLog);
	    cout << "<log>" << endl << infoLog << endl << "</log>" << endl;
      delete [] infoLog;
    }

    glDeleteShader(s);

    return 0;
  }

  return s;
}

GLuint compileAndLinkProgram(const char *vertexShader, const char *fragmentShader) {
  cout << "CompileAndLink " << endl;
  GLuint program = glCreateProgram();
  if (program == 0) {
    cerr << "Failed to create program" << endl;
    return 0;
  }

  GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShader);
  GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShader);

  if (vs && fs) {
    glAttachShader(program, vs);
    glAttachShader(program, fs);
    glLinkProgram(program);

    GLint param;
    glGetProgramiv(program, GL_LINK_STATUS, &param);
    if (param != GL_TRUE) {
      cerr << "Failed to link shader program " << endl;
      glGetError();
      int infologLength = 0;
      char *infoLog;

      glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infologLength);

      if (infologLength > 0) {
        infoLog = new char[infologLength];
        glGetProgramInfoLog(program, infologLength, NULL, infoLog);
	      cout << "<log>" << endl << infoLog << endl << "</log>" << endl;
        delete [] infoLog;
      }

      GLchar errorLog[1024] = {0};
      glGetProgramInfoLog(program, 1024, NULL, errorLog);

      cout << "<vertexShader>" << endl << vertexShader << endl << "</vertexShader>" << endl;
      cout << "<fragmentShader>" << endl << fragmentShader << endl << "</fragmentShader>" << endl;

      glDetachShader(program, vs);
      glDeleteShader(vs);

      glDetachShader(program, fs);
      glDeleteShader(fs);

      glDeleteProgram(program);
      return 0;
    }
  } else {
  	glDeleteProgram(program);
  }

  glUseProgram(0);

  if (vs)
    glDeleteShader(vs);

  if (fs)
    glDeleteShader(fs);

  return program;
}


#if defined(HAS_GLES)

std::string vsSource = TO_STRING(
         precision mediump float;
         precision mediump int;
         attribute vec4 vertex;
         varying vec2 vTextureCoord;
         uniform vec2 uScale;
         void main(void)
         {
            gl_Position = vertex;
            vTextureCoord = vertex.xy*0.5+0.5;
            vTextureCoord.x = vTextureCoord.x * uScale.x;
            vTextureCoord.y = vTextureCoord.y * uScale.y;
         }
  );

std::string render_vsSource = TO_STRING(
         precision mediump float;
         precision mediump int;
         attribute vec4 vertex;
         varying vec2 vTextureCoord;
         void main(void)
         {
            gl_Position = vertex;
            vTextureCoord = vertex.xy*0.5+0.5;
         }
  );

std::string render_fsSource = TO_STRING(
         precision mediump float;
         precision mediump int;
         varying vec2 vTextureCoord;
         uniform sampler2D uTexture;
         void main(void)
         {
            gl_FragColor = texture2D(uTexture, vTextureCoord);
         }
  );
#else
std::string vsSource = "void main() { gl_Position = ftransform(); }";
#endif

std::string fsHeader =
"#extension GL_OES_standard_derivatives : enable\n"
#ifdef HAS_GLES
"precision mediump float;\n"
"precision mediump int;\n"
#endif
"uniform vec3      iResolution;\n"
"uniform float     iGlobalTime;\n"
"uniform float     iChannelTime[4];\n"
"uniform vec4      iMouse;\n"
"uniform vec4      iDate;\n"
"uniform float     iSampleRate;\n"
"uniform vec3      iChannelResolution[4];\n"
"uniform sampler2D iChannel0;\n"
"uniform sampler2D iChannel1;\n"
"uniform sampler2D iChannel2;\n"
"uniform sampler2D iChannel3;\n";

std::string fsFooter =
"void main(void)\n"
"{\n"
"  vec4 color = vec4(0.0, 0.0, 0.0, 1.0);\n"
"  mainImage(color, gl_FragCoord.xy);\n"
"  color.w = 1.0;\n"
"  gl_FragColor = color;\n"
"}\n";

bool initialized = false;

GLuint shadertoy_shader = 0;

GLint iResolutionLoc        = 0;
GLint iGlobalTimeLoc        = 0;
GLint iChannelTimeLoc       = 0;
GLint iMouseLoc             = 0;
GLint iDateLoc              = 0;
GLint iSampleRateLoc        = 0;
GLint iChannelResolutionLoc = 0;
GLint iChannelLoc[4];
GLuint iChannel[4];

int width = 0;
int height = 0;

int64_t initial_time;
int bits_precision = 0;

bool needsUpload = true;

kiss_fft_cfg cfg;

float *pcm = NULL;
float *magnitude_buffer = NULL;
GLubyte *audio_data = NULL;
int samplesPerSec = 0;

void unloadTextures() {
  for (int i=0; i<4; i++) {
    if (iChannel[i]) {
      cout << "Unloading iChannel" << i << " " << iChannel[i] << endl;
      glDeleteTextures(1, &iChannel[i]);
      iChannel[i] = 0;
    }
  }
}

void unloadPreset() {
  if (shadertoy_shader) {
    glDeleteProgram(shadertoy_shader);
    shadertoy_shader = 0;
  }
#if defined(HAS_GLES)
  if (state->framebuffer_texture)
  {
    glDeleteTextures(1, &state->framebuffer_texture);
    state->framebuffer_texture = 0;
  }
  if (state->effect_fb)
  {
    glDeleteFramebuffers(1, &state->effect_fb);
    state->effect_fb = 0;
  }
  if (state->render_program) {
    glDeleteProgram(state->render_program);
    state->render_program = 0;
  }
#endif
}

std::string createShader(const std::string &file)
{
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

  cout << "Creating shader from " << fullPath << endl;

  std::ifstream t(fullPath);
  std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

  //std::string str = "void mainImage( out vec4 fragColor, in vec2 fragCoord ) { fragColor = texture2D(iChannel1, fragCoord.xy / iResolution.xy); }";

  std::string fsSource = fsHeader + "\n" + str + "\n" + fsFooter;
  return fsSource;
}

GLint loadTexture(int number)
{
  if (number >= 0 && number < g_numberTextures) {
    GLint format = GL_RGBA;
    GLint scaling = GL_LINEAR;
    GLint repeat = GL_REPEAT;
    return createTexture(g_fileTextures[number], format, scaling, repeat);
  }
  else if (number == 99) { // framebuffer
    return createTexture(GL_LUMINANCE, NUM_BANDS, 2, audio_data);
  }
  return 0;
}

void loadPreset(int preset, std::string vsSource, std::string fsSource)
{
  unloadPreset();
  shadertoy_shader = compileAndLinkProgram(vsSource.c_str(), fsSource.c_str());

  iResolutionLoc        = glGetUniformLocation(shadertoy_shader, "iResolution");
  iGlobalTimeLoc        = glGetUniformLocation(shadertoy_shader, "iGlobalTime");
  iChannelTimeLoc       = glGetUniformLocation(shadertoy_shader, "iChannelTime");
  iMouseLoc             = glGetUniformLocation(shadertoy_shader, "iMouse");
  iDateLoc              = glGetUniformLocation(shadertoy_shader, "iDate");
  iSampleRateLoc        = glGetUniformLocation(shadertoy_shader, "iSampleRate");
  iChannelResolutionLoc = glGetUniformLocation(shadertoy_shader, "iChannelResolution");
  iChannelLoc[0]        = glGetUniformLocation(shadertoy_shader, "iChannel0");
  iChannelLoc[1]        = glGetUniformLocation(shadertoy_shader, "iChannel1");
  iChannelLoc[2]        = glGetUniformLocation(shadertoy_shader, "iChannel2");
  iChannelLoc[3]        = glGetUniformLocation(shadertoy_shader, "iChannel3");

#if defined(HAS_GLES)
  state->uScale         = glGetUniformLocation(shadertoy_shader, "uScale");
  state->attr_vertex_e  = glGetAttribLocation(shadertoy_shader,  "vertex");

  state->render_program = compileAndLinkProgram(render_vsSource.c_str(), render_fsSource.c_str());
  state->uTexture       = glGetUniformLocation(state->render_program, "uTexture");
  state->attr_vertex_r  = glGetAttribLocation(state->render_program,  "vertex");
  // Prepare a texture to render to
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &state->framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, state->framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, state->fbwidth, state->fbheight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  // Prepare a framebuffer for rendering
  glGenFramebuffers(1, &state->effect_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, state->effect_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, state->framebuffer_texture, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  initial_time = P8PLATFORM::GetTimeMs();
#endif
}

static uint64_t GetTimeStamp() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
}

static void RenderTo(GLuint shader, GLuint effect_fb)
{
  glUseProgram(shader);

#if !defined(HAS_GLES)
  glDisable(GL_BLEND);
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glOrtho(-1, 1, -1, 1, -1, 1);
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);
  glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

  glClear(GL_DEPTH_BUFFER_BIT);
  glPushMatrix();
#endif

  if (shader == shadertoy_shader) {
    GLuint w = width, h = height;
#if defined(HAS_GLES)
    if (state->fbwidth && state->fbheight)
      w = state->fbwidth, h = state->fbheight;
#endif
    int64_t intt = P8PLATFORM::GetTimeMs() - initial_time;
    if (bits_precision)
      intt &= (1<<bits_precision)-1;

    if (needsUpload) {
      for (int i=0; i<4; i++) {
        if (g_presets[g_currentPreset].channel[i] == 99) {
          glActiveTexture(GL_TEXTURE0 + i);
          glBindTexture(GL_TEXTURE_2D, iChannel[i]);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, NUM_BANDS, 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, audio_data);
        }
      }
      needsUpload = false;
    }

    float t = intt / 1000.0f;
    GLfloat tv[] = { t, t, t, t };

    glUniform3f(iResolutionLoc, w, h, 0.0f);
    glUniform1f(iGlobalTimeLoc, t);
    glUniform1f(iSampleRateLoc, samplesPerSec);
    glUniform1fv(iChannelTimeLoc, 4, tv);
#if defined(HAS_GLES)
    glUniform2f(state->uScale, (GLfloat)width/state->fbwidth, (GLfloat)height/state->fbheight);
#endif
    time_t now = time(NULL);
    tm *ltm = localtime(&now);

    float year = 1900 + ltm->tm_year;
    float month = ltm->tm_mon;
    float day = ltm->tm_mday;
    float sec = (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60) + ltm->tm_sec;

    glUniform4f(iDateLoc, year, month, day, sec);

    for (int i=0; i<4; i++) {
      glActiveTexture(GL_TEXTURE0 + i);
#if !defined(HAS_GLES)
      glEnable(GL_TEXTURE_2D);
#endif
      glUniform1i(iChannelLoc[i], i);
      glBindTexture(GL_TEXTURE_2D, iChannel[i]);
    }
  } else {
#if defined(HAS_GLES)
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, state->framebuffer_texture);
    glUniform1i(state->uTexture, 0); // first currently bound texture "GL_TEXTURE0"
#endif
  }

#if defined(HAS_GLES)
  // Draw the effect to a texture or direct to framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, effect_fb);

  GLuint attr_vertex = shader == shadertoy_shader ? state->attr_vertex_e : state->attr_vertex_r;
  glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
  glVertexAttribPointer(attr_vertex, 4, GL_FLOAT, 0, 16, 0);
  glEnableVertexAttribArray(attr_vertex);
  glDrawArrays( GL_TRIANGLE_FAN, 0, 4 );
  glDisableVertexAttribArray(attr_vertex);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
#else
  glBegin(GL_QUADS);
    glVertex3f(-1.0f, 1.0f, 0.0f);
    glVertex3f( 1.0f, 1.0f, 0.0f);
    glVertex3f( 1.0f,-1.0f, 0.0f);
    glVertex3f(-1.0f,-1.0f, 0.0f);
  glEnd();
#endif

  for (int i=0; i<4; i++) {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0);
  }
#if !defined(HAS_GLES)
  glPopMatrix();
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
#endif
  glUseProgram(0);
}


class CVisualizationShadertoy
  : public kodi::addon::CAddonBase
  , public kodi::addon::CInstanceVisualization
{
public:
  virtual ~CVisualizationShadertoy();

  virtual ADDON_STATUS Create() override;

  virtual bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  virtual void Stop() override;
  virtual void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) override;
  virtual void Render() override;
  virtual bool GetPresets(std::vector<std::string>& presets) override;
  virtual int GetActivePreset() override;
  virtual bool PrevPreset() override;
  virtual bool NextPreset() override;
  virtual bool LoadPreset(int select) override;
  virtual bool RandomPreset() override;
};

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationShadertoy::Render()
{
  glGetError();
  //cout << "Render" << std::endl;
  if (initialized) {
#if defined(HAS_GLES)
    if (state->fbwidth && state->fbheight) {
      RenderTo(shadertoy_shader, state->effect_fb);
      RenderTo(state->render_program, 0);
    } else {
      RenderTo(shadertoy_shader, 0);
    }
#else
    RenderTo(shadertoy_shader, 0);
#endif
    static int frames = 0;
    static uint64_t ts;
    if (frames == 0) {
      ts = GetTimeStamp();
    }
    frames++;
    uint64_t ts2 = GetTimeStamp();
    if (ts2 - ts > 1e6)
    {
     printf("%d fps\n", frames);
     ts += 1e6;
     frames = 0;
    }
  }
}

#ifdef HAS_GLES
static int determine_bits_precision()
{
  std::string vsPrecisionSource = TO_STRING(
	void mainImage( out vec4 fragColor, in vec2 fragCoord )
	{
	    float y = ( fragCoord.y / iResolution.y ) * 26.0;
	    float x = 1.0 - ( fragCoord.x / iResolution.x );
	    float b = fract( pow( 2.0, floor(y) ) + x );
	    if (fract(y) >= 0.9)
		b = 0.0;
	    fragColor = vec4(b, b, b, 1.0 );
	}
  );
  std::string fsPrecisionSource = fsHeader + "\n" + vsPrecisionSource + "\n" + fsFooter;

  state->fbwidth = 32, state->fbheight = 26*10;
  loadPreset(0, vsSource, fsPrecisionSource);
  RenderTo(shadertoy_shader, state->effect_fb);
  glFinish();

  unsigned char *buffer = new unsigned char[state->fbwidth * state->fbheight * 4];
  if (buffer)
    glReadPixels(0, 0, state->fbwidth, state->fbheight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  #if 0
  for (int j=0; j<state->fbheight; j++) {
    for (int i=0; i<state->fbwidth; i++) {
      printf("%02x ", buffer[4*(j*state->fbwidth+i)]);
    }
    printf("\n");
  }
  #endif
  int bits = 0;
  unsigned char b = 0; 
  for (int j=0; j<state->fbheight; j++) {
    unsigned char c = buffer[4*(j*state->fbwidth+(state->fbwidth>>1))];
    if (c && !b)
      bits++;
    b = c;
  }
  delete buffer;
  unloadPreset();
  return bits;
}

static double measure_performance(int preset, int size)
{
  int iterations = -1;
  std::string fsSource = createShader(g_presets[preset].file);

  state->fbwidth = state->fbheight = size;
  loadPreset(preset, vsSource, fsSource);

  int64_t end, start;
  do {
#if defined(HAS_GLES)
    RenderTo(shadertoy_shader, state->effect_fb);
    RenderTo(state->render_program, state->effect_fb);
#else
    RenderTo(shadertoy_shader, 0);
#endif
    glFinish();
    if (++iterations == 0)
      start = P8PLATFORM::GetTimeMs();
    end = P8PLATFORM::GetTimeMs();
  } while (end - start < 50);
  double t = (double)(end - start)/iterations;
  //printf("%s %dx%d %.1fms = %.2f fps\n", __func__, size, size, t, 1000.0/t);
  unloadPreset();
  return t;
}
#endif

static void launch(int preset)
{
#ifdef HAS_GLES
  bits_precision = determine_bits_precision();
  // mali-400 has only 10 bits which means milliseond timer wraps after ~1 second.
  // we'll fudge that up a bit as having a larger range is more important than ms accuracy
  bits_precision = max(bits_precision, 13);
  printf("bits=%d\n", bits_precision);
#endif
  
  unloadTextures();
  for (int i=0; i<4; i++) {
    if (g_presets[preset].channel[i] >= 0)
      iChannel[i] = loadTexture(g_presets[preset].channel[i]);
  }

#ifdef HAS_GLES
  const int size1 = 256, size2=512;
  double t1 = measure_performance(preset, size1);
  double t2 = measure_performance(preset, size2);
 
  double expected_fps = 40.0;
  // time per pixel for rendering fragment shader
  double B = (t2-t1)/(size2*size2-size1*size1);
  // time to render to screen
  double A = t2 - size2*size2 * B;
  // how many pixels get the desired fps
  double pixels = (1000.0/expected_fps - A) / B;
  state->fbwidth = sqrtf(pixels * width / height);
  if (state->fbwidth * 4 >= width * 3)
    state->fbwidth = 0;
  else if (state->fbwidth < 320)
    state->fbwidth = 320;
  state->fbheight = state->fbwidth * height / width;

  printf("expected fps=%f, pixels=%f %dx%d (A:%f B:%f t1:%.1f t2:%.1f)\n", expected_fps, pixels, state->fbwidth, state->fbheight, A, B, t1, t2);
#endif

  std::string fsSource = createShader(g_presets[preset].file);
  loadPreset(preset, vsSource, fsSource);
}

bool CVisualizationShadertoy::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, std::string szSongName)
{
  cout << "Start " << iChannels << " " << iSamplesPerSec << " " << iBitsPerSample << " " << szSongName << std::endl;
  samplesPerSec = iSamplesPerSec;
  return true;
}

void CVisualizationShadertoy::Stop()
{
  cout << "Stop" << std::endl;
}

void Mix(float *destination, const float *source, size_t frames, size_t channels)
{
  size_t length = frames * channels;
  for (unsigned int i = 0; i < length; i += channels) {
    float v = 0.0f;
    for (size_t j = 0; j < channels; j++) {
       v += source[i + j];
    }

    destination[(i / 2)] = v / (float)channels;
  }
}

void WriteToBuffer(const float *input, size_t length, size_t channels)
{
  size_t frames = length / channels;

  if (frames >= AUDIO_BUFFER) {
    size_t offset = frames - AUDIO_BUFFER;

    Mix(pcm, input + offset, AUDIO_BUFFER, channels);
  } else {
    size_t keep = AUDIO_BUFFER - frames;
    memmove(pcm, pcm + frames, keep * sizeof(float));

    Mix(pcm + keep, input, frames, channels);
  }
}

void CVisualizationShadertoy::AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
  //cout << "AudioData" << std::endl;
  WriteToBuffer(pAudioData, iAudioDataLength, 2);

  kiss_fft_cpx in[AUDIO_BUFFER], out[AUDIO_BUFFER];
  for (unsigned int i = 0; i < AUDIO_BUFFER; i++) {
    in[i].r = blackmanWindow(pcm[i], i, AUDIO_BUFFER);
    in[i].i = 0;
  }

  kiss_fft(cfg, in, out);

  out[0].i = 0;

  smoothingOverTime(magnitude_buffer, magnitude_buffer, out, NUM_BANDS, SMOOTHING_TIME_CONSTANT, AUDIO_BUFFER);

  const double rangeScaleFactor = MAX_DECIBELS == MIN_DECIBELS ? 1 : (1.0 / (MAX_DECIBELS - MIN_DECIBELS));
  for (unsigned int i = 0; i < NUM_BANDS; i++) {
    float linearValue = magnitude_buffer[i];
    double dbMag = !linearValue ? MIN_DECIBELS : linearToDecibels(linearValue);
    double scaledValue = UCHAR_MAX * (dbMag - MIN_DECIBELS) * rangeScaleFactor;

    audio_data[i] = std::max(std::min((int)scaledValue, UCHAR_MAX), 0);
  }

  for (unsigned int i = 0; i < NUM_BANDS; i++) {
    float v = (pcm[i] + 1.0f) * 128.0f;
    audio_data[i + NUM_BANDS] = std::max(std::min((int)v, UCHAR_MAX), 0);
  }

  needsUpload = true;
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
bool CVisualizationShadertoy::NextPreset()
{
  LogAction("VIS_ACTION_NEXT_PRESET");
  g_currentPreset = (g_currentPreset + 1) % g_presets.size();
  launch(g_currentPreset);
  kodi::SetSettingInt("lastpresetidx", g_currentPreset);
  return true;
}

bool CVisualizationShadertoy::PrevPreset()
{
  LogAction("VIS_ACTION_PREV_PRESET");
  g_currentPreset = (g_currentPreset - 1) % g_presets.size();
  launch(g_currentPreset);
  kodi::SetSettingInt("lastpresetidx", g_currentPreset);
  return true;
}

bool CVisualizationShadertoy::LoadPreset(int select)
{
  LogAction("VIS_ACTION_LOAD_PRESET");
  g_currentPreset = select % g_presets.size();
  launch(g_currentPreset);
  kodi::SetSettingInt("lastpresetidx", g_currentPreset);
  return true;
}

bool CVisualizationShadertoy::RandomPreset()
{
  LogAction("VIS_ACTION_RANDOM_PRESET");
  g_currentPreset = (int)((std::rand() / (float)RAND_MAX) * g_presets.size());
  launch(g_currentPreset);
  kodi::SetSettingInt("lastpresetidx", g_currentPreset);
  return true;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
bool CVisualizationShadertoy::GetPresets(std::vector<std::string>& presets)
{
  for (auto preset : g_presets)
    presets.push_back(preset.name);
  return true;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
int CVisualizationShadertoy::GetActivePreset()
{
  return g_currentPreset;
}

// chunk_to_int() returns the integer from the C string from the token tok in json string str
// Used in ADDON_Create when parsing json file
int chunk_to_int(const char *str, jsmntok_t *tok)
{
  char *ss = (char *)malloc( sizeof(char) * ( tok->end - tok->start + 1 ) );
  int i, result;
  for( i = tok->start; i < tok->end; i++ ) {
    ss[ i - tok->start ] = str[i];
  }
  ss[ i - tok->start ] = '\0';

  // if ss is not a valid integer string, result = 0
  result = (int)strtol( ss, NULL, 0 );
  free( ss );

  return result;
}


//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS CVisualizationShadertoy::Create()
{
  cout << "ADDON_Create" << std::endl;

  g_pathPresets = Presets();
  width = Width();
  height = Height();

  audio_data = new GLubyte[AUDIO_BUFFER]();
  magnitude_buffer = new float[NUM_BANDS]();
  pcm = new float[AUDIO_BUFFER]();

  cfg = kiss_fft_alloc(AUDIO_BUFFER, 0, NULL, NULL);

  if (!initialized)
  {

    g_currentPreset = kodi::GetSettingInt("lastpresetidx");

    int i, j, nPreset;
    jsmn_parser parser;
    int nTokens;
    int bError = true;

    // .........................................
    // load presets list from external json file
    // .........................................
    std::ostringstream ss;
    ss << g_pathPresets << "/resources/" << presetsFile;
    std::string fullPath = ss.str();

    cout << "Loading shader list from " << fullPath << endl;

    std::ifstream t(fullPath);
    std::string JSON_STRING((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

    jsmn_init(&parser);
    nTokens = jsmn_parse(&parser, JSON_STRING.c_str(), JSON_STRING.length(), NULL, 1);

    if ( nTokens > 9 ) { // at least one record (6+1A+1A+1S+1O) must be present
      jsmntok_t tokens[ nTokens ];
      jsmn_init(&parser);
      nTokens = jsmn_parse(&parser, JSON_STRING.c_str(), JSON_STRING.length(), tokens, nTokens);
      if ( nTokens > 0 && tokens[0].type == JSMN_OBJECT ) { // '{'
        if ( tokens[2].type == JSMN_ARRAY ) { // ':[ [],[],... ]'
          g_presets.resize( tokens[2].size ); // number of presets
          nPreset = 0;
          for ( i = 3; i < nTokens; i++ ) {
            if ( tokens[i].type == JSMN_ARRAY && // [description, file, iChannel0..4]
                 tokens[i].size >= 6 ) { // elements over 6 will be ignored, but don't stop the loading
              try {

                // check that data is as expected
                for ( j=i+1; j<=i+6; j++) {
                  // length of each field between 1 and 41 chars (Unicode will be shorter...)
                  if ( ( tokens[j].end - tokens[j].start < 1  ) ||
                       ( tokens[j].end - tokens[j].start > 41 ) )
                    throw 1;
                  // fields 3 to 6 are ~ integer numbers (if they're not, their value will be 0)
                  char testNum = JSON_STRING.c_str()[ tokens[j].start ];
                  if ( j>i+2 && ( ( testNum<48 && testNum!= 45 ) || testNum>57 ) )
                    throw 2;
                }

                g_presets[nPreset].name = std::string( JSON_STRING, (size_t)tokens[i+1].start, (size_t)(tokens[i+1].end-tokens[i+1].start) );
                // presets with description starting with "TEST:" are Testing Presets:
                if ( strcmp( g_presets[nPreset].name.substr(0,5).c_str(), "TEST:" ) == 0 )
                  testingPresets++;
                g_presets[nPreset].file = std::string( JSON_STRING, (size_t)tokens[i+2].start, (size_t)(tokens[i+2].end-tokens[i+2].start) );
                g_presets[nPreset].channel[0] = chunk_to_int( JSON_STRING.c_str(), &tokens[i+3] );
                g_presets[nPreset].channel[1] = chunk_to_int( JSON_STRING.c_str(), &tokens[i+4] );
                g_presets[nPreset].channel[2] = chunk_to_int( JSON_STRING.c_str(), &tokens[i+5] );
                g_presets[nPreset].channel[3] = chunk_to_int( JSON_STRING.c_str(), &tokens[i+6] );

              } catch (...) {
                cerr << "Incorrect data in json file while reading shader #" << (nPreset+1) << endl;
                break;
              }

              nPreset++;
              i += tokens[i].size;
            } else {
              cerr << "JSON: incorrect shader #" << (nPreset+1) << endl;
              break;
            }
          }
          bError = false;
        }
      }
    }

    if ( bError ) {
      cerr << "Failed to load shader list from " << fullPath << endl;
      return ADDON_STATUS_UNKNOWN;
    }
    // .........................................

#if defined(HAS_GLES)
    static const GLfloat vertex_data[] = {
        -1.0,1.0,1.0,1.0,
        1.0,1.0,1.0,1.0,
        1.0,-1.0,1.0,1.0,
        -1.0,-1.0,1.0,1.0,
    };
    glGetError();
    // Upload vertex data to a buffer
    glGenBuffers(1, &state->vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, state->vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);
#endif
    launch(g_currentPreset);

    initialized = true;

  }

  return ADDON_STATUS_OK;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
CVisualizationShadertoy::~CVisualizationShadertoy()
{
  cout << "ADDON_Destroy" << std::endl;

  unloadPreset();
  unloadTextures();

  if (audio_data) {
    delete [] audio_data;
    audio_data = NULL;
  }

  if (magnitude_buffer) {
    delete [] magnitude_buffer;
    magnitude_buffer = NULL;
  }

  if (pcm) {
    delete [] pcm;
    pcm = NULL;
  }

  if (cfg) {
    free(cfg);
    cfg = 0;
  }
#if defined(HAS_GLES)
  glDeleteBuffers(1, &state->vertex_buffer);
#endif

  initialized = false;
}

ADDONCREATOR(CVisualizationShadertoy) // Don't touch this!
