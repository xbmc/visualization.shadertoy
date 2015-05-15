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

#pragma comment(lib, "d3dcompiler.lib")

#include "kodi/xbmc_vis_dll.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
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
#include <vector>

#include "kiss_fft.h"
#include "WICTextureLoader.h"

#define SAFE_RELEASE(p) do { if(p) { (p)->Release(); (p)=nullptr; } } while (0)
#define ALIGN(x, a) (((x)+(a)-1)&~((a)-1))
#define SHADER_SOURCE(...) #__VA_ARGS__
#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif
#define SMOOTHING_TIME_CONSTANT (0.8)
#define MIN_DECIBELS (-100.0)
#define MAX_DECIBELS (-30.0)

#define AUDIO_BUFFER (1024)
#define NUM_BANDS (AUDIO_BUFFER / 2)

using namespace DirectX;

std::string g_pathPresets;

typedef enum GLenum {
  VERTEX_SHADER,
  PIXEL_SHADER
};

struct Preset {
  std::string name;
  std::string file;
  int channel1;
  int channel2;
};

struct MYVERTEX {
  float x, y, z;
  float u, v;
};

const std::vector<Preset> g_presets =
{
   {"Audio Reaktive by choard1895",             "audioreaktive.frag.glsl",          -1, -1},
   {"AudioVisual by Passion",                   "audiovisual.frag.glsl",            -1, -1},
   {"Beating Circles by Phoenix72",             "beatingcircles.frag.glsl",         -1, -1},
   {"BPM by iq",                                "bpm.frag.glsl",                    -1, -1},
   {"Polar Beats by sauj123",                   "polarbeats.frag.glsl",             -1, -1},
   {"Sound Flower by iq",                       "soundflower.frag.glsl",            -1, -1},
   {"Sound sinus wave by Eitraz",               "soundsinuswave.frag.glsl",         -1, -1},
   {"symmetrical sound visualiser by thelinked","symmetricalsound.frag.glsl",       -1, -1},
   {"Twisted Rings by poljere",                 "twistedrings.frag.glsl",           -1, -1},
   {"Undulant Spectre by mafik",                "undulantspectre.frag.glsl",        -1, -1},
   {"Waves Remix by ADOB",                      "wavesremix.frag.glsl",             -1, -1},
};

int g_numberPresets = g_presets.size();
int g_currentPreset = 0;
char** lpresets = nullptr;

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
ID3D11Texture2D*     g_textures[17] = { };
ID3D11Device*        g_pDevice = nullptr;
ID3D11DeviceContext* g_pContext = nullptr;
ID3D11InputLayout*   g_pInputLayout = nullptr;
ID3D11Buffer*        g_pCBParams = nullptr;
ID3D11Buffer*        g_pVBuffer = nullptr;
ID3D11VertexShader*  g_pVShader = nullptr;

void LogProps(VIS_PROPS *props) {
  std::cout << "Props = {" << std::endl
    << "\t x: " << props->x << std::endl
    << "\t y: " << props->y << std::endl
    << "\t width: " << props->width << std::endl
    << "\t height: " << props->height << std::endl
    << "\t pixelRatio: " << props->pixelRatio << std::endl
    << "\t name: " << props->name << std::endl
    << "\t presets: " << props->presets << std::endl
    << "\t profile: " << props->profile << std::endl
//       << "\t submodule: " << props->submodule << endl // Causes problems? Is it initialized?
    << "}" << std::endl;
}

void LogTrack(VisTrack *track) {
  std::cout << "Track = {" << std::endl
    << "\t title: " << track->title << std::endl
    << "\t artist: " << track->artist << std::endl
    << "\t album: " << track->album << std::endl
    << "\t albumArtist: " << track->albumArtist << std::endl
    << "\t genre: " << track->genre << std::endl
    << "\t comment: " << track->comment << std::endl
    << "\t lyrics: " << track->lyrics << std::endl
    << "\t trackNumber: " << track->trackNumber << std::endl
    << "\t discNumber: " << track->discNumber << std::endl
    << "\t duration: " << track->duration << std::endl
    << "\t year: " << track->year << std::endl
    << "\t rating: " << track->rating << std::endl
    << "}" << std::endl;
}

void LogAction(const char *message) {
  std::cout << "Action " << message << std::endl;
}

void LogActionString(const char *message, const char *param) {
  std::cout << "Action " << message << " " << param << std::endl;
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

HRESULT createTexture(DXGI_FORMAT format, unsigned int w, unsigned int h, const void * data, size_t size, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppView = nullptr) 
{
  ID3D11Texture2D* texture = nullptr;
  CD3D11_TEXTURE2D_DESC texDesc(format, w, h, 1, 1, D3D11_BIND_SHADER_RESOURCE, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
  D3D11_SUBRESOURCE_DATA srData = { data, size };

  HRESULT hr = g_pDevice->CreateTexture2D(&texDesc, data ? &srData : nullptr, &texture);
  if (SUCCEEDED(hr) && texture != nullptr)
  {
    if (ppView != nullptr)
    {
      CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(texture, D3D11_SRV_DIMENSION_TEXTURE2D);
      hr = g_pDevice->CreateShaderResourceView(texture, &srvDesc, ppView);
      if (FAILED(hr))
      {
        texture->Release();
        return hr;
      }
    }
    if (ppTexture != nullptr)
      *ppTexture = texture;
  }

  return hr;
}

HRESULT createTexture(const char *file, DXGI_FORMAT internalFormat, D3D11_FILTER scaling, D3D11_TEXTURE_ADDRESS_MODE address,
                      ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppView = nullptr, ID3D11SamplerState** ppSState = nullptr) 
{
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

  std::cout << "creating texture " << fullPath << std::endl;

  const size_t cSize = strlen(file) + 1;
  std::wstring wfile(cSize, L'#');
  mbstowcs(&wfile[0], file, cSize);

  HRESULT hr = CreateWICTextureFromFile(g_pDevice, wfile.c_str(), reinterpret_cast<ID3D11Resource**>(ppTexture), ppView);
  if (FAILED(hr))
    return hr;

  if (ppSState != nullptr)
  {
    D3D11_SAMPLER_DESC sampDesc;
    ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
    sampDesc.Filter = scaling;
    sampDesc.AddressU = address;
    sampDesc.AddressV = address;
    sampDesc.AddressW = address;
    sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sampDesc.MinLOD = 0;
    sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
    hr = g_pDevice->CreateSamplerState(&sampDesc, ppSState);
  }

  if (FAILED(hr))
  {
    if (ppTexture != nullptr)
      SAFE_RELEASE(*ppTexture);
    if (ppView != nullptr)
      SAFE_RELEASE(*ppView);

    return hr;
  }

  return hr;
}

HRESULT compileShader(GLenum shaderType, const char *shader, size_t srcSize, ID3DBlob** ppBlob)
{
  ID3DBlob *pErrors = nullptr;
  HRESULT hr;

  hr = D3DCompile(shader, srcSize, nullptr, nullptr, nullptr, "main",
                  shaderType == PIXEL_SHADER ? "ps_4_0_level_9_3" : "vs_4_0_level_9_1", 
                  0, 0, ppBlob, &pErrors);

  if (FAILED(hr)) {
    SAFE_RELEASE(pErrors);
    return hr;
  }

  SAFE_RELEASE(pErrors);
  return S_OK;
}

const std::string vsSource = SHADER_SOURCE(
  void main(in  float4 pos : POSITION, in float2      uv : TEXCOORD0,
            out float2 outUV : TEXCOORD0, out float4 outPos : SV_POSITION)
  {
    outPos = pos;
    outUV = uv;
  }
);

std::string fsHeader = SHADER_SOURCE(
#define vec2 float2\n
#define vec3 float3\n
#define vec4 float4\n
#define mat2 float2x2\n
#define mat3 float3x3\n
#define mat4 float4x4\n
#define fract frac\n
  float atan(float y, float x)
{
  if (x == 0 && y == 0) x = 1;   // Avoid producing a NaN
  return atan2(y, x);
}
float mod(float x, float y)
{
  return x - y * floor(x / y);
}
float2 mod(float2 x, float2 y)
{
  return x - y * floor(x / y);
}
float2 mod(float2 x, float y)
{
  return x - y * floor(x / y);
}
float3 mod(float3 x, float3 y)
{
  return x - y * floor(x / y);
}
float3 mod(float3 x, float y)
{
  return x - y * floor(x / y);
}
float4 mod(float4 x, float4 y)
{
  return x - y * floor(x / y);
}
float4 mod(float4 x, float y)
{
  return x - y * floor(x / y);
}
float3 mix(float3 x, float3 y, float a)
{
  return x*(1 - a) + y*a;
}
float mix(float x, float y, float a)
{
  return x*(1 - a) + y*a;
}
float mix(float x, float y, bool a)
{
  return a ? y : x;
}
float2 mix(float2 x, float2 y, bool2 a)
{
  return a ? y : x;
}
float3 mix(float3 x, float3 y, bool3 a)
{
  return a ? y : x;
}
float4 mix(float4 x, float4 y, bool4 a)
{
  return a ? y : x;
}
cbuffer cbParams : register(b0)
{
  float3    iResolution;
  float     iGlobalTime;
  float     iChannelTime[4];
  float4    iMouse;
  float4    iDate;
  float     iSampleRate;
  float3    iChannelResolution[4];
};
Texture2D iChannel0 : register(t0);
Texture2D iChannel1 : register(t1);
Texture2D iChannel2 : register(t2);
Texture2D iChannel3 : register(t3);
SamplerState iChannel0Samp : register(s0);
SamplerState iChannel1Samp : register(s1);
SamplerState iChannel2Samp : register(s2);
SamplerState iChannel3Samp : register(s3);
\n#define texture2D(x, uv) (##x##.Sample(##x##Samp, uv))\n
);

std::string fsFooter = SHADER_SOURCE(
  void main(in float2 uv : TEXCOORD0, out float4 color : SV_TARGET)
{
  float2 fragCoord = uv;
  fragCoord.x *= (iResolution.x - 0.5);
  fragCoord.y *= (iResolution.y - 0.5);
  mainImage(color, fragCoord);
  //color.w = 1.0;\n"
}
);


bool initialized = false;

ID3D11PixelShader* shader = nullptr;

struct Params {
  XMFLOAT3 iResolution;
  float    iGlobalTime;
  XMFLOAT4 iChannelTime;
  XMFLOAT4 iMouse;
  XMFLOAT4 iDate;
  float    iSampleRate;
  XMFLOAT3 iChannelResolution[4];
};

ID3D11Texture2D*          iChannel[4]        = { nullptr, nullptr, nullptr, nullptr };
ID3D11ShaderResourceView* iChannelView[4]    = { nullptr, nullptr, nullptr, nullptr };
ID3D11SamplerState*       iChannelSampler[4] = { nullptr, nullptr, nullptr, nullptr };
Params                    cbParams;

bool needsUpload = true;

kiss_fft_cfg cfg;

float *pcm = NULL;
float *magnitude_buffer = NULL;
byte *audio_data = NULL;
int samplesPerSec = 0;
int width = 0;
int height = 0;

HRESULT dxInit(ID3D11DeviceContext* pContex)
{
  HRESULT hr = S_OK;
  g_pContext = pContex;
  g_pContext->GetDevice(&g_pDevice);

  ID3DBlob* vs = nullptr;
  if (SUCCEEDED(compileShader(VERTEX_SHADER, vsSource.c_str(), vsSource.size(), &vs)))
  {
    hr = g_pDevice->CreateVertexShader(vs->GetBufferPointer(), vs->GetBufferSize(), nullptr, &g_pVShader);
    if (SUCCEEDED(hr))
    {
      D3D11_INPUT_ELEMENT_DESC layout[] =
      {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
      };
      hr = g_pDevice->CreateInputLayout(layout, ARRAYSIZE(layout), vs->GetBufferPointer(), vs->GetBufferSize(), &g_pInputLayout);
    }
    SAFE_RELEASE(vs);
  }

  if (FAILED(hr))
    return hr;

  MYVERTEX verts[] = 
  {
    { -1.0f, -1.0f, 0.0f, 0.0f, 0.0f },
    { -1.0f,  1.0f, 0.0f, 0.0f, 1.0f },
    {  1.0f, -1.0f, 0.0f, 1.0f, 0.0f },
    {  1.0f,  1.0f, 0.0f, 1.0f, 1.0f },
  };
  CD3D11_BUFFER_DESC bDesc(sizeof(MYVERTEX) * 4, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_IMMUTABLE);
  D3D11_SUBRESOURCE_DATA srData = { verts };
  if (FAILED(hr = g_pDevice->CreateBuffer(&bDesc, &srData, &g_pVBuffer)))
    return hr;

  bDesc.ByteWidth = ALIGN(sizeof(Params), 16);
  bDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
  bDesc.Usage     = D3D11_USAGE_DYNAMIC;
  bDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
  srData.pSysMem = &cbParams;
  if (FAILED(hr = g_pDevice->CreateBuffer(&bDesc, &srData, &g_pCBParams)))
    return hr;

  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_LINEAR_MIP_POINT;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
  g_pDevice->CreateSamplerState(&sampDesc, &iChannelSampler[0]);

  return hr;
}


void unloadPreset() 
{
  SAFE_RELEASE(shader);

  if (iChannel[1]) {
    std::cout << "Unloading iChannel1 " << iChannel[1] << std::endl;
    SAFE_RELEASE(iChannelSampler[1]);
    SAFE_RELEASE(iChannelView[1]);
    SAFE_RELEASE(iChannel[1]);
  }

  if (iChannel[2]) {
    std::cout << "Unloading iChannel2 " << iChannel[2] << std::endl;
    SAFE_RELEASE(iChannelSampler[2]);
    SAFE_RELEASE(iChannelView[2]);
    SAFE_RELEASE(iChannel[2]);
  }

  if (iChannel[3]) {
    std::cout << "Unloading iChannel3 " << iChannel[3] << std::endl;
    SAFE_RELEASE(iChannelSampler[3]);
    SAFE_RELEASE(iChannelView[3]);
    SAFE_RELEASE(iChannel[3]);
  }
}

HRESULT createShader(const std::string &file, ID3D11PixelShader** ppPShader)
{
  HRESULT hr;
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

  std::cout << "Creating shader from " << fullPath << std::endl;

  FILE* f;
  f = fopen(fullPath.c_str(), "r");
  if (f != NULL)
  {
    std::string source;
    fseek(f, 0, SEEK_END);
    source.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    size_t read = fread(&source[0], 1, source.size(), f);
    fclose(f);

    std::ostringstream ss;
    ss << fsHeader << "\n" << source << "\n" << fsFooter;
    std::string fsSource = ss.str();

    ID3DBlob* pPSBlob = nullptr;
    hr = compileShader(PIXEL_SHADER, fsSource.c_str(), fsSource.size(), &pPSBlob);
    if (SUCCEEDED(hr) && pPSBlob != nullptr)
      hr = g_pDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, ppPShader);

    SAFE_RELEASE(pPSBlob);
  }
  else
    return E_FAIL;

  return hr;
}

HRESULT loadTexture(int number, ID3D11Texture2D** ppTexture, ID3D11ShaderResourceView** ppView = nullptr, ID3D11SamplerState** ppSState = nullptr)
{
  if (number >= 0 && number < g_numberTextures) {
    D3D11_FILTER scaling = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    D3D11_TEXTURE_ADDRESS_MODE address = D3D11_TEXTURE_ADDRESS_CLAMP;

    if (number == 15 || number == 16) {
      scaling = D3D11_FILTER_MIN_MAG_MIP_POINT;
    }

    if (number == 16) {
      address = D3D11_TEXTURE_ADDRESS_MIRROR;
    }

    return createTexture(g_fileTextures[number], DXGI_FORMAT_B8G8R8A8_UNORM, scaling, address, ppTexture, ppView, ppSState);
  }

  return E_FAIL;
}

void loadPreset(int number)
{
  if (number >= 0 && number < g_numberPresets)
  {
    g_currentPreset = number;

    unloadPreset();
    if (FAILED(createShader(g_presets[g_currentPreset].file, &shader)))
    {
      // TODO fallback to default
    }

    if (g_presets[g_currentPreset].channel1 >= 0) {
      loadTexture(g_presets[g_currentPreset].channel1, &iChannel[1], &iChannelView[1], &iChannelSampler[1]);
    }

    if (g_presets[g_currentPreset].channel2 >= 0) {
      loadTexture(g_presets[g_currentPreset].channel1, &iChannel[2], &iChannelView[2], &iChannelSampler[2]);
    }
  }
}

//-- Render -------------------------------------------------------------------
// Called once per frame. Do all rendering here.
//-----------------------------------------------------------------------------
extern "C" void Render()
{
  if (initialized) 
  {
    g_pContext->PSSetShaderResources(0, ARRAYSIZE(iChannelView), iChannelView);
    if (needsUpload) {
      D3D11_MAPPED_SUBRESOURCE res = {};
      if (SUCCEEDED(g_pContext->Map(iChannel[0], 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
      {
        memcpy(res.pData, audio_data, AUDIO_BUFFER);
        g_pContext->Unmap(iChannel[0], 0);
      }
    }

    g_pContext->VSSetShader(g_pVShader, nullptr, 0);
    g_pContext->PSSetShader(shader, nullptr, 0);

    float t = (float)PLATFORM::GetTimeMs() / 1000.0f;
    time_t now = time(NULL);
    tm *ltm = localtime(&now);
    float year = 1900 + ltm->tm_year;
    float month = ltm->tm_mon;
    float day = ltm->tm_mday;
    float sec = (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60) + ltm->tm_sec;

    cbParams.iResolution  = XMFLOAT3( (float)width, (float)height, 0.0f );
    cbParams.iGlobalTime  = t;
    cbParams.iSampleRate  = samplesPerSec;
    cbParams.iChannelTime = XMFLOAT4(t, t, t, t);
    cbParams.iDate        = XMFLOAT4(year, month, day, sec);

    D3D11_MAPPED_SUBRESOURCE res = {};
    if (SUCCEEDED(g_pContext->Map(g_pCBParams, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
    {
      memcpy(res.pData, &cbParams, sizeof(Params));
      g_pContext->Unmap(g_pCBParams, 0);
    }
    g_pContext->PSSetConstantBuffers(0, 1, &g_pCBParams);
    g_pContext->PSSetSamplers(0, ARRAYSIZE(iChannelSampler), iChannelSampler);

    g_pContext->IASetInputLayout(g_pInputLayout);
    g_pContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    size_t strides = sizeof(MYVERTEX), offsets = 0;
    g_pContext->IASetVertexBuffers(0, 1, &g_pVBuffer, &strides, &offsets);

    g_pContext->Draw(4, 0);

    g_pContext->PSSetShader(nullptr, nullptr, 0);
    g_pContext->VSSetShader(nullptr, nullptr, 0);
  }
}

extern "C" void Start(int iChannels, int iSamplesPerSec, int iBitsPerSample, const char* szSongName)
{
  std::cout << "Start " << iChannels << " " << iSamplesPerSec << " " << iBitsPerSample << " " << szSongName << std::endl;
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

    audio_data[i] = (byte) std::max(std::min((int)scaledValue, UCHAR_MAX), 0);
  }

  for (unsigned int i = 0; i < NUM_BANDS; i++) {
    float v = (pcm[i] + 1.0f) * 128.0f;
    audio_data[i + NUM_BANDS] = (byte) std::max(std::min((int)v, UCHAR_MAX), 0);
  }

  needsUpload = true;
}

//-- GetInfo ------------------------------------------------------------------
// Tell XBMC our requirements
//-----------------------------------------------------------------------------
extern "C" void GetInfo(VIS_INFO *pInfo)
{
  std::cout << "GetInfo" << std::endl;
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}


//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
  std::cout << "GetSubModules" << std::endl;
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
  std::cout << "GetPresets " << g_numberPresets << std::endl;

  if (!lpresets) 
  {
    lpresets = new char*[g_presets.size()];
    
    for (size_t i = 0; i < g_presets.size(); ++i)
      lpresets[i] = const_cast<char*>(&(g_presets[i].name)[0]);
  }

  *presets = lpresets;
  return g_presets.size();
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
  std::cout << "IsLocked" << std::endl;
  return false;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  std::cout << "ADDON_Create" << std::endl;
  VIS_PROPS *p = (VIS_PROPS *)props;

  LogProps(p);

  g_pathPresets = p->presets;
  width = p->width;
  height = p->height;

  audio_data = new byte[AUDIO_BUFFER]();
  magnitude_buffer = new float[NUM_BANDS]();
  pcm = new float[AUDIO_BUFFER]();

  cfg = kiss_fft_alloc(AUDIO_BUFFER, 0, NULL, NULL);

  if (S_OK != dxInit(reinterpret_cast<ID3D11DeviceContext*>(p->device))) 
  {
	  std::cout << "Failed to initialize dx";
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  if (!initialized)
  {
    createTexture(DXGI_FORMAT_R8_UNORM, NUM_BANDS, 2, audio_data, sizeof(byte) * AUDIO_BUFFER,
                  &iChannel[0], &iChannelView[0]);
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
  std::cout << "ADDON_Stop" << std::endl;
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  std::cout << "ADDON_Destroy" << std::endl;

  unloadPreset();

  if (lpresets)
    delete[] lpresets, lpresets = nullptr;

  if (iChannel[0]) {
    SAFE_RELEASE(iChannel[0]);
    SAFE_RELEASE(iChannelView[0]);
    SAFE_RELEASE(iChannelSampler[0]);
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

  SAFE_RELEASE(g_pInputLayout);
  SAFE_RELEASE(g_pCBParams);
  SAFE_RELEASE(g_pVBuffer);
  SAFE_RELEASE(g_pVShader);
  SAFE_RELEASE(g_pDevice);

  initialized = false;
}

//-- HasSettings --------------------------------------------------------------
// Returns true if this add-on use settings
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" bool ADDON_HasSettings()
{
  std::cout << "ADDON_HasSettings" << std::endl;
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  std::cout << "ADDON_GetStatus" << std::endl;
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  std::cout << "ADDON_GetSettings" << std::endl;
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

extern "C" void ADDON_FreeSettings()
{
  std::cout << "ADDON_FreeSettings" << std::endl;
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  std::cout << "ADDON_SetSetting " << strSetting << std::endl;
  if (!strSetting || !value)
    return ADDON_STATUS_UNKNOWN;

  // TODO Someone _needs_ to fix this API in kodi, its terrible.
  // a) Why not use GetSettings instead of hacking SetSettings like this?
  // b) Why does it give index and not settings key?
  // c) Seemingly random ###End which if you never write will while(true) the app
  // d) Writes into const setting and value...
  if (strcmp(strSetting, "###GetSavedSettings") == 0)
  {
    std::cout << "WTF...." << std::endl;
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
    std::cout << "lastpresetidx = " << *((int *)value) << std::endl;
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
  std::cout << "ADDON_Announce " << flag << " " << sender << " " << message << std::endl;
}
