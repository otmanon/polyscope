#include "polyscope/surface_graph_quantity.h"

#include "polyscope/gl/materials/materials.h"
#include "polyscope/gl/shaders.h"
#include "polyscope/gl/shaders/cylinder_shaders.h"
#include "polyscope/gl/shaders/sphere_shaders.h"
#include "polyscope/polyscope.h"
#include "polyscope/utilities.h"

#include "imgui.h"

using std::cout;
using std::endl;

namespace polyscope {

SurfaceGraphQuantity::SurfaceGraphQuantity(std::string name, std::vector<glm::vec3> nodes_,
                                           std::vector<std::array<size_t, 2>> edges_, SurfaceMesh& mesh_)
    : SurfaceMeshQuantity(name, mesh_), nodes(std::move(nodes_)), edges(std::move(edges_))

{

  // Validate that indices are in bounds
  for (auto& p : edges) {
    if (p[0] >= nodes.size()) {
      warning("surface graph [" + name + "] has out of bounds edge index",
              "index = " + std::to_string(p[0]) + " but nNodes = " + std::to_string(nodes.size()));
    }
    if (p[1] >= nodes.size()) {
      warning("surface graph [" + name + "] has out of bounds edge index",
              "index = " + std::to_string(p[1]) + " but nNodes = " + std::to_string(nodes.size()));
    }
  }

  // initialize the color to something nice
  color = getNextUniqueColor();
}

void SurfaceGraphQuantity::draw() {
  if (!enabled) return;

  if (pointProgram == nullptr || lineProgram == nullptr) {
    createPrograms();
  }

  setUniforms();

  pointProgram->draw();
  //lineProgram->draw();
}

void SurfaceGraphQuantity::setUniforms() {

  parent.setTransformUniforms(*pointProgram);
  parent.setTransformUniforms(*lineProgram);

  // Point billboard uniforms
  glm::vec3 lookDir, upDir, rightDir;
  view::getCameraFrame(lookDir, upDir, rightDir);
  pointProgram->setUniform("u_camZ", lookDir);
  pointProgram->setUniform("u_camUp", upDir);
  pointProgram->setUniform("u_camRight", rightDir);

  // Radii and colors
  pointProgram->setUniform("u_pointRadius", radius * state::lengthScale);
  lineProgram->setUniform("u_radius", radius * state::lengthScale);
  pointProgram->setUniform("u_baseColor", color);
  lineProgram->setUniform("u_color", color);
}


void SurfaceGraphQuantity::createPrograms() {

  { // Point program
    pointProgram.reset(new gl::GLProgram(&SPHERE_VERT_SHADER, &SPHERE_COLOR_BILLBOARD_GEOM_SHADER,
                                         &SPHERE_BILLBOARD_FRAG_SHADER, gl::DrawMode::Points));

    pointProgram->setAttribute("a_position", nodes);
    setMaterialForProgram(*pointProgram, "wax");
  }

  { // Line program

    lineProgram.reset(new gl::GLProgram(&PASSTHRU_CYLINDER_VERT_SHADER, &CYLINDER_GEOM_SHADER, &CYLINDER_FRAG_SHADER,
                                        gl::DrawMode::Points));

    // Build buffers
    std::vector<glm::vec3> edgeStarts, edgeEnds;
    edgeStarts.reserve(edges.size());
    edgeEnds.reserve(edges.size());
    for (auto& p : edges) {
      edgeStarts.push_back(nodes[p[0]]);
      edgeEnds.push_back(nodes[p[1]]);
    }

    // Store data in buffers
    lineProgram->setAttribute("a_position_tail", edgeStarts);
    lineProgram->setAttribute("a_position_tip", edgeEnds);

    setMaterialForProgram(*lineProgram, "wax");
  }
}

void SurfaceGraphQuantity::buildCustomUI() {

  ImGui::ColorEdit3("Color", (float*)&color, ImGuiColorEditFlags_NoInputs);
  ImGui::Text("Nodes: %lu", nodes.size());
  ImGui::Text("Edges: %lu", edges.size());
  ImGui::SliderFloat("Radius", &radius, 0.0, .1, "%.5f", 3.);
}

std::string SurfaceGraphQuantity::niceName() { return name; }


} // namespace polyscope
