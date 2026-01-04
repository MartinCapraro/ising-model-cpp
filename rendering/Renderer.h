#pragma once

// This abstract class defines a contract specific instances of renderers must follow
class Renderer {
public:
    virtual ~Renderer() = default;
    virtual void draw(const int* latticeState, int latticeDimension) = 0;
};