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

#include "kodi/xbmc_vis_dll.h"
#include <GL/glew.h>
#include <iostream>
#include <algorithm>
#include <string>
#include <fstream>
#include <streambuf>
#include <ctime>
#include "platform/util/timeutils.h"
#include <math.h>
#include <complex.h>
#include <limits.h>
#include <fstream>
#include <sstream>

#include "kiss_fft.h"
#include "lodepng.h"

using namespace std;

string g_pathPresets;

// TODO Move presets into a struct
const char *g_presets[] = {
  "Audio Reaktive by choard1895",
  "AudioVisual by Passion",
  "Beating Circles by Phoenix72",
  "BPM by iq",
  "Circle Wave by TekF",
  "Circuits by Kali",
  "Colored Bars by novalis",
  "Cubescape by iq",
  "The Disco Tunnel by poljere",
  "Fractal Land by Kali",
  "Gameboy by iq",
  "I/O by movAX13h",
  "Kaleidoscope Visualizer by Qqwy",
  "Nyancat by mu6k",
  "Polar Beats by sauj123",
  "Revision 2015 Livecoding Round 1 by mu6k",
  "Ribbons by XT95",
  "Simplicity Galaxy by CBS",
  "Sound Flower by iq",
  "Sound sinus wave by Eitraz",
  "symmetrical sound visualiser by thelinked",
  "Twisted Rings by poljere",
  "Undulant Spectre by mafik",
  "Demo - Volumetric Lines by iq",
  "Waves Remix by ADOB"
};

const char *g_filePresets[] = {
  "audioreaktive.frag.glsl",
  "audiovisual.frag.glsl",
  "beatingcircles.frag.glsl",
  "bpm.frag.glsl",
  "circlewave.frag.glsl",
  "circuits.frag.glsl",
  "coloredbars.frag.glsl",
  "cubescape.frag.glsl",
  "discotunnel.frag.glsl",
  "fractalland.frag.glsl",
  "gameboy.frag.glsl",
  "io.frag.glsl",
  "kaleidoscopevisualizer.frag.glsl",
  "nyancat.frag.glsl",
  "polarbeats.frag.glsl",
  "revision2015.frag.glsl",
  "ribbons.frag.glsl",
  "simplicitygalaxy.frag.glsl",
  "soundflower.frag.glsl",
  "soundsinuswave.frag.glsl",
  "symmetricalsound.frag.glsl",
  "twistedrings.frag.glsl",
  "undulantspectre.frag.glsl",
  "volumetriclines.frag.glsl",
  "wavesremix.frag.glsl"
};

int g_channel1Presets[] {
  -1,
  -1,
  -1,
  -1,
  -1,
  7,
  -1,
  5,
  2,
  13,
  -1,
  -1,
  15,
  13,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1
};

int g_channel2Presets[] {
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  14 // I think
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1,
  -1
};

int g_numberPresets = 25;
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
  "tex14.png",
  "tex15.png",
  "tex16.png"
};

int g_numberTextures = 17;
GLint g_textures[17] = { };

void LogProps(VIS_PROPS *props) {
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

  glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, w, h, 0, format, GL_UNSIGNED_BYTE, data);
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

      int infologLength = 0;
      char *infoLog;

      glGetShaderiv(program, GL_INFO_LOG_LENGTH, &infologLength);

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

std::string vsSource = "void main() { gl_Position = ftransform(); }";

std::string fsHeader =
"#ifdef GL_ES\n"
"precision highp float;\n"
"#endif\n"
"#extension GL_OES_standard_derivatives : enable\n"
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

GLuint shader = 0;

GLint iResolutionLoc        = 0;
GLint iGlobalTimeLoc        = 0;
GLint iChannelTimeLoc       = 0;
GLint iMouseLoc             = 0;
GLint iDateLoc              = 0;
GLint iSampleRateLoc        = 0;
GLint iChannelResolutionLoc = 0;
GLint iChannel0Loc          = 0;
GLint iChannel1Loc          = 0;
GLint iChannel2Loc          = 0;
GLint iChannel3Loc          = 0;

GLuint iChannel0 = 0;
GLuint iChannel1 = 0;
GLuint iChannel2 = 0;
GLuint iChannel3 = 0;

bool needsUpload = true;

kiss_fft_cfg cfg;

float *pcm = NULL;
float *magnitude_buffer = NULL;
GLubyte *audio_data = NULL;
int samplesPerSec = 0;
int width = 0;
int height = 0;

void unloadPreset() {
  if (shader) {
    glDeleteProgram(shader);
    shader = 0;
  }

  if (iChannel1) {
    cout << "Unloading iChannel1 " << iChannel1 << endl;
    glDeleteTextures(1, &iChannel1);
    iChannel1 = 0;
  }

  if (iChannel2) {
    cout << "Unloading iChannel2 " << iChannel1 << endl;
    glDeleteTextures(1, &iChannel2);
    iChannel2 = 0;
  }

  if (iChannel3) {
    cout << "Unloading iChannel3 " << iChannel1 << endl;
    glDeleteTextures(1, &iChannel3);
    iChannel3 = 0;
  }
}

GLuint createShader(const string &file)
{
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

  cout << "Creating shader from " << fullPath << endl;

  std::ifstream t(fullPath);
  std::string str((std::istreambuf_iterator<char>(t)), std::istreambuf_iterator<char>());

  //std::string str = "void mainImage( out vec4 fragColor, in vec2 fragCoord ) { fragColor = texture2D(iChannel1, fragCoord.xy / iResolution.xy); }";

  std::string fsSource = fsHeader + "\n" + str + "\n" + fsFooter;
  return compileAndLinkProgram(vsSource.c_str(), fsSource.c_str());
}

GLint loadTexture(int number)
{
  if (number >= 0 && number < g_numberTextures) {
    GLint format = number > 10 ? GL_RGBA : GL_RGB;
    GLint scaling = GL_LINEAR;
    GLint repeat = GL_CLAMP_TO_EDGE;

    if (number == 16) {
      format = GL_LUMINANCE;
    }

    if (number == 15 || number == 16) {
      scaling = GL_NEAREST;
    }

    if (number == 16) {
      repeat = GL_REPEAT;
    }

    return createTexture(g_fileTextures[number], format, scaling, repeat);
  }

  return 0;
/*
            var format = gl.RGBA;
            if( url.mSrc=="/presets/tex15.png" || url.mSrc=="/presets/tex17.png" )
                format = gl.LUMINANCE;

            if( url.mSrc=="/presets/tex14.png" )
                createGLTextureNearest( gl, texture.image, texture.globject );
            else if( url.mSrc=="/presets/tex15.png" )
                createGLTextureNearestRepeat( gl, texture.image, texture.globject );
            else
               createGLTexture( gl, texture.image, format, texture.globject );

            texture.loaded = true;
            if( me.mTextureCallbackFun!=null )
                me.mTextureCallbackFun( me.mTextureCallbackObj, slot, texture.image, true, true, 0, -1.0, me.mID );
        }
*/
}

void loadPreset(int number)
{
  if (number >= 0 && number < g_numberPresets)
  {
    g_currentPreset = number;

    unloadPreset();
    shader = createShader(g_filePresets[g_currentPreset]);

    iResolutionLoc        = glGetUniformLocation(shader, "iResolution");
    iGlobalTimeLoc        = glGetUniformLocation(shader, "iGlobalTime");
    iChannelTimeLoc       = glGetUniformLocation(shader, "iChannelTime");
    iMouseLoc             = glGetUniformLocation(shader, "iMouse");
    iDateLoc              = glGetUniformLocation(shader, "iDate");
    iSampleRateLoc        = glGetUniformLocation(shader, "iSampleRate");
    iChannelResolutionLoc = glGetUniformLocation(shader, "iChannelResolution");
    iChannel0Loc          = glGetUniformLocation(shader, "iChannel0");
    iChannel1Loc          = glGetUniformLocation(shader, "iChannel1");
    iChannel2Loc          = glGetUniformLocation(shader, "iChannel2");
    iChannel3Loc          = glGetUniformLocation(shader, "iChannel3");

    if (g_channel1Presets[g_currentPreset] >= 0) {
    	iChannel1 = loadTexture(g_channel1Presets[g_currentPreset]);
    }

    if (g_channel2Presets[g_currentPreset] >= 0) {
    	iChannel2 = loadTexture(g_channel2Presets[g_currentPreset]);
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  if (initialized) {
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

    glBindTexture(GL_TEXTURE_2D, iChannel0);
    if (needsUpload) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, NUM_BANDS, 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, audio_data);
      needsUpload = false;
    }

    float t = (float)PLATFORM::GetTimeMs() / 1000.0f;
    GLfloat tv[] = { t, t, t, t };

    glUseProgram(shader);
    glUniform3f(iResolutionLoc, width, height, 0.0f);
    glUniform1f(iGlobalTimeLoc, t);
    glUniform1f(iSampleRateLoc, samplesPerSec);
    glUniform1fv(iChannelTimeLoc, 4, tv);

    time_t now = time(NULL);
    tm *ltm = localtime(&now);

    float year = 1900 + ltm->tm_year;
    float month = ltm->tm_mon;
    float day = ltm->tm_mday;
    float sec = (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60) + ltm->tm_sec;

    glUniform4f(iDateLoc, year, month, day, sec);

    glActiveTexture(GL_TEXTURE0);
    glEnable(GL_TEXTURE_2D);
    glUniform1i(iChannel0Loc, 0);
    glBindTexture(GL_TEXTURE_2D, iChannel0);

    glActiveTexture(GL_TEXTURE1);
    glEnable(GL_TEXTURE_2D);
    glUniform1i(iChannel1Loc, 1);
    glBindTexture(GL_TEXTURE_2D, iChannel1);

    glActiveTexture(GL_TEXTURE2);
    glEnable(GL_TEXTURE_2D);
    glUniform1i(iChannel2Loc, 2);
    glBindTexture(GL_TEXTURE_2D, iChannel2);

    glActiveTexture(GL_TEXTURE3);
    glEnable(GL_TEXTURE_2D);
    glUniform1i(iChannel3Loc, 3);
    glBindTexture(GL_TEXTURE_2D, iChannel3);

    glBegin(GL_QUADS);
      glVertex3f(-1.0f, 1.0f, 0.0f);
      glVertex3f( 1.0f, 1.0f, 0.0f);
      glVertex3f( 1.0f,-1.0f, 0.0f);
      glVertex3f(-1.0f,-1.0f, 0.0f);
    glEnd();

    glUseProgram(0);

    glPopMatrix();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, 0);

    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, 0);

    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
  }
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  cout << "Start " << iChannels << " " << iSamplesPerSec << " " << iBitsPerSample << " " << szSongName << std::endl;
  samplesPerSec = iSamplesPerSec;
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
    memcpy(pcm, pcm + frames, keep * sizeof(float));

    Mix(pcm + keep, input, frames, channels);
  }
}

extern "C" void AudioData(const float* pAudioData, int iAudioDataLength, float *pFreqData, int iFreqDataLength)
{
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

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO *pInfo)
{
  cout << "GetInfo" << std::endl;
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}


//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  cout << "GetSubModules" << std::endl;
  return 0; // this vis supports 0 sub modules
}

//-- OnAction -----------------------------------------------------------------
// Handle XBMC actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
extern "C" bool OnAction(long flags, const void *param)
{
  switch (flags)
  {
    case VIS_ACTION_NEXT_PRESET:
      LogAction("VIS_ACTION_NEXT_PRESET");
      loadPreset((g_currentPreset + 1)  % g_numberPresets);
      return true;
    case VIS_ACTION_PREV_PRESET:
      LogAction("VIS_ACTION_PREV_PRESET");
      loadPreset((g_currentPreset - 1)  % g_numberPresets);
      return true;
    case VIS_ACTION_LOAD_PRESET:
      LogAction("VIS_ACTION_LOAD_PRESET"); // TODO param is int *
      if (param)
      {
        loadPreset(*(int *)param);
        return true;
      }

      break;
    case VIS_ACTION_RANDOM_PRESET:
      LogAction("VIS_ACTION_RANDOM_PRESET");
      loadPreset((int)((std::rand() / (float)RAND_MAX) * g_numberPresets));
      return true;

    case VIS_ACTION_LOCK_PRESET:
      LogAction("VIS_ACTION_LOCK_PRESET");
      break;
    case VIS_ACTION_RATE_PRESET_PLUS:
      LogAction("VIS_ACTION_RATE_PRESET_PLUS");
      break;
    case VIS_ACTION_RATE_PRESET_MINUS:
      LogAction("VIS_ACTION_RATE_PRESET_MINUS");
      break;
    case VIS_ACTION_UPDATE_ALBUMART:
      LogActionString("VIS_ACTION_UPDATE_ALBUMART", (const char *)param);
      break;
    case VIS_ACTION_UPDATE_TRACK:
      LogTrack((VisTrack *)param);
      break;

    default:
      break;
  }

  return false;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to XBMC for display
//-----------------------------------------------------------------------------
extern "C" unsigned int GetPresets(char ***presets)
{
  cout << "GetPresets " << g_numberPresets << std::endl;

  *presets = const_cast<char**>(g_presets);
  return g_numberPresets;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
extern "C" unsigned GetPreset()
{
  return g_currentPreset;
}

//-- IsLocked -----------------------------------------------------------------
// Returns true if this add-on use settings
//-----------------------------------------------------------------------------
extern "C" bool IsLocked()
{
  cout << "IsLocked" << std::endl;
  return false;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  cout << "ADDON_Create" << std::endl;
  VIS_PROPS *p = (VIS_PROPS *)props;

  LogProps(p);

  g_pathPresets = p->presets;
  width = p->width;
  height = p->height;

  audio_data = new GLubyte[AUDIO_BUFFER]();
  magnitude_buffer = new float[NUM_BANDS]();
  pcm = new float[AUDIO_BUFFER]();

  cfg = kiss_fft_alloc(AUDIO_BUFFER, 0, NULL, NULL);

  if (GLEW_OK != glewInit()) {
	  std::cout << "Failed to initialize glew";
  }

  if (!initialized)
  {
    iChannel0 = createTexture(GL_LUMINANCE, NUM_BANDS, 2, audio_data);
    loadPreset(g_currentPreset);

    initialized = true;
  }

  if (!props)
    return ADDON_STATUS_UNKNOWN;

  return ADDON_STATUS_NEED_SAVEDSETTINGS;
}

//-- Stop ---------------------------------------------------------------------
// This dll must cease all runtime activities
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Stop()
{
  cout << "ADDON_Stop" << std::endl;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  cout << "ADDON_Destroy" << std::endl;

  unloadPreset();

  if (iChannel0) {
    glDeleteTextures(1, &iChannel0);
    iChannel0 = 0;
  }

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

  initialized = false;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  cout << "ADDON_HasSettings" << std::endl;
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  cout << "ADDON_GetStatus" << std::endl;
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  cout << "ADDON_GetSettings" << std::endl;
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

extern "C" void ADDON_FreeSettings()
{
  cout << "ADDON_FreeSettings" << std::endl;
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  cout << "ADDON_SetSetting " << strSetting << std::endl;
  if (!strSetting || !value)
    return ADDON_STATUS_UNKNOWN;

  // TODO Someone _needs_ to fix this API in kodi, its terrible.
  // a) Why not use GetSettings instead of hacking SetSettings like this?
  // b) Why does it give index and not settings key?
  // c) Seemingly random ###End which if you never write will while(true) the app
  // d) Writes into const setting and value...
  if (strcmp(strSetting, "###GetSavedSettings") == 0)
  {
    cout << "WTF...." << endl;
    if (strcmp((char*)value, "0") == 0)
    {
      strcpy((char*)strSetting, "lastpresetidx");
      sprintf ((char*)value, "%i", (int)g_currentPreset);
    }
    if (strcmp((char*)value, "1") == 0)
    {
      strcpy((char*)strSetting, "###End");
    }

    return ADDON_STATUS_OK;
  }

  if (strcmp(strSetting, "lastpresetidx") == 0)
  {
    cout << "lastpresetidx = " << *((int *)value) << endl;
    loadPreset(*(int *)value);
    return ADDON_STATUS_OK;
  }

  return ADDON_STATUS_UNKNOWN;
}

//-- Announce -----------------------------------------------------------------
// Receive announcements from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Announce(const char *flag, const char *sender, const char *message, const void *data)
{
  cout << "ADDON_Announce " << flag << " " << sender << " " << message << std::endl;
}
