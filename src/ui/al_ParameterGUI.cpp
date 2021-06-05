#include "al/ui/al_ParameterGUI.hpp"

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace al;
using namespace std;

class Sub : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        // addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.0, 0.0, 1.0);
        createInternalTriggerParameter("envDur",1, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cf2", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("bw1", 700.0, 10.0, 5000);
        createInternalTriggerParameter("bw2", 900.0, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("hmnum", 12.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.0, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(amplitude,  amplitude, -4);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

void ParameterGUI::drawVectorParameters(std::vector<ParameterMeta *> params,
                                        string suffix) {
  for (auto *param : params) {
    drawParameterMeta(std::vector<ParameterMeta *>{param}, suffix);
  }
}

void ParameterGUI::drawParameterMeta(ParameterMeta *param, string suffix) {
  drawParameterMeta(std::vector<ParameterMeta *>{param}, suffix);
}

void ParameterGUI::drawParameter(Parameter *param, std::string suffix) {
  drawParameter(std::vector<Parameter *>{param}, suffix);
}

void ParameterGUI::drawParameterString(ParameterString *param, string suffix) {
  drawParameterString(std::vector<ParameterString *>{param}, suffix);
}

void ParameterGUI::drawParameterInt(ParameterInt *param, string suffix) {
  drawParameterInt(std::vector<ParameterInt *>{param}, suffix);
}

void ParameterGUI::drawParameterBool(ParameterBool *param, std::string suffix) {
  drawParameterBool(std::vector<ParameterBool *>{param}, suffix);
}

void ParameterGUI::drawParameterPose(ParameterPose *pose, std::string suffix) {
  drawParameterPose(std::vector<ParameterPose *>{pose}, suffix);
}

void ParameterGUI::drawParameterColor(ParameterColor *param,
                                      std::string suffix) {
  drawParameterColor(std::vector<ParameterColor *>{param}, suffix);
}

void ParameterGUI::drawMenu(ParameterMenu *param, std::string suffix) {
  drawMenu(std::vector<ParameterMenu *>{param}, suffix);
}

void ParameterGUI::drawChoice(ParameterChoice *param, std::string suffix) {
  drawChoice(std::vector<ParameterChoice *>{param}, suffix);
}

void ParameterGUI::drawVec3(ParameterVec3 *param, string suffix) {
  drawVec3(std::vector<ParameterVec3 *>{param}, suffix);
}

void ParameterGUI::drawVec4(ParameterVec4 *param, string suffix) {
  drawVec4(std::vector<ParameterVec4 *>{param}, suffix);
}

void ParameterGUI::drawTrigger(Trigger *param, string suffix) {
  drawTrigger(std::vector<Trigger *>{param}, suffix);
}

void ParameterGUI::drawParameterMeta(std::vector<ParameterMeta *> params,
                                     string suffix, int index) {
  assert(params.size() > 0);

  auto &param = *params[index];
  if (strcmp(typeid(param).name(), typeid(ParameterBool).name()) ==
      0) { // ParameterBool
    drawParameterBool(dynamic_cast<ParameterBool *>(&param), suffix);
  } else if (strcmp(typeid(param).name(), typeid(Parameter).name()) ==
             0) { // Parameter
    drawParameter(dynamic_cast<Parameter *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterString).name()) == 0) { // ParameterString
    drawParameterString(dynamic_cast<ParameterString *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterInt).name()) == 0) { // ParameterInt
    drawParameterInt(dynamic_cast<ParameterInt *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterPose).name()) == 0) { // ParameterPose
    drawParameterPose(dynamic_cast<ParameterPose *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterMenu).name()) == 0) { // ParameterMenu
    drawMenu(dynamic_cast<ParameterMenu *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterChoice).name()) == 0) { // ParameterChoice
    drawChoice(dynamic_cast<ParameterChoice *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterVec3).name()) == 0) { // ParameterVec3
    drawVec3(dynamic_cast<ParameterVec3 *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterVec4).name()) == 0) { // ParameterVec4
    drawVec4(dynamic_cast<ParameterVec4 *>(&param), suffix);
  } else if (strcmp(typeid(param).name(),
                    typeid(ParameterColor).name()) == 0) { // ParameterColor
    drawParameterColor(dynamic_cast<ParameterColor *>(&param), suffix);
  } else if (strcmp(typeid(param).name(), typeid(Trigger).name()) ==
             0) { // ParameterColor
    drawTrigger(dynamic_cast<Trigger *>(&param), suffix);
  } else {
    // TODO this check should be performed on registration
    std::cout << "Unsupported Parameter type for display" << std::endl;
  }
}

void ParameterGUI::drawParameter(std::vector<Parameter *> params, string suffix,
                                 int index) {
  if (params.size() == 0 || index >= (int)params.size())
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0 || (param->min() > param->max()))
    return;
  float value = param->get();
  bool changed;
  bool isSpinBox = false;
  auto spinDecimals = param->getHint("input", &isSpinBox);
  if (isSpinBox) {
    std::string format = "%." + std::to_string(int(spinDecimals)) + "f";
    changed = ImGui::InputFloat((param->displayName() + suffix).c_str(), &value,
                                pow(10, -spinDecimals),
                                pow(10, -(spinDecimals - 1.0)), format.c_str(),
                                ImGuiInputTextFlags_EnterReturnsTrue);
  } else {
    changed = ImGui::SliderFloat((param->displayName() + suffix).c_str(),
                                 &value, param->min(), param->max());
  }
  if (changed) {
    for (auto *p : params) {
      p->set(value);
    }
  }
}

void ParameterGUI::drawParameterString(std::vector<ParameterString *> params,
                                       string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  ImGui::Text("%s", (param->displayName() + ":").c_str());
  ImGui::SameLine();
  ImGui::Text("%s", (param->get()).c_str());
}

void ParameterGUI::drawParameterInt(std::vector<ParameterInt *> params,
                                    string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0 || (param->min() > param->max()))
    return;
  int value = param->get();
  bool changed = ImGui::SliderInt((param->displayName() + suffix).c_str(),
                                  &value, param->min(), param->max());
  if (changed) {
    for (auto *p : params) {
      p->set(value);
    }
  }
}

void ParameterGUI::drawParameterBool(std::vector<ParameterBool *> params,
                                     string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  bool changed;
  //    if (param->getHint("latch") == 1.0) {
  bool value = param->get() == 1.0;
  changed = ImGui::Checkbox((param->displayName() + suffix).c_str(), &value);
  if (changed) {
    param->set(value ? 1.0 : 0.0);
  }
}

void ParameterGUI::drawParameterPose(std::vector<ParameterPose *> params,
                                     std::string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &pose = params[index];
  if (pose->getHint("hide") == 1.0)
    return;
  if (ImGui::CollapsingHeader(("Pose:" + pose->displayName()).c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader)) {
    Vec3d currentPos = pose->get().pos();
    Quatd currQuat = pose->get().quat();
    float x = currentPos.x;
    if (ImGui::SliderFloat(("X" + suffix + pose->displayName()).c_str(), &x, -5,
                           5)) {
      currentPos.x = x;
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }
    float y = currentPos.y;
    if (ImGui::SliderFloat(("Y" + suffix + pose->displayName()).c_str(), &y, -5,
                           5)) {
      currentPos.y = y;
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }
    float z = currentPos.z;
    if (ImGui::SliderFloat(("Z" + suffix + pose->displayName()).c_str(), &z,
                           -10, 0)) {
      currentPos.z = z;
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }
    ImGui::Text("Quaternion");
    float w = currQuat.w;
    if (ImGui::SliderFloat(("w" + suffix + pose->displayName()).c_str(), &w, 0,
                           1)) {
      currQuat.w = w;
      currQuat.normalize();
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }

    x = currQuat.x;
    if (ImGui::SliderFloat(("x" + suffix + pose->displayName()).c_str(), &x, 0,
                           1)) {
      currQuat.x = x;
      currQuat.normalize();
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }

    y = currQuat.y;
    if (ImGui::SliderFloat(("y" + suffix + pose->displayName()).c_str(), &y, 0,
                           1)) {
      currQuat.y = y;
      currQuat.normalize();
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }

    z = currQuat.z;
    if (ImGui::SliderFloat(("z" + suffix + pose->displayName()).c_str(), &z, 0,
                           1)) {
      currQuat.z = z;
      currQuat.normalize();
      for (auto *p : params) {
        p->set(Pose(currentPos, currQuat));
      }
    }

    ImGui::Spacing();
  }
}

void ParameterGUI::drawParameterColor(std::vector<ParameterColor *> params,
                                      string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  Color c = param->get();
  ImVec4 color = ImColor(c.r, c.g, c.b, c.a);

  static bool alpha_preview = true;
  static bool alpha_half_preview = false;
  static bool drag_and_drop = true;
  static bool options_menu = true;
  static bool hdr = false;

  bool showAlpha = param->getHint("showAlpha");
  bool showHsv = param->getHint("hsv");

  int misc_flags =
      (!showAlpha ? ImGuiColorEditFlags_NoAlpha : 0) |
      (hdr ? ImGuiColorEditFlags_HDR : 0) |
      (drag_and_drop ? 0 : ImGuiColorEditFlags_NoDragDrop) |
      (alpha_half_preview
           ? ImGuiColorEditFlags_AlphaPreviewHalf
           : (alpha_preview ? ImGuiColorEditFlags_AlphaPreview : 0)) |
      (options_menu ? 0 : ImGuiColorEditFlags_NoOptions) |
      (!showHsv ? 0 : ImGuiColorEditFlags_PickerHueWheel);

  //    ImGui::Text("Color widget HSV with Alpha:");
  if (ImGui::ColorEdit4((param->displayName() + suffix).c_str(),
                        (float *)&color, misc_flags)) {
    c.r = color.x;
    c.g = color.y;
    c.b = color.z;
    c.a = color.w;
    for (auto *p : params) {
      p->set(c);
    }
  }
}

void ParameterGUI::drawMenu(std::vector<ParameterMenu *> params, string suffix,
                            int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  int value = param->get();
  auto values = param->getElements();
  bool changed =
      ImGui::Combo((param->displayName() + suffix).c_str(), &value,
                   vector_getter, static_cast<void *>(&values), values.size());
  if (changed) {
    for (auto *p : params) {
      p->set(value);
    }
  }
}

void ParameterGUI::drawChoice(std::vector<ParameterChoice *> params,
                              string suffix, int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  uint64_t value = param->get();
  auto elements = param->getElements();
  if (ImGui::CollapsingHeader((param->displayName() + suffix).c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    for (unsigned int i = 0; i < elements.size(); i++) {
      bool state = value & (1 << i);
      ImGui::PushID((const void *)param);
      if (ImGui::Checkbox(
              (elements[i] + suffix + "_" + std::to_string(i)).c_str(),
              &state)) {
        value ^=
            ((state ? -1 : 0) ^ value) & (1UL << i); // Set an individual bit
        for (auto *p : params) {
          p->set(value);
        }
      }
      ImGui::PopID();
    }
  }
}

void ParameterGUI::drawVec3(std::vector<ParameterVec3 *> params, string suffix,
                            int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  if (ImGui::CollapsingHeader((param->displayName() + suffix).c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    Vec3f currentValue = param->get();
    float x = currentValue.elems()[0];
    bool updated = false;
    float max = 10;
    float min = -10;
    bool exists;
    float value;
    value = param->getHint("maxx", &exists);
    if (exists) {
      max = value;
    }
    value = param->getHint("minx", &exists);
    if (exists) {
      min = value;
    }
    bool changed = ImGui::SliderFloat(("X" + suffix + param->getName()).c_str(),
                                      &x, min, max);
    if (changed) {
      currentValue.x = x;
      updated = true;
    }
    float y = currentValue.elems()[1];
    max = 10;
    min = -10;
    value = param->getHint("maxy", &exists);
    if (exists) {
      max = value;
    }
    value = param->getHint("miny", &exists);
    if (exists) {
      min = value;
    }
    changed = ImGui::SliderFloat(("Y" + suffix + param->getName()).c_str(), &y,
                                 min, max);
    if (changed) {
      currentValue.y = y;
      updated = true;
    }
    float z = currentValue.elems()[2];
    max = 10;
    min = -10;
    value = param->getHint("maxz", &exists);
    if (exists) {
      max = value;
    }
    value = param->getHint("minz", &exists);
    if (exists) {
      min = value;
    }
    changed = ImGui::SliderFloat(("Z" + suffix + param->getName()).c_str(), &z,
                                 min, max);
    if (changed) {
      currentValue.z = z;
      updated = true;
    }
    if (updated) {
      for (auto *p : params) {
        p->set(currentValue);
      }
    }
  }
}

void ParameterGUI::drawVec4(std::vector<ParameterVec4 *> params, string suffix,
                            int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  if (ImGui::CollapsingHeader((param->displayName() + suffix).c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    Vec4f currentValue = param->get();
    float x = currentValue.elems()[0];
    bool updated = false;
    bool changed = ImGui::SliderFloat(("X" + suffix + param->getName()).c_str(),
                                      &x, -10, 10);
    if (changed) {
      currentValue.x = x;
      updated = true;
    }
    float y = currentValue.elems()[1];
    changed = ImGui::SliderFloat(("Y" + suffix + param->getName()).c_str(), &y,
                                 -10, 10);
    if (changed) {
      currentValue.y = y;
      updated = true;
    }
    float z = currentValue.elems()[2];
    changed = ImGui::SliderFloat(("Z" + suffix + param->getName()).c_str(), &z,
                                 -10, 10);
    if (changed) {
      currentValue.z = z;
      updated = true;
    }
    if (updated) {
      param->set(currentValue);
    }
    float w = currentValue.elems()[3];
    changed = ImGui::SliderFloat(("W" + suffix + param->getName()).c_str(), &z,
                                 -10, 10);
    if (changed) {
      currentValue.w = w;
      updated = true;
    }
    if (updated) {
      for (auto *p : params) {
        p->set(currentValue);
      }
    }
  }
}

void ParameterGUI::drawTrigger(std::vector<Trigger *> params, string suffix,
                               int index) {
  if (params.size() == 0)
    return;
  assert(index < (int)params.size());
  auto &param = params[index];
  if (param->getHint("hide") == 1.0)
    return;
  bool changed;
  changed = ImGui::Button((param->displayName() + suffix).c_str());
  if (changed) {
    for (auto *p : params) {
      p->set(true);
    }
  }
}

void ParameterGUI::drawNav(Nav *mNav, std::string suffix) {
  if (ImGui::CollapsingHeader(("Navigation##nav" + suffix).c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    Vec3d &currentPos = mNav->pos();
    float x = currentPos.elems()[0];

    bool changed =
        ImGui::SliderFloat(("X##__nav_" + suffix).c_str(), &x, -10, 10);
    if (changed) {
      currentPos.elems()[0] = x;
    }
    float y = currentPos.elems()[1];
    changed = ImGui::SliderFloat(("Y##__nav_" + suffix).c_str(), &y, -10, 10);
    if (changed) {
      currentPos.elems()[1] = y;
    }
    float z = currentPos.elems()[2];
    changed = ImGui::SliderFloat(("Z##__nav_" + suffix).c_str(), &z, -10, 10);
    if (changed) {
      currentPos.elems()[2] = z;
    }
    ImGui::Spacing();
  }
}

void ParameterGUI::drawDynamicScene(DynamicScene *scene, std::string suffix) {}

ParameterGUI::PresetHandlerState &
ParameterGUI::drawPresetHandler(PresetHandler *presetHandler, int presetColumns,
                                int presetRows) {
  static std::map<PresetHandler *, PresetHandlerState> stateMap;
  if (stateMap.find(presetHandler) == stateMap.end()) {
    //        std::cout << "Created state for " << (unsigned long)
    //        presetHandler
    //        << std::endl;
    stateMap[presetHandler] =
        PresetHandlerState{"", 0, presetHandler->availablePresetMaps()};
    if (stateMap[presetHandler].mapList.size() > 0) {
      stateMap[presetHandler].currentBank = stateMap[presetHandler].mapList[0];
      stateMap[presetHandler].currentBankIndex = 0;
    }
    presetHandler->registerPresetMapCallback([&](std::string mapName) {
      stateMap[presetHandler].currentBank = mapName;
    });
  }
  PresetHandlerState &state = stateMap[presetHandler];
  float fontSize = ImGui::GetFontSize();

  std::string id = std::to_string((uint64_t)presetHandler);
  std::string suffix = "##PresetHandler" + id;
  ImGui::PushID(suffix.c_str());

  if (ImGui::CollapsingHeader(
          ("Presets " + presetHandler->getCurrentPath()).c_str(),
          ImGuiTreeNodeFlags_CollapsingHeader |
              ImGuiTreeNodeFlags_DefaultOpen)) {
    int selection = presetHandler->getCurrentPresetIndex();
    std::string currentPresetName = presetHandler->getCurrentPresetName();
    size_t delim_index = 0;
    if (currentPresetName.length() == 0)
      currentPresetName = "none";
    else
      delim_index = (state.currentBank + "-").size();

    if (currentPresetName.size() > delim_index &&
        currentPresetName.substr(0, delim_index) == state.currentBank + "-")
      ImGui::Text("Current Preset: %s",
                  currentPresetName.substr(delim_index).c_str());
    else
      ImGui::Text("Current Preset: %s", currentPresetName.c_str());
    int counter = state.presetHandlerBank * (presetColumns * presetRows);
    if (state.storeButtonState) {
      ImGui::PushStyleColor(ImGuiCol_Text, 0xff0000ff);
    }
    float presetWidth = (ImGui::GetContentRegionAvail().x / 12.0f) - 8.0f;
    for (int row = 0; row < presetRows; row++) {
      for (int column = 0; column < presetColumns; column++) {
        std::string name = std::to_string(counter);
        ImGui::PushID(counter);

        bool is_selected = selection == counter;
        if (is_selected) {
          ImGui::PushStyleColor(ImGuiCol_Border,
                                ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        }

        const bool selectableClicked =
            ImGui::Selectable(name.c_str(), is_selected, 0,
                              ImVec2(presetWidth, ImGui::GetFontSize() * 1.2f));
        if (selectableClicked) {

          if (state.storeButtonState) {
            std::string saveName = state.enteredText;
            if (saveName.size() == 0) {
              saveName = name;
            }
            presetHandler->storePreset(counter, saveName.c_str());
            selection = counter;
            state.storeButtonState = false;
            ImGui::PopStyleColor();
            state.enteredText.clear();
          } else {
            if (presetHandler->recallPreset(counter) !=
                "") { // Preset is available
              selection = counter;
            }
          }
        }

        if (ImGui::IsItemHovered()) { // tooltip showing preset name
          const std::string currentlyhoveringPresetName =
              presetHandler->getPresetName(counter);
          if (currentlyhoveringPresetName.size() > 0) {
            ImGui::SetTooltip("%s", currentlyhoveringPresetName.c_str());
          }
        }
        if (is_selected) {
          ImGui::PopStyleColor(1);
        }
        if (column < presetColumns - 1)
          ImGui::SameLine();
        counter++;
        ImGui::PopID();
      }
    }
    if (state.storeButtonState) {
      ImGui::PopStyleColor();
    }
    if (ImGui::Button("<-")) {
      state.presetHandlerBank -= 1;
      if (state.presetHandlerBank < 0) {
        state.presetHandlerBank = 4;
      }
    }
    ImGui::SameLine();
    if (ImGui::Button("->")) {
      state.presetHandlerBank += 1;
      if (state.presetHandlerBank > 4) {
        state.presetHandlerBank = 0;
      }
    }
    ImGui::SameLine(0.0f, 40.0f);

    if (state.storeButtonState) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 0.0, 0.0, 1.0));
    }
    std::string storeText = state.storeButtonState ? "Cancel" : "Store";
    bool storeButtonPressed = ImGui::Button(storeText.c_str(), ImVec2(100, 0));
    if (state.storeButtonState) {
      ImGui::PopStyleColor();
    }
    if (storeButtonPressed) {
      state.storeButtonState = !state.storeButtonState;
      //          if (state.storeButtonState) {
      //            state.enteredText = currentPresetName;
      //          }
    }
    if (state.storeButtonState) {
      char buf1[64];
      strncpy(buf1, state.enteredText.c_str(), 63);
      ImGui::Text("Store preset as:");
      ImGui::SameLine();
      if (ImGui::InputText("preset", buf1, 64)) {
        state.enteredText = buf1;
      }
      ImGui::Text("Click on a preset number to store.");
    } else {
      //          ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.65f);
      if (ImGui::BeginCombo("Preset Map", state.currentBank.data())) {
        stateMap[presetHandler].mapList = presetHandler->availablePresetMaps();
        for (auto mapName : stateMap[presetHandler].mapList) {
          bool isSelected = (state.currentBank == mapName);
          if (ImGui::Selectable(mapName.data(), isSelected)) {
            state.currentBank = mapName;
            presetHandler->setCurrentPresetMap(mapName);
          }
          if (isSelected) {
            ImGui::SetItemDefaultFocus();
          }
        }
        ImGui::EndCombo();
      }
      if (!state.newMap) {
        ImGui::SameLine();
        if (ImGui::Button("+")) {
          state.newMap = true;
        }
      } else {
        char buf2[64];
        strncpy(buf2, state.newMapText.c_str(), 63);
        ImGui::Text("New map:");
        //              ImGui::SameLine();
        ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
        if (ImGui::InputText("", buf2, 64)) {
          state.newMapText = buf2;
        }
        ImGui::PopItemWidth();
        ImGui::SameLine();
        if (ImGui::Button("Create")) {
          auto path = File::conformDirectory(presetHandler->getCurrentPath()) +
                      state.newMapText + ".presetMap";
          // Create an empty file
          ofstream file;
          file.open(path, ios::out);
          file.close();
          state.newMap = false;

          stateMap[presetHandler].mapList =
              presetHandler->availablePresetMaps();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel")) {
          state.newMapText = "";
          state.newMap = false;
        }
      }
      // TODO options to create new bank
      //        ImGui::SameLine();
      //          ImGui::PopItemWidth();
      ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.2f);
      float morphTime = presetHandler->getMorphTime();
      if (ImGui::InputFloat("morph time", &morphTime, 0.0f, 20.0f)) {
        presetHandler->setMorphTime(morphTime);
      }
      ImGui::PopItemWidth();
    }

    //            ImGui::Text("%s", currentPresetName.c_str());
  }
  ImGui::PopID();
  return state;
}

void ParameterGUI::drawPresetSequencer(PresetSequencer *presetSequencer,
                                       int &currentPresetSequencerItem) {
  struct SequencerState {
    float currentTime;
    float totalDuration;
    float startOffset = 0.0f;
    std::vector<std::string> seqList;
  };
  static std::map<PresetSequencer *, SequencerState> stateMap;
  if (stateMap.find(presetSequencer) == stateMap.end()) {
    stateMap[presetSequencer] = SequencerState{0.0, 0.0, {}};
    float *currentTime = &(stateMap[presetSequencer].currentTime);
    presetSequencer->registerTimeChangeCallback(
        [currentTime](float currTime) { *currentTime = currTime; }, 0.05f);
    presetSequencer->registerBeginCallback([&](PresetSequencer *sender) {
      stateMap[presetSequencer].totalDuration =
          sender->getSequenceTotalDuration(sender->currentSequence());
    });
    stateMap[presetSequencer].seqList = presetSequencer->getSequenceList();
    if (stateMap[presetSequencer].seqList.size() > 64) {
      stateMap[presetSequencer].seqList.resize(64);
      std::cout << "Cropping sequence list to 64 items for display"
                << std::endl;
    }
    if (currentPresetSequencerItem >= 0 &&
        currentPresetSequencerItem <
            (int)stateMap[presetSequencer].seqList.size()) {
      std::cout << stateMap[presetSequencer].seqList[currentPresetSequencerItem]
                << std::endl;
      presetSequencer->loadSequence(
          stateMap[presetSequencer].seqList[currentPresetSequencerItem]);

      stateMap[presetSequencer].startOffset =
          presetSequencer->getSequenceStartOffset(
              stateMap[presetSequencer].seqList[currentPresetSequencerItem]);
      stateMap[presetSequencer].totalDuration =
          presetSequencer->getSequenceTotalDuration(
              stateMap[presetSequencer].seqList[currentPresetSequencerItem]);
    }
  }
  SequencerState &state = stateMap[presetSequencer];
  std::string id = std::to_string((uint64_t)presetSequencer);
  std::string suffix = "##PresetSequencer" + id;
  ImGui::PushID(suffix.c_str());
  if (ImGui::CollapsingHeader("Preset Sequencer",
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    if (stateMap[presetSequencer].seqList.size() > 0) {
      if (ImGui::Combo("Sequences", &currentPresetSequencerItem,
                       ParameterGUI::vector_getter,
                       static_cast<void *>(&stateMap[presetSequencer].seqList),
                       stateMap[presetSequencer].seqList.size())) {
        std::cout << currentPresetSequencerItem << std::endl;
        stateMap[presetSequencer].seqList = presetSequencer->getSequenceList();
        if (currentPresetSequencerItem >= 0 &&
            currentPresetSequencerItem <
                (int)stateMap[presetSequencer].seqList.size()) {
          presetSequencer->loadSequence(
              stateMap[presetSequencer].seqList[currentPresetSequencerItem]);
          state.startOffset = presetSequencer->getSequenceStartOffset(
              stateMap[presetSequencer].seqList[currentPresetSequencerItem]);
          state.totalDuration = presetSequencer->getSequenceTotalDuration(
              stateMap[presetSequencer].seqList[currentPresetSequencerItem]);
        }
        //                state.totalDuration =
        //                presetSequencer->getSequenceTotalDuration(seqList[currentPresetSequencerItem]);
      }
      if (ImGui::Button("Play")) {
        if (!presetSequencer->running()) {
          presetSequencer->stopSequence();
          if (presetSequencer->playbackFinished()) {
            if (state.currentTime != 0) {
              presetSequencer->setTime(-state.startOffset);
            }
          }
          if (currentPresetSequencerItem >= 0) {
            double sliderTime = state.currentTime;
            presetSequencer->playSequence(
                stateMap[presetSequencer].seqList[currentPresetSequencerItem],
                1.0, sliderTime - state.startOffset);
          } else {
            std::cout << "No sequence selected for playback." << std::endl;
          }
        }
      }
      ImGui::SameLine();
      if (ImGui::Button("Pause")) {
        presetSequencer->stopSequence(false);
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        presetSequencer->stopSequence();
        presetSequencer->setTime(stateMap[presetSequencer].totalDuration);
      }
      float time = state.currentTime;
      //        std::cout << time << std::endl;
      if (ImGui::SliderFloat("Position", &time, -state.startOffset,
                             state.totalDuration - state.startOffset)) {
        //        std::cout << "Requested time:" << time << std::endl;
        presetSequencer->setTime(time);
      }
    } else {
      ImGui::Text("No sequences found.");
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawSequenceRecorder(SequenceRecorder *sequenceRecorder) {
  struct SequenceRecorderState {
    bool overwriteButtonValue{false};
  };
  static std::map<SequenceRecorder *, SequenceRecorderState> stateMap;
  if (stateMap.find(sequenceRecorder) == stateMap.end()) {
    stateMap[sequenceRecorder] = SequenceRecorderState{0};
  }
  SequenceRecorderState &state = stateMap[sequenceRecorder];
  if (ImGui::CollapsingHeader("Preset Sequence Recorder##__SequenceRecorder",
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    static char buf_seq_recorder[64] = "test";
    ImGui::InputText("Record Name##__SequenceRecorder", buf_seq_recorder, 64);
    static bool button_seq_recorder_value = false;
    if (button_seq_recorder_value) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 0.0, 0.0, 1.0));
    }
    std::string buttonText = button_seq_recorder_value
                                 ? "Stop##__SequenceRecorder"
                                 : "Record##__SequenceRecorder";
    bool recordButtonClicked = ImGui::Button(buttonText.c_str());
    if (button_seq_recorder_value) {
      ImGui::PopStyleColor();
    }
    if (recordButtonClicked) {
      button_seq_recorder_value = !button_seq_recorder_value;
      if (button_seq_recorder_value) {
        sequenceRecorder->startRecord(buf_seq_recorder,
                                      state.overwriteButtonValue);
      } else {
        sequenceRecorder->stopRecord();
      }
    }
    // ImGui::SameLine();
    ImGui::Checkbox("Overwrite##__SequenceRecorder",
                    &state.overwriteButtonValue);
    ImGui::SameLine();
    ImGui::Checkbox("Quantize##__SequenceRecorder",
                    &state.overwriteButtonValue);
  }
}

// NEW
void ParameterGUI::drawSynthSequencer(std::vector<SynthSequencer*> sequencerList) {
    std::string headerLabel = "Multi Sequencer";
    if (ImGui::CollapsingHeader(headerLabel.c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
        for(int i = 0; i < sequencerList.size(); i++){
            SynthSequencer* synthSequencer = sequencerList[i];
            struct SynthSequencerState {
                int currentItem;
                float totalDuration{0.0f};
                float currentTime{0.0f};
                bool newSequence{false};
                std::string loadedSequence;
            };
            static std::map<SynthSequencer *, SynthSequencerState> stateMap;
            if (stateMap.find(synthSequencer) == stateMap.end()) {
                stateMap[synthSequencer] = SynthSequencerState{0};
                float *currentTime = &(stateMap[synthSequencer].currentTime);
                auto *state = &(stateMap[synthSequencer]);
                synthSequencer->registerTimeChangeCallback(
                    [currentTime](float currTime) { *currentTime = currTime; }, 0.1f);
                synthSequencer->registerSequenceBeginCallback(
                    [state](std::string sequenceName) {
                    state->loadedSequence = sequenceName;
                    state->newSequence = true; // poor man's mutex... perhaps should eb
                                                // changed to a real mutex...
                    });
            }
            SynthSequencerState &state = stateMap[synthSequencer];

            std::vector<std::string> seqList = synthSequencer->getSequenceList();
            char arr[seqList[i].length() + 1];
            std::strcpy(arr, seqList[i].c_str());
            if(ImGui::Button(arr)){
                state.currentItem = i;
                state.newSequence = false;
                state.totalDuration =
                synthSequencer->getSequenceDuration(seqList[state.currentItem]);
                synthSequencer->playSequence(seqList[i]);
                synthSequencer->stopSequence();
                while (synthSequencer->synth().getActiveVoices()) {
                synthSequencer->synth().allNotesOff();
                al_sleep(0.05);
                }
                state.totalDuration =
                    synthSequencer->getSequenceDuration(seqList[state.currentItem]);
                synthSequencer->playSequence(seqList[state.currentItem]);
            }
        }

        // std::string id = std::to_string((uint64_t)synthSequencer);
        // std::string suffix = "##EventSequencer" + id;
        // ImGui::PushID(suffix.c_str());
        // std::string headerLabel = "Event Sequencer";
        // if (ImGui::CollapsingHeader(headerLabel.c_str(),
        //                             ImGuiTreeNodeFlags_CollapsingHeader |
        //                                 ImGuiTreeNodeFlags_DefaultOpen)) {
        //     std::vector<std::string> seqList = synthSequencer->getSequenceList();
        //     if (state.newSequence) {
        //     ptrdiff_t pos =
        //         find(seqList.begin(), seqList.end(), state.loadedSequence) -
        //         seqList.begin();
        //     if (size_t(pos) < seqList.size()) {
        //         state.currentItem = pos;
        //         state.newSequence = false;
        //         state.totalDuration =
        //             synthSequencer->getSequenceDuration(seqList[state.currentItem]);
        //     }
        //     state.newSequence = false;
        //     }
        //     // TODO we should only refresh occasionally or perhaps reactively.
        //     if (seqList.size() > 0) {
        //     if (seqList.size() > 64) {
        //         seqList.resize(64);
        //         std::cout << "Cropping sequence list to 64 items for display"
        //                 << std::endl;
        //     }
        //     //   for(int i = 0; i < 10; i++){
        //     //       char arr[seqList[i].length() + 1];
        //     //       strcpy(arr, seqList[i].c_str());
        //     //       if(ImGui::Button(arr)){
        //     //           synthSequencer->playSequence(seqList[i]);
        //     //       }
        //     //   }
        //     if (ImGui::Combo("Sequences", &state.currentItem,
        //                     ParameterGUI::vector_getter,
        //                     static_cast<void *>(&seqList), seqList.size())) {
        //         state.totalDuration =
        //             synthSequencer->getSequenceDuration(seqList[state.currentItem]);
        //     }
        //     if (ImGui::Button("Play")) {
        //         synthSequencer->stopSequence();
        //         while (synthSequencer->synth().getActiveVoices()) {
        //         synthSequencer->synth().allNotesOff();
        //         al_sleep(0.05);
        //         }
        //         state.totalDuration =
        //             synthSequencer->getSequenceDuration(seqList[state.currentItem]);
        //         synthSequencer->playSequence(seqList[state.currentItem]);
        //     }
        //     ImGui::SameLine();
        //     if (ImGui::Button("Stop")) {
        //         synthSequencer->synth().allNotesOff();
        //         synthSequencer->stopSequence();
        //     }
        //     //            static float time = state.currentTime;
        //     if (ImGui::SliderFloat("Position", &state.currentTime, 0.0f,
        //                             state.totalDuration)) {
        //         std::cout << "Requested time:" << state.currentTime << std::endl;
        //         synthSequencer->setTime(state.currentTime);
        //     }
        //     } else {
        //     ImGui::Text("No sequences found.");
        //     }
        // }
        ImGui::PopID();
    }
}

// NEW
void ParameterGUI::drawMultiSynthSequencer(SynthSequencer *synthSequencer, SynthSequencer *synthSequencer2) {
    al::ParameterGUI::drawSynthSequencer(synthSequencer);
    al::ParameterGUI::drawSynthSequencer(synthSequencer2);
    // std::string headerLabel = "Multi Sequencer";
    // if (ImGui::CollapsingHeader(headerLabel.c_str(),
    //                           ImGuiTreeNodeFlags_CollapsingHeader |
    //                               ImGuiTreeNodeFlags_DefaultOpen)) {
    //     if(ImGui::Button())
    // }
}

bool comp(SynthSequencerEvent first, SynthSequencerEvent second){
    return first.startTime < second.startTime;
}

void ParameterGUI::drawSynthSequencer(SynthSequencer *synthSequencer) {
  struct SynthSequencerState {
    int currentItem;
    float totalDuration{0.0f};
    float currentTime{0.0f};
    bool newSequence{false};
    std::string loadedSequence;
    bool adjustTempo;
    std::string include;
    std::vector<int> currentSelectorItem{0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
    std::vector<std::string> selectorList{"", "", "", "", "", "", "", "", "", ""};
    std::list<SynthSequencerEvent> currEvents;
  };
  static std::map<SynthSequencer *, SynthSequencerState> stateMap;
  if (stateMap.find(synthSequencer) == stateMap.end()) {
    stateMap[synthSequencer] = SynthSequencerState{0};
    float *currentTime = &(stateMap[synthSequencer].currentTime);
    auto *state = &(stateMap[synthSequencer]);
    synthSequencer->registerTimeChangeCallback(
        [currentTime](float currTime) { *currentTime = currTime; }, 0.1f);
    synthSequencer->registerSequenceBeginCallback(
        [state](std::string sequenceName) {
          if(sequenceName != "temp"){state->loadedSequence = sequenceName;};
          state->newSequence = true; // poor man's mutex... perhaps should eb
                                     // changed to a real mutex...
        });
  }
  SynthSequencerState &state = stateMap[synthSequencer];

  std::string id = std::to_string((uint64_t)synthSequencer);
  std::string suffix = "##EventSequencer" + id;
  ImGui::PushID(suffix.c_str());
  std::string headerLabel = "Event Sequencer";
  if (ImGui::CollapsingHeader(headerLabel.c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    std::vector<std::string> seqList = synthSequencer->getSequenceList();
    if (state.newSequence) {
      ptrdiff_t pos =
          find(seqList.begin(), seqList.end(), state.loadedSequence) -
          seqList.begin();
      if (size_t(pos) < seqList.size() && seqList[state.currentItem] != "temp") {
        std::cout << "setting currentItem to " << seqList[state.currentItem] << std::endl;
        state.currentItem = pos;
        state.newSequence = false;
        state.totalDuration =
            synthSequencer->getSequenceDuration(seqList[state.currentItem]);
      }
      state.newSequence = false;
    }
    // TODO we should only refresh occasionally or perhaps reactively.
    if (seqList.size() > 0) {
      if (seqList.size() > 64) {
        seqList.resize(64);
        std::cout << "Cropping sequence list to 64 items for display"
                  << std::endl;
      }
      ImGui::PushItemWidth(128);
      // for(int i = 0; i < 10; i++){
      //   // char arr[3];
      //   // strcpy(arr, std::to_string(i).c_str());
      //   std::string label = "button " + std::to_string(i);
      //   char const* pchar = label.c_str();
      //   if(ImGui::Combo(pchar, &state.currentSelectorItem[i],
      //                  ParameterGUI::vector_getter,
      //                  static_cast<void *>(&seqList), seqList.size())){
      //     std::cout << "currentSelectorItem: " << state.currentSelectorItem[i] << std::endl;
      //     state.selectorList[i] = seqList[state.currentSelectorItem[i]];
      //     std::cout << "from first: " << state.selectorList[i] << std::endl;
      //   }
      //   // ImGui::SameLine();

      //   // if (ImGui::Combo("Sequences", &state.currentItem,
      //   //                 ParameterGUI::vector_getter,
      //   //                 static_cast<void *>(&seqList), seqList.size())) {
      //   //   state.totalDuration =
      //   //       synthSequencer->getSequenceDuration(seqList[state.currentItem]);
      //   // }
      // }
      for(int i = 0; i < 10; i++){
        std::string label = "Sequence " + std::to_string(i);
        if(ImGui::Combo(label.c_str(), &state.currentSelectorItem[i],
                       ParameterGUI::vector_getter,
                       static_cast<void *>(&seqList), seqList.size())){
          std::cout << "currentSelectorItem: " << state.currentSelectorItem[i] << std::endl;
          state.selectorList[i] = seqList[state.currentSelectorItem[i]];
          std::cout << "from first: " << state.selectorList[i] << std::endl;
        }
        ImGui::SameLine();
        // char arr[seqList[i].length() + 1];
        // strcpy(arr, seqList[i].c_str());
        // for keyboard playback
        // synthSequencer->sequenceList.push_back(seqList[i].c_str());
        // if(i % 5 != 0){
        //   ImGui::SameLine();
        // }
        // if(ImGui::Button(arr)){
        label = "Play " + std::to_string(i);
        if(ImGui::Button(label.c_str())){
          std::cout << "from second: " << state.selectorList[i] << std::endl;
            // synthSequencer->playSequence(seqList[i]);
            // synthSequencer->synth().registerSynthClass<Sub>(synthSequencer->buildFullPath(seqList[i]));
          // state.include += "= " + to_string(state.currentTime) + " \"" + seqList[i] + ".synthSequence\" 1\n";
          // state.include += "= " + to_string(state.currentTime) + " \"" + state.selectorList[i] + ".synthSequence\" 1\n";
          // std::cout << state.include << std::endl;
          synthSequencer->stopSequence();
          while (synthSequencer->synth().getActiveVoices()) {
            synthSequencer->synth().allNotesOff();
            al_sleep(0.05);
          }

          std::list<SynthSequencerEvent> events = synthSequencer->loadSequence(seqList[i], 0, 1);
          for(auto it = state.currEvents.begin(); it != state.currEvents.end(); it++){
            it->startTime -= (state.currentTime + 0.05);
          }
          state.currEvents.merge(events, comp);
          synthSequencer->playEvents(state.currEvents);

          // auto *voice = synthSequencer->synth().getVoice<Sub>();
          // auto *voice = synthSequencer->synth().getVoice("Sub");
          // synthSequencer->addVoice(voice, state.currentTime, 0.2);
          // synthSequencer->synth().triggerOn(voice, 0);

          // synthSequencer->stopSequence();
          // while (synthSequencer->synth().getActiveVoices()) {
          //   synthSequencer->synth().allNotesOff();
          //   al_sleep(0.05);
          // }
          // // build a temp string and add specified settings
          // std::string name = seqList[state.currentItem];
          // std::cout << state.include << std::endl;
          // std::string currStr = synthSequencer->buildFullPath(seqList[state.currentItem]);
          // std::cout << currStr << std::endl;
          // std::string tempStr = synthSequencer->buildFullPath("temp");
          // std::ifstream src(currStr, std::ios::binary);
          // std::ofstream tempFile(tempStr, std::ios::binary);
          // // std::cout << "include: " << state.include << std::endl;
          // tempFile << state.include.c_str() << "\n";
          // // tempFile << "t " << tempoBuf << "\n";
          // tempFile << src.rdbuf();
          // tempFile.close();
          // name = "temp";

          // // play temp file instead of selected
          // // std::cout << currStr << std::endl;
          // // std::cout << seqList[state.currentItem] << std::endl;
          // // std::cout << tempStr << std::endl;
          // state.totalDuration =
          //     synthSequencer->getSequenceDuration(name);
          // synthSequencer->playSequence(name, state.currentTime + 0.2);
        }

        // if(ImGui::Combo(std::to_string(i).c_str(), &state.currentItem, ParameterGUI::vector_getter,
        //                static_cast<void *>(&seqList), seqList.size())) {
          
        // }
      }
      ImGui::PopItemWidth();
      if (ImGui::Combo("Sequences", &state.currentItem,
                       ParameterGUI::vector_getter,
                       static_cast<void *>(&seqList), seqList.size())) {
        state.totalDuration =
            synthSequencer->getSequenceDuration(seqList[state.currentItem]);
      }

        // adjust tempo
        ImGui::Checkbox("Adjust Tempo##__EventRecorder", &state.adjustTempo);
        ImGui::PushItemWidth(32);
        ImGui::SameLine();
        static char tempoBuf[16] = "120";
        ImGui::InputText("bpm##__EventRecorder", tempoBuf, 16);
        ImGui::PopItemWidth();

      if (ImGui::Button("Play")) {
        synthSequencer->stopSequence();
        while (synthSequencer->synth().getActiveVoices()) {
          synthSequencer->synth().allNotesOff();
          al_sleep(0.05);
        }
        // build a temp string and add specified settings
        std::string name = seqList[state.currentItem];
        std::cout << state.include << std::endl;
        std::string currStr = synthSequencer->buildFullPath(seqList[state.currentItem]);
        std::cout << currStr << std::endl;
        std::string tempStr = synthSequencer->buildFullPath("temp");
        std::ifstream src(currStr, std::ios::binary);
        std::ofstream tempFile(tempStr, std::ios::binary);

        if(state.adjustTempo){
            // std::string currStr = synthSequencer->buildFullPath(seqList[state.currentItem]);
            // std::string tempStr = synthSequencer->buildFullPath("temp");
            // std::ifstream src(currStr, std::ios::binary);
            // std::ofstream tempFile(tempStr, std::ios::binary);
            // std::cout << "include: " << state.include << std::endl;
            // tempFile << state.include.c_str() << "\n";
            tempFile << "t " << tempoBuf << "\n";
            // tempFile << src.rdbuf();
            // tempFile.close();
            // name = "temp";
        }
        // std::cout << "include: " << state.include << std::endl;
        tempFile << state.include.c_str() << "\n";
        // tempFile << "t " << tempoBuf << "\n";
        tempFile << src.rdbuf();
        tempFile.close();
        name = "temp";

        // play temp file instead of selected
        // std::cout << currStr << std::endl;
        // std::cout << seqList[state.currentItem] << std::endl;
        // std::cout << tempStr << std::endl;
        state.totalDuration =
            synthSequencer->getSequenceDuration(seqList[state.currentItem]);
        // synthSequencer->playSequence(name);
        state.currEvents = synthSequencer->loadSequence(seqList[state.currentItem], 0);
        synthSequencer->playEvents(state.currEvents);
        // state.currentTime = 0;

        // state.totalDuration =
        //     synthSequencer->getSequenceDuration(seqList[state.currentItem]);
        // synthSequencer->playSequence(seqList[state.currentItem]);
      }
      ImGui::SameLine();
      if (ImGui::Button("Stop")) {
        // std::cout << seqList[state.currentItem] << std::endl;
        synthSequencer->synth().allNotesOff();
        synthSequencer->stopSequence();
        // delete temp file
        // std::remove(synthSequencer->buildFullPath("temp").c_str());
        state.include = "";
        state.currEvents.clear();
        // state.currentTime = 0;
      }
      //            static float time = state.currentTime;
      if (ImGui::SliderFloat("Position", &state.currentTime, 0.0f,
                             state.totalDuration)) {
        std::cout << "Requested time:" << state.currentTime << std::endl;
        synthSequencer->setTime(state.currentTime);
        if(state.currentTime >= state.totalDuration){
            // delete temp file
            std::remove(synthSequencer->buildFullPath("temp").c_str());
            state.totalDuration =
            synthSequencer->getSequenceDuration(seqList[state.currentItem]);
            synthSequencer->playSequence(seqList[state.currentItem]);
            synthSequencer->synth().allNotesOff();
            synthSequencer->stopSequence();
        }
      }
    } else {
      ImGui::Text("No sequences found.");
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawSynthRecorder(SynthRecorder *synthRecorder) {
  struct SynthRecorderState {
    bool recordButton;
    bool overrideButton;
    bool quantizeButton;
  };
  static std::map<SynthRecorder *, SynthRecorderState> stateMap;
  if (stateMap.find(synthRecorder) == stateMap.end()) {
    stateMap[synthRecorder] = SynthRecorderState{0, false};
  }
  SynthRecorderState &state = stateMap[synthRecorder];

  if (ImGui::CollapsingHeader("Event Recorder##__EventRecorder",
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    static char buf1[64] = "test";
    ImGui::InputText("Record Name##__EventRecorder", buf1, 64);
    if (state.recordButton) {
      ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1.0, 0.0, 0.0, 1.0));
    }
    // ImGui::SameLine();
    ImGui::Checkbox("Overwrite##__EventRecorder", &state.overrideButton);
    // add quantize function
    ImGui::Checkbox("Quantize##__EventRecorder", &state.quantizeButton);
    ImGui::PushItemWidth(32);
    ImGui::SameLine();
    static char tempoBuf[16] = "120";
    ImGui::InputText("Tempo##__EventRecorder", tempoBuf, 16);
    ImGui::SameLine();
    static char noteBuf[16] = "8";
    ImGui::InputText("Note Type##__EventRecorder", noteBuf, 16);
    ImGui::PopItemWidth();
    // move record button to next row
    std::string buttonText = state.recordButton ? "Stop##__EventRecorder"
                                                : "Record##__EventRecorder";
    bool recordButtonClicked = ImGui::Button(buttonText.c_str());
    if (state.recordButton) {
      ImGui::PopStyleColor();
    }
    if (recordButtonClicked) {
      state.recordButton = !state.recordButton;
      if (state.recordButton) {
        synthRecorder->startRecord(buf1, state.overrideButton, 
                                   tempoBuf, state.quantizeButton, noteBuf);
      } else {
        synthRecorder->stopRecord();
      }
    }
  }
}

void ParameterGUI::drawParameterMIDI(ParameterMIDI *midi) {
  struct ParameterMIDIState {
    std::vector<std::string> devices;
    int currentDevice;
    bool verbose;
  };
  auto updateDevices = [](ParameterMIDIState &state) {
    RtMidiIn in;
    state.devices.clear();
    unsigned int numDevices = in.getPortCount();
    for (unsigned int i = 0; i < numDevices; i++) {
      state.devices.push_back(in.getPortName(i));
    }
  };
  static std::map<ParameterMIDI *, ParameterMIDIState> stateMap;
  if (stateMap.find(midi) == stateMap.end()) {
    stateMap[midi] = ParameterMIDIState();
    updateDevices(stateMap[midi]);
  }
  ParameterMIDIState &state = stateMap[midi];

  ImGui::PushID(std::to_string((uint64_t)midi).c_str());
  if (ImGui::CollapsingHeader("Paramter MIDI",
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    if (midi->isOpen()) {
      std::string noteBindings;
      //            noteBindings += "MIDI Note -> Channel, Value\n";
      for (auto &binding : midi->getCurrentNoteBindings()) {
        noteBindings += std::to_string(binding.noteNumber) + " -> ";
        noteBindings += std::to_string(binding.channel) + ":" +
                        std::to_string(binding.value);
        noteBindings += "\n";
      }
      if (noteBindings.size() > 0) {
        noteBindings = "MIDI Note -> Channel, Value\n" + noteBindings;
      }

      for (auto &binding : midi->getCurrentControlBindings()) {
        noteBindings += "Channel " + std::to_string(binding.channel + 1) +
                        " CC#" + std::to_string(binding.controlNumber) + " -> ";
        noteBindings += binding.param->getFullAddress();
        noteBindings += "\n";
      }
      ImGui::Text("%s", noteBindings.c_str());
      if (ImGui::Button("Stop")) {
        midi->close();
      }
    } else {
      if (ImGui::Combo(
              "Device", &state.currentDevice, ParameterGUI::vector_getter,
              static_cast<void *>(&state.devices), state.devices.size())) {
        // TODO adjust valid number of channels.
      }
      ImGui::Checkbox("Verbose", &state.verbose);
      if (ImGui::Button("Start")) {
        midi->open(state.currentDevice, state.verbose);
      }
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawPresetMIDI(PresetMIDI *presetMidi) {
  struct PresetMIDIState {
    std::vector<std::string> devices;
    int currentDevice;
  };
  auto updateDevices = [](PresetMIDIState &state) {
    RtMidiIn in;
    state.devices.clear();
    unsigned int numDevices = in.getPortCount();
    for (unsigned int i = 0; i < numDevices; i++) {
      state.devices.push_back(in.getPortName(i));
    }
  };
  static std::map<PresetMIDI *, PresetMIDIState> stateMap;
  if (stateMap.find(presetMidi) == stateMap.end()) {
    stateMap[presetMidi] = PresetMIDIState();
    updateDevices(stateMap[presetMidi]);
  }
  PresetMIDIState &state = stateMap[presetMidi];

  ImGui::PushID(std::to_string((uint64_t)presetMidi).c_str());
  if (ImGui::CollapsingHeader("Preset MIDI",
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    if (presetMidi->isOpen()) {
      std::string noteBindings;
      noteBindings += "MIDI Note -> Channel, Preset Index";
      //            for (auto &binding: presetMidi->getCurrentNoteBindings())
      //            {
      //                noteBindings += std::to_string(binding.noteNumber) + "
      //                -> "; noteBindings += std::to_string(binding.channel)
      //                +
      //                ":" + std::to_string(binding.presetIndex);
      //                noteBindings
      //                += "\n";
      //            }
      ImGui::Text("%s", noteBindings.c_str());
      if (ImGui::Button("Stop")) {
        presetMidi->close();
      }
    } else {
      if (ImGui::Combo(
              "Device", &state.currentDevice, ParameterGUI::vector_getter,
              static_cast<void *>(&state.devices), state.devices.size())) {
        // TODO adjust valid number of channels.
      }
      if (ImGui::Button("Start")) {
        presetMidi->open(state.currentDevice);
      }
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawAudioIO(AudioIO *io) {
  struct AudioIOState {
    int currentSr = 0;
    int currentBufSize = 0;
    int currentDevice = 0;
    std::vector<std::string> devices;
  };
  auto updateDevices = [&](AudioIOState &state) {
    state.devices.clear();
    int numDevices = AudioDevice::numDevices();
    for (int i = 0; i < numDevices; i++) {
      state.devices.push_back(AudioDevice(i).name());
    }
  };
  static std::map<AudioIO *, AudioIOState> stateMap;
  if (stateMap.find(io) == stateMap.end()) {
    stateMap[io] = AudioIOState();
    updateDevices(stateMap[io]);
  }
  AudioIOState &state = stateMap[io];
  ImGui::PushID(std::to_string((uint64_t)io).c_str());
  if (ImGui::CollapsingHeader("Audio", ImGuiTreeNodeFlags_CollapsingHeader |
                                           ImGuiTreeNodeFlags_DefaultOpen)) {
    if (io->isOpen()) {
      std::string text;
      text += "Sampling Rate: " + std::to_string(io->fps());
      text += "\nbuffer size: " + std::to_string(io->framesPerBuffer());
      text += "\nin chnls: " + std::to_string(io->channelsIn());
      text += "\nout chnls:" + std::to_string(io->channelsOut());
      ImGui::Text("%s", text.c_str());
      if (ImGui::Button("Stop")) {
        io->stop();
        io->close();
      }
    } else {
      if (ImGui::Button("Update Devices")) {
        updateDevices(state);
      }
      if (ImGui::Combo(
              "Device", &state.currentDevice, ParameterGUI::vector_getter,
              static_cast<void *>(&state.devices), state.devices.size())) {
        // TODO adjust valid number of channels.
      }
      std::vector<std::string> samplingRates{"44100", "48000", "88100",
                                             "96000"};
      ImGui::Combo("Sampling Rate", &state.currentSr,
                   ParameterGUI::vector_getter,
                   static_cast<void *>(&samplingRates), samplingRates.size());

      std::vector<std::string> bufferSizes{"64", "128", "256", "512", "1024"};
      ImGui::Combo("Buffer size", &state.currentBufSize,
                   ParameterGUI::vector_getter,
                   static_cast<void *>(&bufferSizes), bufferSizes.size());
      if (ImGui::Button("Start")) {
        io->framesPerSecond(std::stof(samplingRates[state.currentSr]));
        io->framesPerBuffer(std::stof(bufferSizes[state.currentBufSize]));
        io->device(AudioDevice(state.currentDevice));
        io->open();
        io->start();
      }
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawBundleGroup(std::vector<ParameterBundle *> bundleGroup,
                                   string suffix, int &currentBundle,
                                   bool &bundleGlobal) {
  if (bundleGroup.size() == 0) {
    return;
  }
  std::string name = bundleGroup[0]->name();
  int index = currentBundle;

  //    ImGui::Separator();
  ImGui::PushID(suffix.c_str());

  if (ImGui::CollapsingHeader(name.c_str(),
                              ImGuiTreeNodeFlags_CollapsingHeader |
                                  ImGuiTreeNodeFlags_DefaultOpen)) {
    if (!bundleGlobal) {
      //            ImGui::SameLine();
      if (ImGui::InputInt("Current", &index)) {
        if (index >= 0 && index < (int)bundleGroup.size()) {
          currentBundle = index;
        }
      }
      ImGui::SameLine();
    }
    ImGui::Checkbox("Global", &bundleGlobal);
    suffix += "__index_" + std::to_string(index);
    if (bundleGlobal) {
      // We will try to match parameters in order to the first bundle
      // Perhaps we should try to do better matching to match parameter names,
      // but for now we assume that parameters have exactly the same
      // order inside bundles to be able to group them
      for (unsigned int i = 0; i < bundleGroup[0]->parameters().size(); i++) {
        std::vector<Parameter *> params;
        std::vector<ParameterBool *> boolParams;
        std::string paramName = bundleGroup[0]->parameters()[i]->getName();
        for (auto *bundle : bundleGroup) {
          auto &parameters = bundle->parameters();
          auto &param = *parameters[i];
          if (parameters[i]->getName() == paramName) {
            if (strcmp(typeid(param).name(), typeid(Parameter).name()) == 0) {
              params.push_back(dynamic_cast<Parameter *>(&param));
            } else if (strcmp(typeid(param).name(),
                              typeid(ParameterBool).name()) == 0) {
              boolParams.push_back(dynamic_cast<ParameterBool *>(&param));
            }
          }
        }
        drawParameter(params, suffix, index);
        drawParameterBool(boolParams, suffix, index);
      }

    } else {
      for (ParameterMeta *param : bundleGroup[currentBundle]->parameters()) {
        drawParameterMeta(param, suffix);
      }
      for (auto subbundleGroup : bundleGroup[currentBundle]->bundles()) {
        std::string subBundleName = subbundleGroup.first;
        if (ImGui::CollapsingHeader(
                (subBundleName + "##" + name + subBundleName).c_str(),
                ImGuiTreeNodeFlags_CollapsingHeader)) {
          for (auto *bundle : subbundleGroup.second) {
            for (auto *param : bundle->parameters()) {
              drawParameterMeta({param}, suffix + subBundleName, 0);
            }
          }
        }
      }
    }
    ImGui::Separator();
  }
  ImGui::PopID();
}

void ParameterGUI::drawBundle(ParameterBundle *bundle) {
  auto name = bundle->name();
  ImGui::PushID((name + std::to_string((uint64_t)bundle)).c_str());

  if (ImGui::CollapsingHeader(name.c_str())) {
    for (ParameterMeta *param : bundle->parameters()) {
      drawParameterMeta(param);
    }
    for (auto innerBundle : bundle->bundles()) {
      std::string subBundleName = innerBundle.first;
      ImGui::PushID(subBundleName.c_str());
      if (ImGui::CollapsingHeader(subBundleName.c_str(),
                                  ImGuiTreeNodeFlags_CollapsingHeader)) {
        for (auto *bundle : innerBundle.second) {
          for (auto *param : bundle->parameters()) {
            drawParameterMeta({param}, subBundleName, 0);
          }
        }
      }
      ImGui::PopID();
    }
  }
  ImGui::PopID();
}

void ParameterGUI::drawBundleManager(BundleGUIManager *manager) {
  std::string suffix = "##_bundle_" + manager->name();
  ParameterGUI::drawBundleGroup(manager->bundles(), suffix,
                                manager->currentBundle(),
                                manager->bundleGlobal());
}
