#pragma once

#include "Renderer.h"


class LegacyGLRenderer : public Renderer {
public:
    void draw(const int* latticeState, int latticeDimension) override;

private:
    void drawQuad(float xPos, float yPos, float r, float g, float b);
};