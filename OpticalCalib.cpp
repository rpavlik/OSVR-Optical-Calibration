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

#include <SDL2pp/SDL.hh>
#include <SDL2pp/Window.hh>
#include <SDL2pp/Renderer.hh>
#include <GL/glew.h>

#include "SDL2Helpers.h"

// Standard includes
#include <iostream>

static auto const WIDTH = 1920;
static auto const HEIGHT = 1080;

namespace osvrSDL = osvr::SDL2;
// Forward declarations of rendering functions defined below.
void render(osvr::clientkit::DisplayConfig &disp);
void renderScene();

class EyeSurfaceCalibration {
  public:
    EyeSurfaceCalibration(osvr::clientkit::Surface const &s, SDL2pp::Window & w, osvr::clientkit::ClientContext & ctx, osvr::clientkit::DisplayConfig & display)
        : m_surface(s), m_window(w), m_ctx(ctx), m_display(display) {}

    /// @brief Entry point for a calibration
    void operator()() {
        // Event handler
        SDL_Event e;
#ifndef __ANDROID__ // Don't want to pop up the on-screen keyboard
        osvrSDL::TextInput textinput;
#endif
        bool quit = false;
        while (!quit) {
            // Handle all queued events
            while (SDL_PollEvent(&e)) {
                switch (e.type) {
                case SDL_QUIT:
                    // Handle some system-wide quit event
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    if (SDL_SCANCODE_ESCAPE == e.key.keysym.scancode) {
                        // Handle pressing ESC
                        quit = true;
                    }
                    break;
                }
                if (e.type == SDL_QUIT) {
                    quit = true;
                }
            }

            // Update OSVR
            m_ctx.update();

            // Render
            render();

            // Swap buffers
            SDL_GL_SwapWindow(m_window.Get());
        }
    }

    void render() {

        // Clear the screen to black and clear depth
        glClearColor(0, 0, 0, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        /// For each viewer, eye combination...
        m_display.forEachEye([](osvr::clientkit::Eye eye) {

            /// Try retrieving the view matrix (based on eye pose) from OSVR
            double viewMat[OSVR_MATRIX_SIZE];
            eye.getViewMatrix(OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS,
                viewMat);
            /// Initialize the ModelView transform with the view matrix we
            /// received
            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            glMultMatrixd(viewMat);

            /// For each display surface seen by the given eye of the given
            /// viewer...
            eye.forEachSurface([](osvr::clientkit::Surface surface) {
                auto viewport = surface.getRelativeViewport();
                glViewport(static_cast<GLint>(viewport.left),
                    static_cast<GLint>(viewport.bottom),
                    static_cast<GLsizei>(viewport.width),
                    static_cast<GLsizei>(viewport.height));

                /// Set the OpenGL projection matrix based on the one we
                /// computed.
                double zNear = 0.1;
                double zFar = 100;
                double projMat[OSVR_MATRIX_SIZE];
                surface.getProjectionMatrix(
                    zNear, zFar, OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS |
                    OSVR_MATRIX_SIGNEDZ | OSVR_MATRIX_RHINPUT,
                    projMat);

                glMatrixMode(GL_PROJECTION);
                glLoadIdentity();
                glMultMatrixd(projMat);

                /// Set the matrix mode to ModelView, so render code doesn't
                /// mess with the projection matrix on accident.
                glMatrixMode(GL_MODELVIEW);

                /// Call out to render our scene.
                renderScene();
            });
        });
    }

  private:
    osvr::clientkit::Surface m_surface;
    SDL2pp::Window & m_window;
    osvr::clientkit::ClientContext & m_ctx;
    osvr::clientkit::DisplayConfig & m_display;
    double m_x = 0.5;
    double m_y = 0.5;
    double m_radius = 0.5;
};

int main(int argc, char *argv[]) {

    // Open SDL
    SDL2pp::SDL lib(SDL_INIT_VIDEO);

    // Use OpenGL 2.1
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);

    // Create a window
    auto window = SDL2pp::Window{"OSVR",
                                 0,
                                 0,
                                 WIDTH,
                                 HEIGHT,
                                 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_BORDERLESS};

    // Create an OpenGL context and make it current.
    osvrSDL::GLContext glctx(window.Get());

    // Turn on V-SYNC
    SDL_GL_SetSwapInterval(1);

    // Start OSVR and get OSVR display config
    osvr::clientkit::ClientContext ctx("org.osvr.OpticalCalibration");
    osvr::clientkit::DisplayConfig display(ctx);
    if (!display.valid()) {
        std::cerr << "\nCould not get display config (server probably not "
                     "running or not behaving), exiting."
                  << std::endl;
        return -1;
    }

    std::cout << "Waiting for the display to fully start up, including "
                 "receiving initial pose update..."
              << std::endl;
    while (!display.checkStartup()) {
        ctx.update();
    }
    std::cout << "OK, display startup status is good!" << std::endl;



    return 0;
}

/// @brief A simple dummy "draw" function - note that drawing occurs in "room
/// space" by default. (that is, in this example, the modelview matrix when this
/// function is called is initialized such that it transforms from world space
/// to view space)
void renderScene() {}

/// @brief The "wrapper" for rendering to a device described by OSVR.
///
/// This function will set up viewport, initialize view and projection matrices
/// to current values, then call `renderScene()` as needed (e.g. once for each
/// eye, for a simple HMD.)
void render(osvr::clientkit::DisplayConfig &disp) {

    // Clear the screen to black and clear depth
    glClearColor(0, 0, 0, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /// For each viewer, eye combination...
    disp.forEachEye([](osvr::clientkit::Eye eye) {

        /// Try retrieving the view matrix (based on eye pose) from OSVR
        double viewMat[OSVR_MATRIX_SIZE];
        eye.getViewMatrix(OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS,
                          viewMat);
        /// Initialize the ModelView transform with the view matrix we
        /// received
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMultMatrixd(viewMat);

        /// For each display surface seen by the given eye of the given
        /// viewer...
        eye.forEachSurface([](osvr::clientkit::Surface surface) {
            auto viewport = surface.getRelativeViewport();
            glViewport(static_cast<GLint>(viewport.left),
                       static_cast<GLint>(viewport.bottom),
                       static_cast<GLsizei>(viewport.width),
                       static_cast<GLsizei>(viewport.height));

            /// Set the OpenGL projection matrix based on the one we
            /// computed.
            double zNear = 0.1;
            double zFar = 100;
            double projMat[OSVR_MATRIX_SIZE];
            surface.getProjectionMatrix(
                zNear, zFar, OSVR_MATRIX_COLMAJOR | OSVR_MATRIX_COLVECTORS |
                                 OSVR_MATRIX_SIGNEDZ | OSVR_MATRIX_RHINPUT,
                projMat);

            glMatrixMode(GL_PROJECTION);
            glLoadIdentity();
            glMultMatrixd(projMat);

            /// Set the matrix mode to ModelView, so render code doesn't
            /// mess with the projection matrix on accident.
            glMatrixMode(GL_MODELVIEW);

            /// Call out to render our scene.
            renderScene();
        });
    });

    /// Successfully completed a frame render.
}
