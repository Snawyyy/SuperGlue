#include "pass-element.hpp"
#include "decoration.hpp"
#include <hyprland/src/render/OpenGL.hpp>

GluePassElement::GluePassElement(const SGlueData& data)
    : m_data(data) {}

void GluePassElement::draw(const CRegion& damage) {
  m_data.deco->renderPass(
      g_pHyprOpenGL->m_renderData.pMonitor.lock(),
      m_data.a);
}

bool GluePassElement::needsLiveBlur() {
  return false;
}

bool GluePassElement::needsPrecomputeBlur() {
  return false;
}

std::optional<CBox> GluePassElement::boundingBox() {
  return m_data.deco->assignedBoxGlobal();
}
