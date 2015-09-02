/** @file
    @brief

    @date 2015

    @author
    Sensics, Inc.
    <http://sensics.com/osvr>
*/

// Copyright 2015 Sensics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//        http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Internal Includes
#include <osvr/ClientKit/ClientKit.h>
#include <osvr/ClientKit/Display.h>

// Library/third-party includes
#include <SDL.h>

#include <glm/vec2.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp> // for glm::ortho
#include <glm/gtc/type_ptr.hpp>

#include "SDL2Helpers.h"

// Standard includes
#include <iostream>
#include <cstdint>
#include <cmath>
#if 0
static auto const WIDTH = 1920;
static auto const HEIGHT = 1080;
#endif

static auto const WIDTH = 1920 / 2;
static auto const HEIGHT = 1080 / 2;

using Radius = std::uint16_t;

// Forward declarations of rendering functions defined below.
void myDrawCircle(glm::vec2 const &center, Radius radius);

void DrawCircle(float cx, float cy, float r, int num_segments) {
    float theta = 2 * 3.1415926 / float(num_segments);
    float tangetial_factor = tanf(theta); // calculate the tangential factor

    float radial_factor = cosf(theta); // calculate the radial factor

    float x = r; // we start at angle = 0

    float y = 0;

    glBegin(GL_LINE_LOOP);
    for (int ii = 0; ii < num_segments; ii++) {
        glVertex2f(x + cx, y + cy); // output vertex

        // calculate the tangential vector
        // remember, the radial vector is (x, y)
        // to get the tangential vector we flip those coordinates and negate one
        // of them

        float tx = -y;
        float ty = x;

        // add the tangential vector

        x += tx * tangetial_factor;
        y += ty * tangetial_factor;

        // correct using the radial factor

        x *= radial_factor;
        y *= radial_factor;
    }
    glEnd();
}

void circle(float x, float y, float r, int segments) {
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int n = 0; n <= segments; ++n) {
        float const t = 2 * M_PI * (float)n / (float)segments;
        glVertex2f(x + sin(t) * r, y + cos(t) * r);
    }
    glEnd();
}

inline std::ostream &operator<<(std::ostream &os,
                                osvr::clientkit::Surface const &s) {
    os << "Viewer " << int(s.getViewerID()) << ", Eye " << int(s.getEyeID())
       << ", Surface " << int(s.getSurfaceID());
    return os;
}

template <typename F> inline void glxxBegin(GLenum primitive, F &&f) {
    glBegin(primitive);
    f();
    glEnd();
}

template <typename F> inline void glxxPushMatrix(F &&f) {
    glPushMatrix();
    f();
    glPopMatrix();
}

static void rectangle() {
    static const GLfloat matspec[] = {0.5, 0.5, 0.5, 0.0};
    static const GLfloat col[] = {1.f, 1.f, 1.f};
    glMaterialfv(GL_FRONT, GL_SPECULAR, matspec);
    glMaterialf(GL_FRONT, GL_SHININESS, 64.0);
    glxxBegin(GL_QUADS, [&] {
        static const float bound = 20.f;
        glColor3fv(col);
        glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, col);
        glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, col);
        glNormal3f(0.0, 0.0, 1.0);
        glVertex2f(bound, bound);
        glVertex2f(-bound, bound);
        glVertex2f(-bound, -bound);
        glVertex2f(bound, -bound);
    });
}

class EyeSurfaceCalibration {
  public:
    EyeSurfaceCalibration(osvr::clientkit::Surface const &s,
                          osvr::clientkit::DisplayConfig &display)
        : m_surface(s), m_display(display),
          m_viewport(m_surface.getRelativeViewport()),
          m_size(m_viewport.width, m_viewport.height),
          m_projection(glm::ortho(0.f, m_size.x, 0.f, m_size.y)),
          m_radius(std::min(m_viewport.width, m_viewport.height)),
          m_center(m_size / 2.f) {

        std::cout << s << std::endl;
    }
    void changeSize(std::int32_t change) { m_radius += change; }
    void move(glm::vec2 const &offset) { m_center += offset; }

    glm::vec2 const &getCenter() const { return m_center; }
    Radius getRadius() const { return m_radius; }
    /// @brief Entry point for rendering
    void render() {

        // Clear the screen to a light blue
        glClearColor(.3, .3, .8, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        /// For each viewer, eye, surface combination...
        m_display.forEachSurface(
            [&](osvr::clientkit::Surface surface) { handleSurface(surface); });
    }

  private:
    void handleSurface(osvr::clientkit::Surface const &surface) {
        std::cout << "Render: " << surface;
        if (surface != m_surface) {
            std::cout << " - skip this surface" << std::endl;
            return;
        }
        std::cout << " - draw this surface" << std::endl;

        /// Identity modelview - for now
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        std::cout << "Viewport" << static_cast<GLint>(m_viewport.left) << "<"
                  << static_cast<GLint>(m_viewport.bottom) << "<"
                  << static_cast<GLsizei>(m_viewport.width) << "<"
                  << static_cast<GLsizei>(m_viewport.height) << std::endl;
        /// Use the viewport provided
        glViewport(static_cast<GLint>(m_viewport.left),
                   static_cast<GLint>(m_viewport.bottom),
                   static_cast<GLsizei>(m_viewport.width),
                   static_cast<GLsizei>(m_viewport.height));

        /// Don't use the projection matrix from OSVR - we want the ortho one we
        /// made at instantiation.
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glMultMatrixf(glm::value_ptr(m_projection));

        glMatrixMode(GL_MODELVIEW);


        // rectangle();
        // myDrawCircle(m_center, m_radius);
        // DrawCircle(m_center.x, m_center.y, m_radius, 64);
    }
    osvr::clientkit::Surface m_surface;
    osvr::clientkit::DisplayConfig &m_display;
    osvr::clientkit::RelativeViewport m_viewport;
    glm::vec2 m_size;
    glm::mat4 m_projection;
    glm::vec2 m_center;
    Radius m_radius;
};

void russDrawCircle(glm::vec2 center, float radius) {
    glBegin(GL_LINE_STRIP);
    float step = 1 / radius;
    for (float r = 0; r <= 2 * M_PI; r += step) {
        glm::vec2 p(center.x + radius * cos(r),
            center.y + radius * sin(r));
        glVertex2fv(glm::value_ptr(p));
    }
    glEnd();
}
class CalibrationRoutine {
  public:
    CalibrationRoutine() : ctx("org.osvr.OpticalCalibration"), display(ctx) {

        if (!display.valid()) {
            std::cerr << "\nCould not get display config (server probably not "
                         "running or not behaving), exiting."
                      << std::endl;
            throw std::runtime_error("Could not get display config");
        }

        std::cout << "Waiting for the display to fully start up, including "
                     "receiving initial pose update..."
                  << std::endl;
        while (!display.checkStartup()) {
            ctx.update();
        }
        std::cout << "OK, display startup status is good!" << std::endl;
    }

    void operator()() {
        // Create a window
        window = osvr::SDL2::createWindow(
            "OSVR", 15, 15, WIDTH, HEIGHT,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN); // | SDL_WINDOW_BORDERLESS);
        {
            // Create an OpenGL context and make it current.
            osvr::SDL2::GLContext glctx(window.get());
            glDisable(GL_LIGHTING);
            glDisable(GL_DEPTH_TEST);
            glDisable(GL_TEXTURE_2D);
#ifndef __ANDROID__ // Don't want to pop up the on-screen keyboard
            osvr::SDL2::TextInput textinput;
#endif
#if 1
            SDL_Event e;
            bool ok = true;
            while (ok) {
                while (SDL_PollEvent(&e)) {
                    switch (e.type) {
                    case SDL_QUIT:
                        // Handle some system-wide quit event
                        ok = false;
                        break;
                    }
                }
                SDL_GL_MakeCurrent(window.get(), glctx);

                // Clear the screen to a light blue
                glClearColor(.3, .3, .8, 1.0f);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                /// Use the viewport provided
                glViewport(0,
                    0,
                    256,
                    256);

                /// Don't use the projection matrix from OSVR - we want the ortho one we
                /// made at instantiation.
                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                //glMultMatrixf(glm::value_ptr(glm::ortho(-256.f, 256.f, -256.f, 256.f, 0.1f, 100.f)));
                glOrtho(0, 256, 0, 256, 1, 10);
                glMatrixMode(GL_MODELVIEW);
                glLoadIdentity();
                glTranslatef(0.f,0.f, -1.f );
                glColor4f(1.0f, 1.0f, 1.0f, 1.0f); // sets color to white.
                russDrawCircle(glm::vec2(128, 128), 50);
                SDL_GL_SwapWindow(window.get());
            }

#endif
#if 0
            display.forEachSurface([&](osvr::clientkit::Surface surface) {
                handleSurface(surface);
            });
#endif
        }
        window = nullptr;
    }

  private:
    void setQuit() { quit = true; }
    void setSurfaceDone() { surfaceDone = true; }

    void handleSurface(osvr::clientkit::Surface surface) {
        if (quit) {
            return;
        }
        surfaceDone = false;
        // Event handler
        SDL_Event e;
        auto calib = EyeSurfaceCalibration{surface, display};
        while (!surfaceDone && !quit) {
            // Handle all queued events
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    // Handle some system-wide quit event
                    setQuit();
                    break;
                case SDL_KEYDOWN:
                    // Handle a keypress
                    handleKeypress(calib, e.key.keysym);
                    break;
                }
            }

            // Update OSVR
            ctx.update();

            // Render
            calib.render();

            // Swap buffers
            SDL_GL_SwapWindow(window.get());
        }

        std::cout << "Center: " << calib.getCenter().x << ", "
                  << calib.getCenter().y << "\t Radius: " << calib.getRadius()
                  << std::endl;
    }

    void handleKeypress(EyeSurfaceCalibration &calib, SDL_Keysym key) {
        auto posChange = 1.f;
        auto sizeChange = std::int32_t{1};
        switch (key.scancode) {
        // Quit the whole app
        case SDL_SCANCODE_ESCAPE:
            setQuit();
            return;

        // Move circle
        case SDL_SCANCODE_RIGHT:
            calib.move(glm::vec2{posChange, 0});
            return;
        case SDL_SCANCODE_LEFT:
            calib.move(glm::vec2{-posChange, 0});
            return;
        case SDL_SCANCODE_UP:
            calib.move(glm::vec2{0, posChange});
            return;
        case SDL_SCANCODE_DOWN:
            calib.move(glm::vec2{0, -posChange});
            return;

        // Change circle size
        case SDL_SCANCODE_KP_PLUS:
        case SDL_SCANCODE_EQUALS: // aka plus without the shift key
            calib.changeSize(sizeChange);
            return;
        case SDL_SCANCODE_KP_MINUS:
        case SDL_SCANCODE_MINUS:
            calib.changeSize(-sizeChange);
            return;

        // Completed with this surface
        case SDL_SCANCODE_RETURN:
            setSurfaceDone();
            return;
        }
    }

  private:
    osvr::clientkit::ClientContext ctx;
    osvr::clientkit::DisplayConfig display;
    osvr::SDL2::WindowPtr window;
    bool quit = false;
    bool surfaceDone = false;
};

int main(int argc, char *argv[]) {

    osvr::SDL2::Lib lib;

    // Use OpenGL 2.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    CalibrationRoutine app;
    app();
    return 0;
}

void vertex(float x, float y) {
    std::cout << x << "," << y << "\n";
    glVertex2f(static_cast<GLfloat>(x), static_cast<GLfloat>(y));
}
void myDrawCircle(glm::vec2 const &center, Radius radius) {
    glPushMatrix();
    glTranslatef(center.x, center.y, 0.f);
    // glScalef(radius, radius, radius);

    glColor3f(1.f, 1.f, 1.f);
    glBegin(GL_POLYGON);
    // Starting place
    vertex(radius, 0.f);
    static const int segments = 8;
    // Yes, this loop has an extra iteration on the end to close the line.
    for (int n = 0; n <= segments; ++n) {
        auto const t =
            static_cast<float>(2 * M_PI * n) / static_cast<float>(segments);
        vertex(std::sin(t) * radius, std::cos(t) * radius);
    }
    glEnd();
    glPopMatrix();
}
