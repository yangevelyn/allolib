#include "al/core.hpp"
#include <iostream>
#include <string>
#include <array>

#include "apr_general.h"
#include "apr_errno.h"
#include "apr_pools.h"

using namespace al;
using namespace std;

static apr_status_t check_apr(apr_status_t err) {
    if (err != APR_SUCCESS) {
        char errstr[256];
        apr_strerror(err, errstr, sizeof(errstr));
        fprintf(stderr, "%s\n", errstr);
    }
    return err;
}

//class MyApp : public WindowApp, public osc::PacketHandler {
class MyApp : public WindowApp {
public:
  ShaderProgram shader;
  VAOMesh mesh;
  Graphics g;
  //osc::Recv server { 16447, "", 0.05 };
  
  void onCreate() {
    string const vert_source = R"(
      #version 330

      uniform mat4 m;

      layout (location = 0) in vec4 position;
      layout (location = 1) in vec4 color;
      layout (location = 2) in vec2 texcoord;

      out vec4 _color;

      void main() {
        gl_Position = m * position;
        _color = color;
      }
    )";

    string const frag_source = R"(
      #version 330

      in vec4 _color;

      out vec4 frag_color;

      void main() {
        frag_color = _color;
      }
    )";

    shader.compile(vert_source, frag_source);

    mesh.primitive(TRIANGLES);
    mesh.vertex(-0.5, -0.5, 0);
    mesh.color(1.0, 0.0, 0.0);
    mesh.vertex(0.5, -0.5, 0);
    mesh.color(0.0, 1.0, 0.0);
    mesh.vertex(-0.5, 0.5, 0);
    mesh.color(0.0, 0.0, 1.0);
    mesh.vertex(-0.5, 0.5, 0);
    mesh.color(0.0, 0.0, 1.0);
    mesh.vertex(0.5, -0.5, 0);
    mesh.color(0.0, 1.0, 0.0);
    mesh.vertex(0.5, 0.5, 0);
    mesh.color(0.0, 1.0, 1.0);
    mesh.update();

    g.setClearColor(0, 1, 1);

    //server.handler(*this);
    //server.start();

    check_apr(apr_initialize());
    atexit(apr_terminate);	// FIXME - can we have multiple atexit calls?1
  }

  void onDraw() {
    g.viewport(0, 0, fbWidth(), fbHeight());
    g.clear();
    float w = width();
    float h = height();
    Matrix4f mat = Matrix4f::rotate(sec(), 0, 0, 1);
    mat = Matrix4f::scaling(h / w, 1.0f, 1.0f) * mat;
    
    shader.begin();
    shader.uniform("m", mat);
    mesh.draw();
    shader.end();
  }

  //void onMessage(osc::Message& m) {

  //    // Check that the address and tags match what we expect
  //    if (m.addressPattern() == "/test" && m.typeTags() == "si") {

  //        // Extract the data out of the packet
  //        std::string str;
  //        int val;
  //        m >> str >> val;

  //        // Print out the extracted packet data
  //        std::cout << "SERVER: recv " << str << " " << val << "\n";
  //    }
  //}
};

class MyAudioApp : public AudioApp {
public:
  void onSound(AudioIOData& io) {
    static double phase {0};
    // Set the base frequency to 55 Hz
    double freq = 55/io.framesPerSecond();

    while(io()){
      // Update the oscillators' phase
      phase += freq;
      if(phase > 1) phase -= 1;

      // Generate two sine waves at the 5th and 4th harmonics
      float out1 = cos(5*phase * 2*M_PI);
      float out2 = sin(4*phase * 2*M_PI);

      // Send scaled waveforms to output...
      io.out(0) = out1*0.2;
      io.out(1) = out2*0.2;
    }
  }
};

int main(int argc, char* argv[]) {
  MyAudioApp audioApp;
  audioApp.initAudio();
  audioApp.begin();

  MyApp app;
  app.dimensions(1000, 500);
  app.title("mesh test");
  app.start();

  audioApp.end();
  return 0;
}