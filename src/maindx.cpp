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

#include "xbmc_vis_dll.h"
#include <d3d11.h>
#include <DirectXMath.h>
#include <d3dcompiler.h>
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

typedef enum ShaderType {
  VERTEX_SHADER,
  PIXEL_SHADER
};

struct Preset {
  std::string name;
  std::string file;
  int channel[4];
};

struct MYVERTEX {
  float x, y, z;
  float u, v;
};

struct Params {
  XMFLOAT3 iResolution;
  float    iGlobalTime;
  XMFLOAT4 iChannelTime;
  XMFLOAT4 iMouse;
  XMFLOAT4 iDate;
  float    iSampleRate;
  XMFLOAT3 iChannelResolution[4];
};

const std::vector<Preset> g_presets =
{
   {"2D LED Spectrum by un1versal",             "2Dspectrum.frag.glsl",             99, -1, -1, -1},
   {"Audio Reaktive by choard1895",             "audioreaktive.frag.glsl",          99, -1, -1, -1},
   {"AudioVisual by Passion",                   "audiovisual.frag.glsl",            99, -1, -1, -1},
   {"Beating Circles by Phoenix72",             "beatingcircles.frag.glsl",         99, -1, -1, -1},
   {"BPM by iq",                                "bpm.frag.glsl",                    99, -1, -1, -1},
   {"The Disco Tunnel by poljere",              "discotunnel.frag.glsl",             2, 14, 99, -1},
   {"Polar Beats by sauj123",                   "polarbeats.frag.glsl",             99, -1, -1, -1},
   {"Sound Flower by iq",                       "soundflower.frag.glsl",            99, -1, -1, -1},
   {"Sound sinus wave by Eitraz",               "soundsinuswave.frag.glsl",         99, -1, -1, -1},
   {"symmetrical sound visualiser by thelinked","symmetricalsound.frag.glsl",       99, -1, -1, -1},
   {"Twisted Rings by poljere",                 "twistedrings.frag.glsl",           99, -1, -1, -1},
   {"Undulant Spectre by mafik",                "undulantspectre.frag.glsl",        99, -1, -1, -1},
   {"Waves Remix by ADOB",                      "wavesremix.frag.glsl",             99, -1, -1, -1},
};
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

std::string g_pathPresets;
int g_numberPresets = g_presets.size();
int g_currentPreset = 0;
char** lpresets = nullptr;
int g_numberTextures = ARRAYSIZE(g_fileTextures);
bool initialized = false;
bool needsUpload = true;

// directx related vars
ID3D11Device*             g_pDevice = nullptr;
ID3D11DeviceContext*      g_pContext = nullptr;
ID3D11InputLayout*        g_pInputLayout = nullptr;
ID3D11Buffer*             g_pCBParams = nullptr;
ID3D11Buffer*             g_pVBuffer = nullptr;
ID3D11VertexShader*       g_pVShader = nullptr;
ID3D11PixelShader*        g_pPShader = nullptr;
ID3D11ShaderResourceView* iChannelView[4] = { nullptr, nullptr, nullptr, nullptr };
ID3D11SamplerState*       iChannelSampler[4] = { nullptr, nullptr, nullptr, nullptr };
ID3D11Texture2D*          pAudioTexture = nullptr;
ID3D11ShaderResourceView* pAudioView = nullptr;
ID3D11ShaderResourceView* pNoiseView = nullptr;
ID3D11ShaderResourceView* pBackBuffer = nullptr;
ID3D11SamplerState*       pNoiseSampler = nullptr;
ID3D11SamplerState*       pAudioSampler = nullptr;
Params                    cbParams;

// kiss related vars
kiss_fft_cfg cfg;
float *pcm = NULL;
float *magnitude_buffer = NULL;
byte *audio_data = NULL;
int samplesPerSec = 0;
int width = 0;
int height = 0;

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
}

void LogActionString(const char *message, const char *param) {
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

  const size_t cSize = fullPath.length() + 1;
  std::wstring wfile(cSize, L'#');
  mbstowcs(&wfile[0], fullPath.c_str(), cSize);

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
  }

  return hr;
}

HRESULT compileShader(ShaderType shaderType, const char *shader, size_t srcSize, ID3DBlob** ppBlob)
{
  ID3DBlob *pErrors = nullptr;
  HRESULT hr;

  hr = D3DCompile(shader, srcSize, nullptr, nullptr, nullptr, "main",
                  shaderType == PIXEL_SHADER ? "ps_4_0_level_9_3" : "vs_4_0_level_9_1", 
                  0, 0, ppBlob, &pErrors);

  if (FAILED(hr))
  {
    // TODO log errors
  }

  SAFE_RELEASE(pErrors);
  return hr;
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
  color.w = 1.0;
}
);

float fCubicInterpolate(float y0, float y1, float y2, float y3, float t)
{
  float a0, a1, a2, a3, t2;

  t2 = t*t;
  a0 = y3 - y2 - y0 + y1;
  a1 = y0 - y1 - a0;
  a2 = y2 - y0;
  a3 = y1;

  return(a0*t*t2 + a1*t2 + a2*t + a3);
}

DWORD dwCubicInterpolate(DWORD y0, DWORD y1, DWORD y2, DWORD y3, float t)
{
  // performs cubic interpolation on a D3DCOLOR value.
  DWORD ret = 0;
  DWORD shift = 0;
  for (int i = 0; i<4; i++)
  {
    float f = fCubicInterpolate(
      ((y0 >> shift) & 0xFF) / 255.0f,
      ((y1 >> shift) & 0xFF) / 255.0f,
      ((y2 >> shift) & 0xFF) / 255.0f,
      ((y3 >> shift) & 0xFF) / 255.0f,
      t
      );
    if (f<0)
      f = 0;
    if (f>1)
      f = 1;
    ret |= ((DWORD)(f * 255)) << shift;
    shift += 8;
  }
  return ret;
}

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

  // create audio buffer
  createTexture(DXGI_FORMAT_R8_UNORM, NUM_BANDS, 2, nullptr, 0, &pAudioTexture, &pAudioView);

  D3D11_SAMPLER_DESC sampDesc;
  ZeroMemory(&sampDesc, sizeof(D3D11_SAMPLER_DESC));
  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
  sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
  sampDesc.MinLOD = 0;
  sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
  g_pDevice->CreateSamplerState(&sampDesc, &pAudioSampler);

  sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
  sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
  sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
  g_pDevice->CreateSamplerState(&sampDesc, &pNoiseSampler);

  ID3D11DeviceContext* pCtx = nullptr;
  g_pDevice->GetImmediateContext(&pCtx);

  // create noise buffer
  ID3D11Texture2D* pNoise = nullptr;
  int size = 256, zoom_factor = 1;
  createTexture(DXGI_FORMAT_B8G8R8A8_UNORM, size, size, nullptr, 0, &pNoise, &pNoiseView);

  // generate noise data (from MilkDrop)
  if (pNoise)
  {
    D3D11_MAPPED_SUBRESOURCE r;
    if (FAILED(hr = pCtx->Map(pNoise, 0, D3D11_MAP_WRITE_DISCARD, 0, &r)))
    {
      SAFE_RELEASE(pCtx);
      return hr;
    }

    // write to the bits...
    DWORD* dst = (DWORD*)r.pData;
    int dwords_per_line = r.RowPitch / sizeof(DWORD);
    int RANGE = 256;
    for (int y = 0; y<size; y++) {
      LARGE_INTEGER q;
      QueryPerformanceCounter(&q);
      srand(q.LowPart ^ q.HighPart ^ rand());
      int x;
      for (x = 0; x<size; x++) {
        dst[x] = (((DWORD)(rand() % RANGE) + RANGE / 2) << 24) |
          (((DWORD)(rand() % RANGE) + RANGE / 2) << 16) |
          (((DWORD)(rand() % RANGE) + RANGE / 2) << 8) |
          (((DWORD)(rand() % RANGE) + RANGE / 2));
      }
      // swap some pixels randomly, to improve 'randomness'
      for (x = 0; x<256; x++)
      {
        int x1 = (rand() ^ q.LowPart) % size;
        int x2 = (rand() ^ q.HighPart) % size;
        DWORD temp = dst[x2];
        dst[x2] = dst[x1];
        dst[x1] = temp;
      }
      dst += dwords_per_line;
    }

    // smoothing
    if (zoom_factor > 1)
    {
      // first go ACROSS, blending cubically on X, but only on the main lines.
      DWORD* dst = (DWORD*)r.pData;
      for (int y = 0; y<size; y += zoom_factor)
        for (int x = 0; x<size; x++)
          if (x % zoom_factor)
          {
            int base_x = (x / zoom_factor)*zoom_factor + size;
            int base_y = y*dwords_per_line;
            DWORD y0 = dst[base_y + ((base_x - zoom_factor) % size)];
            DWORD y1 = dst[base_y + ((base_x) % size)];
            DWORD y2 = dst[base_y + ((base_x + zoom_factor) % size)];
            DWORD y3 = dst[base_y + ((base_x + zoom_factor * 2) % size)];

            float t = (x % zoom_factor) / (float)zoom_factor;

            DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

            dst[y*dwords_per_line + x] = result;
          }

      // next go down, doing cubic interp along Y, on every line.
      for (int x = 0; x<size; x++)
        for (int y = 0; y<size; y++)
          if (y % zoom_factor)
          {
            int base_y = (y / zoom_factor)*zoom_factor + size;
            DWORD y0 = dst[((base_y - zoom_factor) % size)*dwords_per_line + x];
            DWORD y1 = dst[((base_y) % size)*dwords_per_line + x];
            DWORD y2 = dst[((base_y + zoom_factor) % size)*dwords_per_line + x];
            DWORD y3 = dst[((base_y + zoom_factor * 2) % size)*dwords_per_line + x];

            float t = (y % zoom_factor) / (float)zoom_factor;

            DWORD result = dwCubicInterpolate(y0, y1, y2, y3, t);

            dst[y*dwords_per_line + x] = result;
          }
    }

    pCtx->Unmap(pNoise, 0);
    pNoise->Release();
  }

  // create back buffer
  ID3D11RenderTargetView* pRTView = nullptr;
  g_pContext->OMGetRenderTargets(1, &pRTView, nullptr);
  if (pRTView != nullptr)
  {
    ID3D11Resource* pResource = nullptr;
    pRTView->GetResource(&pResource);
    pRTView->Release();

    if (pResource == nullptr)
      return E_FAIL;

    ID3D11Texture2D* pRTTexture = nullptr;
    hr = pResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&pRTTexture));
    if (FAILED(hr))
    {
      pResource->Release();
      return hr;
    }

    D3D11_TEXTURE2D_DESC texDesc;
    pRTTexture->GetDesc(&texDesc);
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    pRTTexture->Release();

    ID3D11Texture2D* pCopyTexture = nullptr;
    hr = g_pDevice->CreateTexture2D(&texDesc, nullptr, &pCopyTexture);
    if (FAILED(hr))
    {
      pResource->Release();
      return hr;
    }

    pCtx->CopyResource(pCopyTexture, pResource);
    pResource->Release();

    CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(pCopyTexture, D3D11_SRV_DIMENSION_TEXTURE2D);
    hr = g_pDevice->CreateShaderResourceView(pCopyTexture, &srvDesc, &pBackBuffer);

    pCopyTexture->Release();
  }

  SAFE_RELEASE(pCtx);

  return hr;
}


void unloadPreset() 
{
  SAFE_RELEASE(g_pPShader);
  for (size_t i = 0; i < 4; i++)
  {
    SAFE_RELEASE(iChannelSampler[i]);
    SAFE_RELEASE(iChannelView[i]);
  }
}

HRESULT createShader(const std::string &file, ID3D11PixelShader** ppPShader)
{
  HRESULT hr;
  std::ostringstream ss;
  ss << g_pathPresets << "/resources/" << file;
  std::string fullPath = ss.str();

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
  if (number == 99) // audio
  {
    if (ppView != nullptr)
    {
      *ppView = pAudioView;
      pAudioView->AddRef();
    }
    if (ppSState != nullptr)
    {
      *ppSState = pAudioSampler;
      pAudioSampler->AddRef();
    }
    return S_OK;
  }

  if (number == 98) // noise
  {
    if (ppView != nullptr)
    {
      *ppView = pNoiseView;
      pNoiseView->AddRef();
    }
    if (ppSState != nullptr)
    {
      *ppSState = pNoiseSampler;
      pNoiseSampler->AddRef();
    }
    return S_OK;
  }

  if (number == 97) // back buffer
  {
    if (ppView != nullptr)
    {
      *ppView = pBackBuffer;
      pBackBuffer->AddRef();
    }
    if (ppSState != nullptr)
    {
      *ppSState = pNoiseSampler;
      pNoiseSampler->AddRef();
    }
    return S_OK;
  }

  // texture
  if (number >= 0 && number < g_numberTextures)
  {
    D3D11_FILTER scaling = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    D3D11_TEXTURE_ADDRESS_MODE address = D3D11_TEXTURE_ADDRESS_CLAMP;
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
    if (FAILED(createShader(g_presets[g_currentPreset].file, &g_pPShader)))
    {
      // TODO fallback to default
    }

    for (size_t i = 0; i < 4; i++)
    {
      loadTexture(g_presets[g_currentPreset].channel[i], nullptr, &iChannelView[i], &iChannelSampler[i]);
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
    if (needsUpload) 
    {
      D3D11_MAPPED_SUBRESOURCE res = {};
      if (SUCCEEDED(g_pContext->Map(pAudioTexture, 0, D3D11_MAP_WRITE_DISCARD, 0, &res)))
      {
        memcpy(res.pData, audio_data, AUDIO_BUFFER);
        g_pContext->Unmap(pAudioTexture, 0);
      }
    }

    g_pContext->VSSetShader(g_pVShader, nullptr, 0);
    g_pContext->PSSetShader(g_pPShader, nullptr, 0);
    g_pContext->PSSetShaderResources(0, ARRAYSIZE(iChannelView), iChannelView);

    float t = (float)P8PLATFORM::GetTimeMs() / 1000.0f;
    time_t now = time(NULL);
    tm *ltm = localtime(&now);
    float year = 1900 + ltm->tm_year;
    float month = ltm->tm_mon;
    float day = ltm->tm_mday;
    float sec = (ltm->tm_hour * 60 * 60) + (ltm->tm_min * 60) + ltm->tm_sec;

    cbParams.iResolution  = XMFLOAT3((float)width, (float)height, (float)width / (float)height);
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
  pInfo->bWantsFreq = false;
  pInfo->iSyncDelay = 0;
}


//-- GetSubModules ------------------------------------------------------------
// Return any sub modules supported by this vis
//-----------------------------------------------------------------------------
extern "C" unsigned int GetSubModules(char ***names)
{
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
  return false;
}

//-- Create -------------------------------------------------------------------
// Called on load. Addon should fully initalize or return error status
//-----------------------------------------------------------------------------
ADDON_STATUS ADDON_Create(void* hdl, void* props)
{
  VIS_PROPS *p = (VIS_PROPS *)props;

  g_pathPresets = p->presets;
  width = p->width;
  height = p->height;

  audio_data = new byte[AUDIO_BUFFER]();
  magnitude_buffer = new float[NUM_BANDS]();
  pcm = new float[AUDIO_BUFFER]();

  cfg = kiss_fft_alloc(AUDIO_BUFFER, 0, NULL, NULL);

  if (S_OK != dxInit(reinterpret_cast<ID3D11DeviceContext*>(p->device))) 
  {
    return ADDON_STATUS_PERMANENT_FAILURE;
  }

  if (!initialized)
  {
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
}

//-- Destroy ------------------------------------------------------------------
// Do everything before unload of this add-on
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" void ADDON_Destroy()
{
  unloadPreset();

  if (lpresets)
    delete[] lpresets, lpresets = nullptr;

  SAFE_RELEASE(pAudioView);
  SAFE_RELEASE(pAudioTexture);
  SAFE_RELEASE(pNoiseView);
  SAFE_RELEASE(pBackBuffer);
  SAFE_RELEASE(pNoiseSampler);
  SAFE_RELEASE(pAudioSampler);

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
  return true;
}

//-- GetStatus ---------------------------------------------------------------
// Returns the current Status of this visualisation
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_GetStatus()
{
  return ADDON_STATUS_OK;
}

//-- GetSettings --------------------------------------------------------------
// Return the settings for XBMC to display
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" unsigned int ADDON_GetSettings(ADDON_StructSetting ***sSet)
{
  return 0;
}

//-- FreeSettings --------------------------------------------------------------
// Free the settings struct passed from XBMC
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------

extern "C" void ADDON_FreeSettings()
{
}

//-- SetSetting ---------------------------------------------------------------
// Set a specific Setting value (called from XBMC)
// !!! Add-on master function !!!
//-----------------------------------------------------------------------------
extern "C" ADDON_STATUS ADDON_SetSetting(const char *strSetting, const void* value)
{
  if (!strSetting || !value)
    return ADDON_STATUS_UNKNOWN;

  // TODO Someone _needs_ to fix this API in kodi, its terrible.
  // a) Why not use GetSettings instead of hacking SetSettings like this?
  // b) Why does it give index and not settings key?
  // c) Seemingly random ###End which if you never write will while(true) the app
  // d) Writes into const setting and value...
  if (strcmp(strSetting, "###GetSavedSettings") == 0)
  {
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
}
