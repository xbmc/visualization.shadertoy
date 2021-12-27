/*
 *  Copyright (C) 2005-2021 Team Kodi (https://kodi.tv)
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSE.md for more information.
 */

#include "PresetsLoader.h"

#include <json/json.h>
#include <kodi/Filesystem.h>
#include <kodi/General.h>
#include <kodi/tools/StringUtils.h>

bool CPresetLoader::Load(const std::string& path)
{
  bool ret = false;
  char* ptr = nullptr;

  try
  {
    kodi::vfs::CFile file;
    if (!file.OpenFile(path))
    {
      throw kodi::tools::StringUtils::Format("Failed to open shader list file %s", path.c_str());
    }

    ssize_t length = file.GetLength();
    if (length <= 0)
    {
      throw kodi::tools::StringUtils::Format("Shader list file %s seems empty and not usable",
                                             path.c_str());
    }

    ptr = new char[length + 1]{0};
    ssize_t pos = 0;
    do
    {
      ssize_t ret = file.Read(ptr, length);
      if (ret < 0)
      {
        throw kodi::tools::StringUtils::Format("Failed to read shader list file %s", path.c_str());
      }
      pos += ret;
    } while (pos < length);

    try
    {
      JSONCPP_STRING err;
      Json::Value root;
      Json::CharReaderBuilder builder;
      const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
      if (!reader->parse(ptr, ptr + length, &root, &err))
      {
        throw kodi::tools::StringUtils::Format(
            "Failed to parse json within shader list file %s (ERROR: %s)", path.c_str(),
            err.c_str());
      }

      for (unsigned int index = 0; index < root["presets"].size(); index++)
      {
        const auto& entry = root["presets"][index];
        Preset preset;

        // Check the optional JSON value at the end of the preset, which sets
        // possible limits, e.g. supporting only GL ("gl_only")
        if (entry.size() > 6 && entry[6].isString())
        {
#if !defined(HAS_GL)
          if (kodi::tools::StringUtils::CompareNoCase(entry[6].asString(), "gl_only") == 0)
            continue;
#endif
        }

        if (entry[0].isString())
          preset.name = entry[0].asString();
        else
          preset.name = kodi::addon::GetLocalizedString(entry[0].asInt(), "Unknown preset name " +
                                                                            std::to_string(index + 1));

        // Check shader file included within addon or outside and set by user
        const std::string usedDirName = kodi::vfs::GetDirectoryName(entry[1].asString());
        if (usedDirName.rfind("./", 0) == 0)
          preset.file =
              kodi::vfs::GetDirectoryName(path) + kodi::vfs::GetFileName(entry[1].asString());
        else if (usedDirName.rfind("../", 0) == 0)
          preset.file = kodi::vfs::GetDirectoryName(path) + entry[1].asString();
        else if (!usedDirName.empty())
          preset.file = entry[1].asString();
        else
          preset.file = kodi::addon::GetAddonPath("resources/shaders/" + entry[1].asString());

        if (!kodi::vfs::FileExists(preset.file))
          throw kodi::tools::StringUtils::Format("On %s defined GLSL shader file '%s' not found",
                                                 path.c_str(), preset.file.c_str());

        for (unsigned int i = 0; i < 4; ++i)
        {
          // Check shader file included within addon or outside and set by user
          const std::string usedDirName = kodi::vfs::GetDirectoryName(entry[i + 2].asString());
          if (kodi::tools::StringUtils::CompareNoCase(entry[i + 2].asString(), "audio") == 0)
            preset.channel[i] = "audio";
          else if (usedDirName.rfind("./", 0) == 0)
            preset.channel[i] =
                kodi::vfs::GetDirectoryName(path) + kodi::vfs::GetFileName(entry[i + 2].asString());
          else if (usedDirName.rfind("../", 0) == 0)
            preset.channel[i] = kodi::vfs::GetDirectoryName(path) + entry[i + 2].asString();
          else if (!usedDirName.empty())
            preset.channel[i] = entry[i + 2].asString();
          else if (!entry[i + 2].asString().empty())
            preset.channel[i] = kodi::addon::GetAddonPath("resources/" + entry[i + 2].asString());

          if (!preset.channel[i].empty() && preset.channel[i] != "audio")
          {
            if (!kodi::vfs::FileExists(preset.channel[i]))
              throw kodi::tools::StringUtils::Format("On %s defined texture image '%s' not found",
                                                     path.c_str(), preset.channel[i].c_str());
          }
        }

        m_presets.emplace_back(preset);
      }
    }
    catch (Json::LogicError err)
    {
      throw kodi::tools::StringUtils::Format("Json throwed an exception by parse of %s with '%s'",
                                             path.c_str(), err.what());
    }
    catch (std::string err)
    {
      throw std::move(err);
    }
    catch (...)
    {
      throw kodi::tools::StringUtils::Format("Exception by parse of %s", path.c_str());
    }

    ret = true;
  }
  catch (std::string err)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: %s", __func__, err.c_str());
  }
  catch (...)
  {
    kodi::Log(ADDON_LOG_ERROR, "%s: Unknown exception by load of %s", __func__, path.c_str());
  }

  if (ptr)
    delete[] ptr;

  // Inform user that maybe their own preset list is wrong
  if (!ret)
    kodi::QueueNotification(QUEUE_OWN_STYLE, kodi::addon::GetLocalizedString(30030),
                            kodi::addon::GetLocalizedString(30031), "", 5000, true, 20000);

  return ret;
}

bool CPresetLoader::GetAvailablePresets(std::vector<std::string>& presets)
{
  for (auto preset : m_presets)
    presets.push_back(preset.name);
  return true;
}

int CPresetLoader::GetPresetsAmount()
{
  return static_cast<int>(m_presets.size());
}

Preset CPresetLoader::GetPreset(int number)
{
  if (number >= 0 && number < m_presets.size())
    return m_presets[number];

  static Preset empty;
  return empty;
}
