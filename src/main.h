/*
 *  Copyright (C) 2005-2020 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <kodi/addon-instance/Visualization.h>
#include <kodi/gui/gl/Shader.h>
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "kissfft/kiss_fft.h"

class ATTRIBUTE_HIDDEN CVisualizationShadertoy
  : public kodi::addon::CAddonBase
  , public kodi::addon::CInstanceVisualization
{
public:
  CVisualizationShadertoy();
  ~CVisualizationShadertoy() override;

  bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) override;
  void Stop() override;
  void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) override;
  void Render() override;
  bool GetPresets(std::vector<std::string>& presets) override;
  int GetActivePreset() override;
  bool PrevPreset() override;
  bool NextPreset() override;
  bool LoadPreset(int select) override;
  bool RandomPreset() override;

private:
  void RenderTo(GLuint shader, GLuint effect_fb);
  void Mix(float* destination, const float* source, size_t frames, size_t channels);
  void WriteToBuffer(const float* input, size_t length, size_t channels);
  void Launch(int preset);
  void LoadPreset(const std::string& shaderPath);
  void UnloadPreset();
  void UnloadTextures();
  GLuint CreateTexture(GLint format, unsigned int w, unsigned int h, const GLvoid* data);
  GLuint CreateTexture(const GLvoid* data, GLint format, unsigned int w, unsigned int h, GLint internalFormat, GLint scaling, GLint repeat);
  GLuint CreateTexture(const std::string& file, GLint internalFormat, GLint scaling, GLint repeat);
  float BlackmanWindow(float in, size_t i, size_t length);
  void SmoothingOverTime(float* outputBuffer, float* lastOutputBuffer, kiss_fft_cpx* inputBuffer, size_t length, float smoothingTimeConstant, unsigned int fftSize);
  float LinearToDecibels(float linear);
  int DetermineBitsPrecision();
  double MeasurePerformance(const std::string& shaderPath, int size);

  kiss_fft_cfg m_kissCfg;
  GLubyte* m_audioData;
  float* m_magnitudeBuffer;
  float* m_pcm;

  bool m_initialized = false;
  int64_t m_initialTime = 0; // in ms
  int m_bitsPrecision = 0;
  int m_currentPreset = 0;

  int m_samplesPerSec = 0; // Given by Start(...)
  bool m_needsUpload = true; // Set by AudioData(...) to mark presence of data

  GLint m_attrResolutionLoc = 0;
  GLint m_attrGlobalTimeLoc = 0;
  GLint m_attrChannelTimeLoc = 0;
  GLint m_attrMouseLoc = 0;
  GLint m_attrDateLoc = 0;
  GLint m_attrSampleRateLoc = 0;
  GLint m_attrChannelResolutionLoc = 0;
  GLint m_attrChannelLoc[4] = {0};
  GLuint m_channelTextures[4] = {0};

  kodi::gui::gl::CShaderProgram m_shadertoyShader;
  kodi::gui::gl::CShaderProgram m_displayShader;

  struct
  {
    GLuint vertex_buffer;
    GLuint attr_vertex_e;
    GLuint attr_vertex_r, uTexture;
    GLuint effect_fb;
    GLuint framebuffer_texture;
    GLuint uScale;
    int fbwidth, fbheight;
  } m_state;

  bool m_settingsUseOwnshader = false;
  std::string m_usedShaderFile;
  struct ShaderPath
  {
    bool audio = false;
    std::string texture;
  } m_shaderTextures[4];
};
