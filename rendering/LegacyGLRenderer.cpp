#include "LegacyGLRenderer.h"

#include "../globals.h"
#include <GL/glew.h>


void LegacyGLRenderer::draw(const int* latticeState, int latticeDimension)
{
    for (int x = 0; x < latticeDimension; x++) {
        for (int y = 0; y < latticeDimension; y++) {
            float xPos = static_cast<float>(x);
            float yPos = static_cast<float>(y);

            if (latticeState[x + y * latticeDimension] > 0) {
                // Green for spin up
                drawQuad(xPos, yPos, 0.0f, 1.0f, 0.39f);
            } else {
                // Orange for spin down
                drawQuad(xPos, yPos, 0.78f, 0.39f, 0.0f);
            }
        }
    }
}

void LegacyGLRenderer::drawQuad(float xPos, float yPos, float r, float g, float b) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
        glVertex2f(xPos, yPos);                 // Bottom-left corner
        glVertex2f(xPos + 1.0f, yPos);          // Bottom-right corner
        glVertex2f(xPos + 1.0f, yPos + 1.0f);   // Top-right corner
        glVertex2f(xPos, yPos + 1.0f);          // Top-left corner
    glEnd();
}
