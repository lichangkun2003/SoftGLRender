/*
 * SoftGLRender
 * @author 	: keith@robot9.me
 *
 */

#include "ConfigPanel.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"
#include "json11.hpp"
#include "Base/Logger.h"
#include "Base/FileUtils.h"

namespace SoftGL {
namespace View {
int shadowSelectedItem = 0;
bool ConfigPanel::init(void *window, int width, int height) {
  frameWidth_ = width;
  frameHeight_ = height;

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.IniFilename = nullptr;

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  ImGuiStyle *style = &ImGui::GetStyle();
  style->Alpha = 0.8f;

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOpenGL((GLFWwindow *) window, true);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  // load config
  return loadConfig();
}

void ConfigPanel::onDraw() {
  // Start the Dear ImGui frame
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();
  float firstWindowHeight = 0.0f;
  float windowWidth = 250.0f;

  ImGui::SetNextWindowSize(ImVec2(windowWidth, 0)); // 设置第一个窗口的宽度
  ImGui::Begin("##fps",
      nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove);

  // Display your metrics
  ImGui::Text("fps: %.1f (%.2f ms/frame)", ImGui::GetIO().Framerate, 1000.0f / ImGui::GetIO().Framerate);
  ImGui::Text("triangles: %zu", config_.triangleCount_);
  ImGui::SetWindowPos(ImVec2(frameWidth_ - windowWidth, 0));
  // Get the height of the first window
  firstWindowHeight = ImGui::GetWindowHeight();
  // End the window
  ImGui::End();

  // Settings window
  ImGui::Begin("Settings",
               nullptr,
               ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);
  drawSettings();
  ImGui::SetWindowPos(ImVec2(frameWidth_ - windowWidth, firstWindowHeight));
  ImGui::End();

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void ConfigPanel::drawSettings() {
  // my shadow
    const char* shadowItems[] = {
        "no shadow",
        "ShadowMap(basic)",
        "ShadowMap(PCF)",
        "ShadowVolume"
    };

  // renderer
  const char *rendererItems[] = {
      "Software",
      "OpenGL",
      "Vulkan",
  };
  ImGui::Separator();
  ImGui::Text("renderer");
  for (int i = 0; i < 3; i++) {
    if (ImGui::RadioButton(rendererItems[i], config_.rendererType == i)) {
      config_.rendererType = i;
    }
    ImGui::SameLine();
  }
  ImGui::Separator();

  // reset camera
  ImGui::Separator();
  ImGui::Text("camera:");
  ImGui::SameLine();
  if (ImGui::SmallButton("reset")) {
    if (resetCameraFunc_) {
      resetCameraFunc_();
    }
  }

  // frame dump
  ImGui::Separator();
  ImGui::Text("debug (RenderDoc):");
  ImGui::SameLine();
  if (ImGui::SmallButton("capture")) {
    if (frameDumpFunc_) {
      frameDumpFunc_();
    }
  }


  // model
  ImGui::Separator();
  ImGui::Text("load model");

  int modelIdx = 0;
  for (; modelIdx < modelNames_.size(); modelIdx++) {
    if (config_.modelName == modelNames_[modelIdx]) {
      break;
    }
  }
  if (ImGui::Combo("##load model", &modelIdx, modelNames_.data(), (int) modelNames_.size())) {
    reloadModel(modelNames_[modelIdx]);
  }

  // skybox
  ImGui::Separator();
  ImGui::Checkbox("load skybox", &config_.showSkybox);

  if (config_.showSkybox) {
    // pbr ibl
    ImGui::Checkbox("enable IBL", &config_.pbrIbl);

    int skyboxIdx = 0;
    for (; skyboxIdx < skyboxNames_.size(); skyboxIdx++) {
      if (config_.skyboxName == skyboxNames_[skyboxIdx]) {
        break;
      }
    }
    if (ImGui::Combo("##skybox", &skyboxIdx, skyboxNames_.data(), (int) skyboxNames_.size())) {
      reloadSkybox(skyboxNames_[skyboxIdx]);
    }
  }

  // clear Color
  ImGui::Separator();
  ImGui::Text("clear color");
  ImGui::ColorEdit4("clear color", (float *) &config_.clearColor, ImGuiColorEditFlags_NoLabel);

  // wireframe
  ImGui::Separator();
  ImGui::Checkbox("wireframe", &config_.wireframe);

  // world axis
  ImGui::Separator();
  ImGui::Checkbox("world axis", &config_.worldAxis);

  // shadow floor
  //ImGui::Separator();
  //ImGui::Checkbox("shadow floor", &config_.showFloor);
  //config_.shadowMap = config_.showFloor;
  config_.showFloor = true;

  // my shadow
  ImGui::Separator();
  ImGui::Text("shadow");
  if (ImGui::BeginCombo("##generate shadow", shadowItems[shadowSelectedItem])) {
      for (int i = 0; i < 4; i++) {
          bool isSelected = (shadowSelectedItem == i);
          if (ImGui::Selectable(shadowItems[i], isSelected))
              shadowSelectedItem = i;

          if (isSelected)
              ImGui::SetItemDefaultFocus();   // 选中的项目会默认聚焦
      }
      ImGui::EndCombo();
  }
  if (shadowSelectedItem == 2) {
      config_.shadowMap = true;
      config_.shadowtype = 1;
  }
  else if (shadowSelectedItem == 1) {
      config_.shadowMap = true;
      config_.shadowtype = 0;
  }
  else {
      config_.shadowMap = false;
  }

  if (config_.shadowMap) {
      ImGui::Separator();
      ImGui::Text("bias");
      ImGui::SliderFloat("##bias", &biasPercent, 0.0f, 0.001f, "%.6f", ImGuiSliderFlags_Logarithmic);
  }


  if (!config_.wireframe) {
    // light
    ImGui::Separator();
    ImGui::Text("ambient color");
    ImGui::ColorEdit3("ambient color", (float *) &config_.ambientColor, ImGuiColorEditFlags_NoLabel);

    ImGui::Separator();
    ImGui::Checkbox("point light", &config_.showLight);
    if (config_.showLight) {
      ImGui::Text("light color");
      ImGui::ColorEdit3("light color", (float *) &config_.pointLightColor, ImGuiColorEditFlags_NoLabel);

      ImGui::Text("light position");
      ImGui::SliderAngle("##light position", &lightPositionAngle_, 0, 360.f);
    }

    // mipmaps
    ImGui::Separator();
    if (ImGui::Checkbox("mipmaps", &config_.mipmaps)) {
      if (resetMipmapsFunc_) {
        resetMipmapsFunc_();
      }
    }
  }

  // face cull
  ImGui::Separator();
  ImGui::Checkbox("cull face", &config_.cullFace);

  // depth test
  ImGui::Separator();
  ImGui::Checkbox("depth test", &config_.depthTest);

  // reverse z
  ImGui::Separator();
  if (ImGui::Checkbox("reverse z", &config_.reverseZ)) {
    if (resetReverseZFunc_) {
      resetReverseZFunc_();
    }
  }

  // Anti aliasing
  const char *aaItems[] = {
      "NONE",
      "MSAA",
      "FXAA",
  };
  ImGui::Separator();
  ImGui::Text("Anti-aliasing");
  for (int i = 0; i < 3; i++) {
    if (ImGui::RadioButton(aaItems[i], config_.aaType == i)) {
      config_.aaType = i;
    }
    ImGui::SameLine();
  }
}

void ConfigPanel::destroy() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();
}

void ConfigPanel::update() {
  // update shadowmap bias
  config_.shadowMapBias = biasPercent;
  if (updateBiasFunc_) {
      updateBiasFunc_(config_.shadowMapBias);
  }

  // update light position
  config_.pointLightPosition = 2.f * glm::vec3(glm::sin(lightPositionAngle_),
                                               1.2f,
                                               glm::cos(lightPositionAngle_));
  if (updateLightFunc_) {
    updateLightFunc_(config_.pointLightPosition, config_.pointLightColor);
  }
}

void ConfigPanel::updateSize(int width, int height) {
  frameWidth_ = width;
  frameHeight_ = height;
}

bool ConfigPanel::wantCaptureKeyboard() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureKeyboard;
}

bool ConfigPanel::wantCaptureMouse() {
  ImGuiIO &io = ImGui::GetIO();
  return io.WantCaptureMouse;
}

bool ConfigPanel::loadConfig() {
  auto configPath = ASSETS_DIR + "assets.json";
  auto configStr = FileUtils::readText(configPath);
  if (configStr.empty()) {
    LOGE("load models failed: error read config file");
    return false;
  }

  std::string err;
  const auto json = json11::Json::parse(configStr, err);
  for (auto &kv : json["model"].object_items()) {
    modelPaths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
  }
  for (auto &kv : json["skybox"].object_items()) {
    skyboxPaths_[kv.first] = ASSETS_DIR + kv.second["path"].string_value();
  }

  if (modelPaths_.empty()) {
    LOGE("load models failed: %s", err.c_str());
    return false;
  }

  for (const auto &kv : modelPaths_) {
    modelNames_.emplace_back(kv.first.c_str());
  }
  for (const auto &kv : skyboxPaths_) {
    skyboxNames_.emplace_back(kv.first.c_str());
  }

  // load default model & skybox
  return reloadModel(modelPaths_.begin()->first) && reloadSkybox(skyboxPaths_.begin()->first);
}

bool ConfigPanel::reloadModel(const std::string &name) {
  if (name != config_.modelName) {
    config_.modelName = name;
    config_.modelPath = modelPaths_[config_.modelName];

    if (reloadModelFunc_) {
      return reloadModelFunc_(config_.modelPath);
    }
  }

  return true;
}

bool ConfigPanel::reloadSkybox(const std::string &name) {
  if (name != config_.skyboxName) {
    config_.skyboxName = name;
    config_.skyboxPath = skyboxPaths_[config_.skyboxName];

    if (reloadSkyboxFunc_) {
      return reloadSkyboxFunc_(config_.skyboxPath);
    }
  }

  return true;
}

}
}
