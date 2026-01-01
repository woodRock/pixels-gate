#pragma once

namespace PixelsEngine {

struct Camera {
  float x = 0.0f;
  float y = 0.0f;
  int width;
  int height;

  Camera(int w, int h) : width(w), height(h) {}
};

} // namespace PixelsEngine
