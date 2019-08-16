/******************************************************************************
 * Copyright © 2019 SSR Contributors                                          *
 *                                                                            *
 * This file is part of the SoundScape Renderer (SSR).                        *
 *                                                                            *
 * The SSR is free software:  you can redistribute it and/or modify it  under *
 * the terms of the  GNU  General  Public  License  as published by the  Free *
 * Software Foundation, either version 3 of the License,  or (at your option) *
 * any later version.                                                         *
 *                                                                            *
 * The SSR is distributed in the hope that it will be useful, but WITHOUT ANY *
 * WARRANTY;  without even the implied warranty of MERCHANTABILITY or FITNESS *
 * FOR A PARTICULAR PURPOSE.                                                  *
 * See the GNU General Public License for more details.                       *
 *                                                                            *
 * You should  have received a copy  of the GNU General Public License  along *
 * with this program.  If not, see <http://www.gnu.org/licenses/>.            *
 *                                                                            *
 * The SSR is a tool  for  real-time  spatial audio reproduction  providing a *
 * variety of rendering algorithms.                                           *
 *                                                                            *
 * http://spatialaudio.net/ssr                           ssr@spatialaudio.net *
 ******************************************************************************/

/// @file
/// Dynamic scene loaded from ASDF file.

#ifndef SSR_DYNAMIC_SCENE_H
#define SSR_DYNAMIC_SCENE_H

#include <vector>

#include "asdf.h"
#include "geometry.h"

namespace ssr
{

using frame_count_t = uint64_t;

struct Transform
{
  std::optional<quat> rot{};
  std::optional<vec3> pos{};

  // TODO: more stuff
};

class DynamicScene
{
public:
  // TODO: pass input prefix for live sources?
  DynamicScene(const std::string& scene_file_name, uint32_t samplerate
      , uint32_t blocksize, uint32_t buffer_blocks, uint64_t usleeptime)
    : _ptr(asdf_scene_new(
          scene_file_name.c_str(), samplerate, blocksize, buffer_blocks,
          usleeptime))
  {
    if (!_ptr) {
      throw std::runtime_error(asdf_scene_last_error());
    }
    auto file_sources = this->file_sources();
    _audio_data.resize(file_sources * blocksize);
    for (size_t i = 0; i < file_sources; ++i)
    {
      _file_source_ptrs.push_back(_audio_data.data() + i * blocksize);
    }
  }

  ~DynamicScene()
  {
    asdf_scene_free(_ptr);
  }

  size_t file_sources() const
  {
    return asdf_scene_file_sources(_ptr);
  }

  size_t live_sources() const
  {
    // TODO: implement
    return 0;
  }

  std::string get_source_id(size_t index) const {
    assert(_ptr);
    char* ptr = asdf_scene_get_source_id(_ptr, index);
    std::string id = ptr;
    asdf_string_free(ptr);
    return id;
  }

  void free_source_id(char* id) const {
    asdf_string_free(id);
  }

  const float* file_source_ptr(size_t index) const
  {
    return _file_source_ptrs[index];
  }

  bool seek(size_t frame)
  {
    assert(_ptr);
    return asdf_scene_seek(_ptr, frame);
  }

  /// This is realtime-safe
  void update_audio_data(bool rolling)
  {
    assert(_ptr);
    auto success = asdf_scene_get_audio_data(
        _ptr, _file_source_ptrs.data(), rolling);
    if (!success)
    {
      // TODO: use asdf_scene_last_error() once it is supported
      throw std::runtime_error("Unrecoverable error getting audio data");
    }
  }

  /// This is realtime-safe
  /// source_number is 0-based
  std::optional<Transform>
  get_source_transform(size_t source_idx, frame_count_t frame) const
  {
    assert(_ptr);
    auto t = asdf_scene_get_source_transform(_ptr, source_idx, frame);
    std::optional<Transform> result{};
    if (t.flags & ASDF_TRANSFORM_ACTIVE)
    {
      result = Transform{};
      if (t.flags & ASDF_TRANSFORM_POS)
      {
        result->pos = vec3{t.pos[0], t.pos[1], t.pos[2]};
      }
      // TODO: other fields
    }
    return result;
  }

private:
  AsdfScene* _ptr;
  std::vector<float> _audio_data;
  std::vector<float*> _file_source_ptrs;
};

}  // namespace ssr

#endif
