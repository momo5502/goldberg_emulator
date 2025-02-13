/*
 * Copyright (C) Nemirtingas
 * This file is part of the ingame overlay project
 *
 * The ingame overlay project is free software; you can redistribute it
 * and/or modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
 * 
 * The ingame overlay project is distributed in the hope that it will be
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the ingame overlay project; if not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../dll/base.h"

#include <future>
#include <chrono>
#include <memory>

#include "Renderer_Hook.h"

/* Timeout is in seconds.
   Used for determining which rendering API is the real one used by
   the app when multiple APIs are used. (NVIDIA, Wine, etc.)

   This is an internally used value, separate from the timeout given
   to DetectRenderer().

   If you define it below the minimal level (one second), the minimal level
   will be used.

   Any timeout given to DetectRenderer() less than this value will be upgraded
   to it.
*/
#define MAX_RENDERER_API_DETECT_TIMEOUT 1

namespace ingame_overlay {

std::future<Renderer_Hook*> DetectRenderer(std::chrono::milliseconds timeout = std::chrono::milliseconds{ -1 });
void StopRendererDetection();
void FreeDetector();

}