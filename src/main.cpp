/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "main.h"
#include "lodepng.h"

#define _USE_MATH_DEFINES
#include <algorithm>
#include <chrono>
#include <math.h>

#define SMOOTHING_TIME_CONSTANT (0.8)
#define MIN_DECIBELS (-100.0)
#define MAX_DECIBELS (-30.0)

#define AUDIO_BUFFER (1024)
#define NUM_BANDS (AUDIO_BUFFER / 2)

// Override GL_RED if not present with GL_LUMINANCE, e.g. on Android GLES
#ifndef GL_RED
#define GL_RED GL_LUMINANCE
#endif

struct Preset
{
  std::string name;
  std::string file;
  int channel[4];
};

// NOTE: With "#if defined(HAS_GL)" the use of some shaders is avoided
//       as they can cause problems on weaker systems.
const std::vector<Preset> g_presets =
{
   {"2D LED Spectrum by un1versal",             "2Dspectrum.frag.glsl",             99, -1, -1, -1},
   {"Audio Eclipse by airtight",                "audioeclipse.frag.glsl",           99, -1, -1, -1},
   {"Audio Reaktive by choard1895",             "audioreaktive.frag.glsl",          99, -1, -1, -1},
   {"AudioVisual by Passion",                   "audiovisual.frag.glsl",            99, -1, -1, -1},
   {"Beating Circles by Phoenix72",             "beatingcircles.frag.glsl",         99, -1, -1, -1},
   {"BPM by iq",                                "bpm.frag.glsl",                    99, -1, -1, -1},
   {"Circle Wave by TekF",                      "circlewave.frag.glsl",             99, -1, -1, -1},
#if defined(HAS_GL)
   {"Circuits by Kali",                         "circuits.frag.glsl",               99,  7, -1, -1},
   {"Colored Bars by novalis",                  "coloredbars.frag.glsl",            99, -1, -1, -1},
   {"Cubescape by iq",                          "cubescape.frag.glsl",              99,  5, -1, -1},
#endif
   {"Dancing Metalights by Danguafare",         "dancingmetalights.frag.glsl",      99, -1, -1, -1},
   {"The Disco Tunnel by poljere",              "discotunnel.frag.glsl",             2, 13, 99, -1},
   {"Electric pulse by un1versal",              "electricpulse.frag.glsl",          99, -1, -1, -1},
#if defined(HAS_GL)
   {"Fractal Land by Kali",                     "fractalland.frag.glsl",             2, 13, 99, -1},
#endif
   {"Gameboy by iq",                            "gameboy.frag.glsl",                99, -1, -1, -1},
   {"Input Sound by iq",                        "input.frag.glsl",                  99, -1, -1, -1},
#if defined(HAS_GL)
   {"I/O by movAX13h",                          "io.frag.glsl",                     99, -1, -1, -1},
#endif
   {"Kaleidoscope Visualizer by Qqwy",          "kaleidoscopevisualizer.frag.glsl", 99, 15, -1, -1},
   {"LED spectrum by simesgreen",               "ledspectrum.frag.glsl",            99, -1, -1, -1},
   {"Polar Beats by sauj123",                   "polarbeats.frag.glsl",             99, -1, -1, -1},
   {"Simplicity Galaxy by CBS",                 "simplicitygalaxy.frag.glsl",       99, -1, -1, -1},
   {"Sound Flower by iq",                       "soundflower.frag.glsl",            99, -1, -1, -1},
   {"Sound sinus wave by Eitraz",               "soundsinuswave.frag.glsl",         99, -1, -1, -1},
   {"Spectrometer by jaba",                     "spectrometer.frag.glsl",           99, -1, -1, -1},
   {"symmetrical sound visualiser by thelinked","symmetricalsound.frag.glsl",       99, -1, -1, -1},
   {"Twisted Rings by poljere",                 "twistedrings.frag.glsl",           99, -1, -1, -1},
   {"Undulant Spectre by mafik",                "undulantspectre.frag.glsl",        99, -1, -1, -1},
#if defined(HAS_GL)
   {"Demo - Volumetric Lines by iq",            "volumetriclines.frag.glsl",        99, -1, -1, -1},
#endif
   {"Waves Remix by ADOB",                      "wavesremix.frag.glsl",             99, -1, -1, -1}
};

const std::vector<std::string> g_fileTextures =
{
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

#if defined(HAS_GL)

std::string fsHeader =
R"shader(#version 150

#extension GL_OES_standard_derivatives : enable

uniform vec3 iResolution;
uniform float iGlobalTime;
uniform float iChannelTime[4];
uniform vec4 iMouse;
uniform vec4 iDate;
uniform float iSampleRate;
uniform vec3 iChannelResolution[4];
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

out vec4 FragColor;

#define iTime iGlobalTime

#ifndef texture2D
#define texture2D texture
#endif
)shader";

std::string fsFooter =
R"shader(
void main(void)
{
  vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
  mainImage(color, gl_FragCoord.xy);
  color.w = 1.0;
  FragColor = color;
}
)shader";

#else

std::string fsHeader =
R"shader(#version 100

#extension GL_OES_standard_derivatives : enable

precision mediump float;
precision mediump int;

uniform vec3 iResolution;
uniform float iGlobalTime;
uniform float iChannelTime[4];
uniform vec4 iMouse;
uniform vec4 iDate;
uniform float iSampleRate;
uniform vec3 iChannelResolution[4];
uniform sampler2D iChannel0;
uniform sampler2D iChannel1;
uniform sampler2D iChannel2;
uniform sampler2D iChannel3;

#define iTime iGlobalTime
#ifndef texture
#define texture texture2D
#endif

#ifndef textureLod
vec4 textureLod(sampler2D sampler, vec2 uv, float lod)
{
  return texture2D(sampler, uv, lod);
}
#endif
)shader";

std::string fsFooter =
R"shader(
void main(void)
{
  vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
  mainImage(color, gl_FragCoord.xy);
  color.w = 1.0;
  gl_FragColor = color;
}
)shader";

#endif

CVisualizationShadertoy::CVisualizationShadertoy()
  : m_kissCfg(kiss_fft_alloc(AUDIO_BUFFER, 0, nullptr, nullptr)),
    m_audioData(new GLubyte[AUDIO_BUFFER]()),
    m_magnitudeBuffer(new float[NUM_BANDS]()),
    m_pcm(new float[AUDIO_BUFFER]())
{
  m_settingsUseOwnshader = kodi::GetSettingBoolean("ownshader");
  if (m_settingsUseOwnshader)
    m_currentPreset = -1;
  else
    m_currentPreset = kodi::GetSettingInt("lastpresetidx");
}

CVisualizationShadertoy::~CVisualizationShadertoy()
{
  delete [] m_audioData;
  delete [] m_magnitudeBuffer;
  delete [] m_pcm;
  free(m_kissCfg);
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
void CVisualizationShadertoy::Render()
{
  if (m_initialized)
  {
    if (m_state.fbwidth && m_state.fbheight)
    {
      RenderTo(m_shadertoyShader.ProgramHandle(), m_state.effect_fb);
      RenderTo(m_displayShader.ProgramHandle(), 0);
    }
    else
    {
      RenderTo(m_shadertoyShader.ProgramHandle(), 0);
    }
  }
}

bool CVisualizationShadertoy::Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, std::string szSongName)
{
#ifdef DEBUG_PRINT
  printf("Start %i %i %i %s\n", iChannels, iSamplesPerSec, iBitsPerSample, szSongName.c_str());
#endif

  static const GLfloat vertex_data[] =
  {
    -1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0,
     1.0,-1.0, 1.0, 1.0,
    -1.0,-1.0, 1.0, 1.0,
  };

  // Upload vertex data to a buffer
  glGenBuffers(1, &m_state.vertex_buffer);
  glBindBuffer(GL_ARRAY_BUFFER, m_state.vertex_buffer);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STATIC_DRAW);

  m_samplesPerSec = iSamplesPerSec;
  Launch(m_currentPreset);
  m_initialized = true;

  return true;
}

void CVisualizationShadertoy::Stop()
{
  m_initialized = false;
#ifdef DEBUG_PRINT
  printf("Stop\n");
#endif

  UnloadPreset();
  UnloadTextures();

  glDeleteBuffers(1, &m_state.vertex_buffer);
}


void CVisualizationShadertoy::AudioData(const float* pAudioData, int iAudioDataLength, float* pFreqData, int iFreqDataLength)
{
  WriteToBuffer(pAudioData, iAudioDataLength, 2);

  kiss_fft_cpx in[AUDIO_BUFFER], out[AUDIO_BUFFER];
  for (unsigned int i = 0; i < AUDIO_BUFFER; i++)
  {
    in[i].r = BlackmanWindow(m_pcm[i], i, AUDIO_BUFFER);
    in[i].i = 0;
  }

  kiss_fft(m_kissCfg, in, out);

  out[0].i = 0;

  SmoothingOverTime(m_magnitudeBuffer, m_magnitudeBuffer, out, NUM_BANDS, SMOOTHING_TIME_CONSTANT, AUDIO_BUFFER);

  const double rangeScaleFactor = MAX_DECIBELS == MIN_DECIBELS ? 1 : (1.0 / (MAX_DECIBELS - MIN_DECIBELS));
  for (unsigned int i = 0; i < NUM_BANDS; i++)
  {
    float linearValue = m_magnitudeBuffer[i];
    double dbMag = !linearValue ? MIN_DECIBELS : LinearToDecibels(linearValue);
    double scaledValue = UCHAR_MAX * (dbMag - MIN_DECIBELS) * rangeScaleFactor;

    m_audioData[i] = std::max(std::min((int)scaledValue, UCHAR_MAX), 0);
  }

  for (unsigned int i = 0; i < NUM_BANDS; i++)
  {
    float v = (m_pcm[i] + 1.0f) * 128.0f;
    m_audioData[i + NUM_BANDS] = std::max(std::min((int)v, UCHAR_MAX), 0);
  }

  m_needsUpload = true;
}

//-- OnAction -----------------------------------------------------------------
// Handle Kodi actions such as next preset, lock preset, album art changed etc
//-----------------------------------------------------------------------------
bool CVisualizationShadertoy::NextPreset()
{
  if (!m_settingsUseOwnshader)
  {
    m_currentPreset = (m_currentPreset + 1) % g_presets.size();
    Launch(m_currentPreset);
    kodi::SetSettingInt("lastpresetidx", m_currentPreset);
  }
  return true;
}

bool CVisualizationShadertoy::PrevPreset()
{
  if (!m_settingsUseOwnshader)
  {
    m_currentPreset = (m_currentPreset - 1) % g_presets.size();
    Launch(m_currentPreset);
    kodi::SetSettingInt("lastpresetidx", m_currentPreset);
  }
  return true;
}

bool CVisualizationShadertoy::LoadPreset(int select)
{
  if (!m_settingsUseOwnshader)
  {
    m_currentPreset = select % g_presets.size();
    Launch(m_currentPreset);
    kodi::SetSettingInt("lastpresetidx", m_currentPreset);
  }
  return true;
}

bool CVisualizationShadertoy::RandomPreset()
{
  if (!m_settingsUseOwnshader)
  {
    m_currentPreset = (int)((std::rand() / (float)RAND_MAX) * g_presets.size());
    Launch(m_currentPreset);
    kodi::SetSettingInt("lastpresetidx", m_currentPreset);
  }
  return true;
}

//-- GetPresets ---------------------------------------------------------------
// Return a list of presets to Kodi for display
//-----------------------------------------------------------------------------
bool CVisualizationShadertoy::GetPresets(std::vector<std::string>& presets)
{
  if (!m_settingsUseOwnshader)
  {
    for (auto preset : g_presets)
      presets.push_back(preset.name);
  }
  return true;
}

//-- GetPreset ----------------------------------------------------------------
// Return the index of the current playing preset
//-----------------------------------------------------------------------------
int CVisualizationShadertoy::GetActivePreset()
{
  return m_currentPreset;
}

void CVisualizationShadertoy::RenderTo(GLuint shader, GLuint effect_fb)
{
  glUseProgram(shader);

  if (shader == m_shadertoyShader.ProgramHandle())
  {
    GLuint w = Width();
    GLuint h = Height();
    if (m_state.fbwidth && m_state.fbheight)
      w = m_state.fbwidth, h = m_state.fbheight;
    int64_t intt = static_cast<int64_t>(std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() * 1000.0) - m_initialTime;
    if (m_bitsPrecision)
      intt &= (1<<m_bitsPrecision)-1;

    if (m_needsUpload)
    {
      for (int i = 0; i < 4; i++)
      {
        if (m_shaderTextures[i].audio)
        {
          glActiveTexture(GL_TEXTURE0 + i);
          glBindTexture(GL_TEXTURE_2D, m_channelTextures[i]);
          glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, NUM_BANDS, 2, 0, GL_RED, GL_UNSIGNED_BYTE, m_audioData);
        }
      }
      m_needsUpload = false;
    }

    float t = intt / 1000.0f;
    GLfloat tv[] = { t, t, t, t };

    glUniform3f(m_attrResolutionLoc, w, h, 0.0f);
    glUniform1f(m_attrGlobalTimeLoc, t);
    glUniform1f(m_attrSampleRateLoc, m_samplesPerSec);
    glUniform1fv(m_attrChannelTimeLoc, 4, tv);
    glUniform2f(m_state.uScale, static_cast<GLfloat>(Width()) / m_state.fbwidth, static_cast<GLfloat>(Height()) /m_state.fbheight);

    time_t now = time(NULL);
    tm *ltm = localtime(&now);

    float year = 1900 + ltm->tm_year;
    float month = ltm->tm_mon;
    float day = ltm->tm_mday;
    float sec = (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60) + ltm->tm_sec;

    glUniform4f(m_attrDateLoc, year, month, day, sec);

    for (int i = 0; i < 4; i++)
    {
      glActiveTexture(GL_TEXTURE0 + i);
      glUniform1i(m_attrChannelLoc[i], i);
      glBindTexture(GL_TEXTURE_2D, m_channelTextures[i]);
    }
  }
  else
  {
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_state.framebuffer_texture);
    glUniform1i(m_state.uTexture, 0); // first currently bound texture "GL_TEXTURE0"
  }

  // Draw the effect to a texture or direct to framebuffer
  glBindFramebuffer(GL_FRAMEBUFFER, effect_fb);

  GLuint attr_vertex = shader == m_shadertoyShader.ProgramHandle() ? m_state.attr_vertex_e : m_state.attr_vertex_r;
  glBindBuffer(GL_ARRAY_BUFFER, m_state.vertex_buffer);
  glVertexAttribPointer(attr_vertex, 4, GL_FLOAT, 0, 16, 0);
  glEnableVertexAttribArray(attr_vertex);
  glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  glDisableVertexAttribArray(attr_vertex);
  glBindBuffer(GL_ARRAY_BUFFER, 0);

  for (int i = 0; i < 4; i++)
  {
    glActiveTexture(GL_TEXTURE0 + i);
    glBindTexture(GL_TEXTURE_2D, 0);
  }

  glUseProgram(0);
}

void CVisualizationShadertoy::Mix(float* destination, const float* source, size_t frames, size_t channels)
{
  size_t length = frames * channels;
  for (unsigned int i = 0; i < length; i += channels)
  {
    float v = 0.0f;
    for (size_t j = 0; j < channels; j++)
    {
      v += source[i + j];
    }

    destination[(i / 2)] = v / (float)channels;
  }
}

void CVisualizationShadertoy::WriteToBuffer(const float* input, size_t length, size_t channels)
{
  size_t frames = length / channels;

  if (frames >= AUDIO_BUFFER)
  {
    size_t offset = frames - AUDIO_BUFFER;

    Mix(m_pcm, input + offset, AUDIO_BUFFER, channels);
  }
  else
  {
    size_t keep = AUDIO_BUFFER - frames;
    memmove(m_pcm, m_pcm + frames, keep * sizeof(float));

    Mix(m_pcm + keep, input, frames, channels);
  }
}

void CVisualizationShadertoy::Launch(int preset)
{
  m_bitsPrecision = DetermineBitsPrecision();
  // mali-400 has only 10 bits which means milliseond timer wraps after ~1 second.
  // we'll fudge that up a bit as having a larger range is more important than ms accuracy
  m_bitsPrecision = std::max(m_bitsPrecision, 13);
#ifdef DEBUG_PRINT
  printf("bits=%d\n", m_bitsPrecision);
#endif

  UnloadTextures();

  if (preset < 0)
  {
    m_usedShaderFile = kodi::GetSettingString("shader");
    m_shaderTextures[0].audio = kodi::GetSettingBoolean("texture0-sound");
    m_shaderTextures[0].texture = kodi::GetSettingString("texture0");
    m_shaderTextures[1].audio = kodi::GetSettingBoolean("texture1-sound");
    m_shaderTextures[1].texture = kodi::GetSettingString("texture1");
    m_shaderTextures[2].audio = kodi::GetSettingBoolean("texture2-sound");
    m_shaderTextures[2].texture = kodi::GetSettingString("texture2");
    m_shaderTextures[3].audio = kodi::GetSettingBoolean("texture3-sound");
    m_shaderTextures[3].texture = kodi::GetSettingString("texture3");
  }
  else
  {
    m_usedShaderFile = kodi::GetAddonPath("resources/shaders/" + g_presets[preset].file);
    for (int i = 0; i < 4; i++)
    {
      if (g_presets[preset].channel[i] >= 0 && g_presets[preset].channel[i] < g_fileTextures.size())
      {
        m_shaderTextures[i].texture = kodi::GetAddonPath("resources/" + g_fileTextures[g_presets[preset].channel[i]]);
      }
      else if (g_presets[preset].channel[i] == 99) // framebuffer
      {
        m_shaderTextures[i].audio = true;
      }
      else
      {
        m_shaderTextures[i].texture = "";
        m_shaderTextures[i].audio = false;
      }
    }
  }

  for (int i = 0; i < 4; i++)
  {
    if (m_shaderTextures[i].audio)
    {
      m_channelTextures[i] = CreateTexture(GL_RED, NUM_BANDS, 2, m_audioData);
    }
    else if (!m_shaderTextures[i].texture.empty())
    {
      GLint format = GL_RGBA;
      GLint scaling = GL_LINEAR;
      GLint repeat = GL_REPEAT;
      m_channelTextures[i] = CreateTexture(m_shaderTextures[i].texture, format, scaling, repeat);
    }
  }

  const int size1 = 256, size2=512;
  double t1 = MeasurePerformance(m_usedShaderFile, size1);
  double t2 = MeasurePerformance(m_usedShaderFile, size2);

  double expected_fps = 40.0;
  // time per pixel for rendering fragment shader
  double B = (t2-t1)/(size2*size2-size1*size1);
  // time to render to screen
  double A = t2 - size2*size2 * B;
  // how many pixels get the desired fps
  double pixels = (1000.0/expected_fps - A) / B;
  m_state.fbwidth = sqrtf(pixels * Width() / Height());
  if (m_state.fbwidth >= Width())
    m_state.fbwidth = 0;
  else if (m_state.fbwidth < 320)
    m_state.fbwidth = 320;
  m_state.fbheight = m_state.fbwidth * Height() / Width();

#ifdef DEBUG_PRINT
  printf("expected fps=%f, pixels=%f %dx%d (A:%f B:%f t1:%.1f t2:%.1f)\n", expected_fps, pixels, m_state.fbwidth, m_state.fbheight, A, B, t1, t2);
#endif

  LoadPreset(m_usedShaderFile);
}

void CVisualizationShadertoy::UnloadTextures()
{
  for (int i = 0; i < 4; i++)
  {
    if (m_channelTextures[i])
    {
      glDeleteTextures(1, &m_channelTextures[i]);
      m_channelTextures[i] = 0;
    }
  }
}

void CVisualizationShadertoy::LoadPreset(const std::string& shaderPath)
{
  UnloadPreset();
  std::string vertShadertoyShader = kodi::GetAddonPath("resources/shaders/main_shadertoy_" GL_TYPE_STRING ".vert.glsl");
  if (!m_shadertoyShader.LoadShaderFiles(vertShadertoyShader, shaderPath) ||
      !m_shadertoyShader.CompileAndLink("", "", fsHeader, fsFooter))
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to compile shadertoy shaders (current shadertoy file '%s')", shaderPath.c_str());
    return;
  }

  GLuint shadertoyShader = m_shadertoyShader.ProgramHandle();

  m_attrResolutionLoc = glGetUniformLocation(shadertoyShader, "iResolution");
  m_attrGlobalTimeLoc = glGetUniformLocation(shadertoyShader, "iGlobalTime");
  m_attrChannelTimeLoc = glGetUniformLocation(shadertoyShader, "iChannelTime");
  m_attrMouseLoc = glGetUniformLocation(shadertoyShader, "iMouse");
  m_attrDateLoc = glGetUniformLocation(shadertoyShader, "iDate");
  m_attrSampleRateLoc  = glGetUniformLocation(shadertoyShader, "iSampleRate");
  m_attrChannelResolutionLoc = glGetUniformLocation(shadertoyShader, "iChannelResolution");
  m_attrChannelLoc[0] = glGetUniformLocation(shadertoyShader, "iChannel0");
  m_attrChannelLoc[1] = glGetUniformLocation(shadertoyShader, "iChannel1");
  m_attrChannelLoc[2] = glGetUniformLocation(shadertoyShader, "iChannel2");
  m_attrChannelLoc[3] = glGetUniformLocation(shadertoyShader, "iChannel3");

  m_state.uScale = glGetUniformLocation(shadertoyShader, "uScale");
  m_state.attr_vertex_e = glGetAttribLocation(shadertoyShader,  "vertex");

  std::string vertShader = kodi::GetAddonPath("resources/shaders/main_display_" GL_TYPE_STRING ".vert.glsl");
  std::string fraqShader = kodi::GetAddonPath("resources/shaders/main_display_" GL_TYPE_STRING ".frag.glsl");
  if (!m_displayShader.LoadShaderFiles(vertShader, fraqShader) ||
      !m_displayShader.CompileAndLink())
  {
    kodi::Log(ADDON_LOG_ERROR, "Failed to compile main shaders");
    return;
  }

  m_state.uTexture = glGetUniformLocation(m_displayShader.ProgramHandle(), "uTexture");
  m_state.attr_vertex_r = glGetAttribLocation(m_displayShader.ProgramHandle(), "vertex");

  // Prepare a texture to render to
  glActiveTexture(GL_TEXTURE0);
  glGenTextures(1, &m_state.framebuffer_texture);
  glBindTexture(GL_TEXTURE_2D, m_state.framebuffer_texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_state.fbwidth, m_state.fbheight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  // Prepare a framebuffer for rendering
  glGenFramebuffers(1, &m_state.effect_fb);
  glBindFramebuffer(GL_FRAMEBUFFER, m_state.effect_fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_state.framebuffer_texture, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  m_initialTime = static_cast<int64_t>(std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() * 1000.0);
}

void CVisualizationShadertoy::UnloadPreset()
{
  if (m_state.framebuffer_texture)
  {
    glDeleteTextures(1, &m_state.framebuffer_texture);
    m_state.framebuffer_texture = 0;
  }
  if (m_state.effect_fb)
  {
    glDeleteFramebuffers(1, &m_state.effect_fb);
    m_state.effect_fb = 0;
  }
}

GLuint CVisualizationShadertoy::CreateTexture(GLint format, unsigned int w, unsigned int h, const GLvoid* data)
{
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

GLuint CVisualizationShadertoy::CreateTexture(const GLvoid* data, GLint format, unsigned int w, unsigned int h, GLint internalFormat, GLint scaling, GLint repeat)
{
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

GLuint CVisualizationShadertoy::CreateTexture(const std::string& file, GLint internalFormat, GLint scaling, GLint repeat)
{
#ifdef DEBUG_PRINT
  printf("creating texture %s\n", file.c_str());
#endif

  unsigned error;
  unsigned char* image;
  unsigned width, height;

  error = lodepng_decode32_file(&image, &width, &height, file.c_str());
  if (error)
  {
    kodi::Log(ADDON_LOG_ERROR, "lodepng_decode32_file error %u: %s", error, lodepng_error_text(error));
    return 0;
  }

  GLuint texture = CreateTexture(image, GL_RGBA, width, height, internalFormat, scaling, repeat);
  free(image);
  return texture;
}

float CVisualizationShadertoy::BlackmanWindow(float in, size_t i, size_t length)
{
  double alpha = 0.16;
  double a0 = 0.5 * (1.0 - alpha);
  double a1 = 0.5;
  double a2 = 0.5 * alpha;

  float x = (float)i / (float)length;
  return in * (a0 - a1 * cos(2.0 * M_PI * x) + a2 * cos(4.0 * M_PI * x));
}

void CVisualizationShadertoy::SmoothingOverTime(float* outputBuffer, float* lastOutputBuffer, kiss_fft_cpx* inputBuffer, size_t length, float smoothingTimeConstant, unsigned int fftSize)
{
  for (size_t i = 0; i < length; i++)
  {
    kiss_fft_cpx c = inputBuffer[i];
    float magnitude = sqrt(c.r * c.r + c.i * c.i) / (float)fftSize;
    outputBuffer[i] = smoothingTimeConstant * lastOutputBuffer[i] + (1.0 - smoothingTimeConstant) * magnitude;
  }
}

float CVisualizationShadertoy::LinearToDecibels(float linear)
{
  if (!linear)
    return -1000;
  return 20 * log10f(linear);
}

int CVisualizationShadertoy::DetermineBitsPrecision()
{
  m_state.fbwidth = 32, m_state.fbheight = 26*10;
  LoadPreset(kodi::GetAddonPath("resources/shaders/main_test.frag.glsl"));
  RenderTo(m_shadertoyShader.ProgramHandle(), m_state.effect_fb);
  glFinish();

  unsigned char* buffer = new unsigned char[m_state.fbwidth * m_state.fbheight * 4];
  if (buffer)
    glReadPixels(0, 0, m_state.fbwidth, m_state.fbheight, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

  int bits = 0;
  unsigned char b = 0;
  for (int j=0; j<m_state.fbheight; j++)
  {
    unsigned char c = buffer[4*(j*m_state.fbwidth+(m_state.fbwidth>>1))];
    if (c && !b)
      bits++;
    b = c;
  }
  delete buffer;
  UnloadPreset();
  return bits;
}

double CVisualizationShadertoy::MeasurePerformance(const std::string& shaderPath, int size)
{
  int iterations = -1;
  m_state.fbwidth = m_state.fbheight = size;
  LoadPreset(shaderPath);

  int64_t end, start;
  do
  {
    RenderTo(m_shadertoyShader.ProgramHandle(), m_state.effect_fb);
    RenderTo(m_displayShader.ProgramHandle(), m_state.effect_fb);
    glFinish();
    if (++iterations == 0)
      start = static_cast<int64_t>(std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() * 1000.0);
    end = static_cast<int64_t>(std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count() * 1000.0);
  } while (end - start < 50);
  double t = (double)(end - start)/iterations;
#ifdef DEBUG_PRINT
  printf("%s %dx%d %.1fms = %.2f fps\n", __func__, size, size, t, 1000.0/t);
#endif
  UnloadPreset();
  return t;
}

ADDONCREATOR(CVisualizationShadertoy) // Don't touch this!
