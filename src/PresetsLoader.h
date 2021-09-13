/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#pragma once

#include <string>
#include <vector>

struct Preset
{
  std::string name;
  std::string file;
  std::string channel[4];
};

class CPresetLoader
{
public:
  CPresetLoader() = default;
  ~CPresetLoader() = default;

  bool Load(const std::string& path);

  bool GetAvailablePresets(std::vector<std::string>& presets);
  int GetPresetsAmount();
  Preset GetPreset(int number);

private:
  std::vector<Preset> m_presets;
};
