/**
 ******************************************************************************
 * Xenia : Xbox 360 Emulator Research Project                                 *
 ******************************************************************************
 * Copyright 2023 Ben Vanik. All rights reserved.                             *
 * Released under the BSD license - see LICENSE in the root for more details. *
 ******************************************************************************
 */

#define _USE_MATH_DEFINES

#include "xenia/hid/winkey/hookables/GoW.h"

#include "xenia/base/platform_win.h"
#include "xenia/cpu/processor.h"
#include "xenia/emulator.h"
#include "xenia/hid/hid_flags.h"
#include "xenia/hid/input_system.h"
#include "xenia/kernel/util/shim_utils.h"
#include "xenia/kernel/xmodule.h"
#include "xenia/kernel/xthread.h"
#include "xenia/xbox.h"

using namespace xe::kernel;

DECLARE_double(sensitivity);
DECLARE_bool(invert_y);
DECLARE_bool(invert_x);

const uint32_t kTitleIdGoW3 = 0x4D5308AB;
const uint32_t kTitleIdGoW2 = 0x4D53082D;
const uint32_t kTitleIdGoW = 0x4D5307D5;

namespace xe {
namespace hid {
namespace winkey {
struct GameBuildAddrs {
  const char* title_version;
  uint32_t x_address;
  uint32_t y_address;
};

std::map<GearsOfWarGame::GameBuild, GameBuildAddrs> supported_builds{
    {GearsOfWarGame::GameBuild::GearsOfWar3_TU0,
     {"11.0", 0x437612A6, 0x437612A2}},
    {GearsOfWarGame::GameBuild::GearsOfWar2_TU0,
     {"5.0", 0x40E66266, 0x40E66262}}};

GearsOfWarGame::~GearsOfWarGame() = default;

bool GearsOfWarGame::IsGameSupported() {
  if (kernel_state()->title_id() != kTitleIdGoW3 &&
      kernel_state()->title_id() != kTitleIdGoW2 &&
      kernel_state()->title_id() != kTitleIdGoW) {
    return false;
  }

  const std::string current_version =
      kernel_state()->emulator()->title_version();

  for (auto& build : supported_builds) {
    if (current_version == build.second.title_version) {
      game_build_ = build.first;
      return true;
    }
  }

  return false;
}

std::string GearsOfWarGame::ChooseBinds() { return "Default"; }

bool GearsOfWarGame::DoHooks(uint32_t user_index, RawInputState& input_state,
                             X_INPUT_STATE* out_state) {
  if (!IsGameSupported()) {
    return false;
  }

  if (supported_builds.count(game_build_) == 0) {
    return false;
  }

  // Don't constantly write if there is no mouse movement.
  if (input_state.mouse.x_delta == 0 || input_state.mouse.y_delta == 0) {
    return false;
  }

  xe::be<uint16_t>* x_axis =
      kernel_memory()->TranslateVirtual<xe::be<uint16_t>*>(
          supported_builds[game_build_].x_address);

  xe::be<uint16_t>* y_axis =
      kernel_memory()->TranslateVirtual<xe::be<uint16_t>*>(
          supported_builds[game_build_].y_address);

  if (x_axis == nullptr) {
    return false;
  }

  if (y_axis == nullptr) {
    return false;
  }

  uint16_t x_delta = static_cast<uint16_t>((input_state.mouse.x_delta * 10) *
                                           cvars::sensitivity);
  uint16_t y_delta = static_cast<uint16_t>((input_state.mouse.y_delta * 10) *
                                           cvars::sensitivity);

  if (!cvars::invert_x) {
    *x_axis += x_delta;
  } else {
    *x_axis -= x_delta;
  }

  if (!cvars::invert_y) {
    *y_axis -= y_delta;
  } else {
    *y_axis += y_delta;
  }

  return true;
}

bool GearsOfWarGame::ModifierKeyHandler(uint32_t user_index,
                                        RawInputState& input_state,
                                        X_INPUT_STATE* out_state) {
  return false;
}
}  // namespace winkey
}  // namespace hid
}  // namespace xe