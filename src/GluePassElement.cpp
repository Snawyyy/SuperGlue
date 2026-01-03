#include "GluePassElement.hpp"
#include <hyprland/src/render/OpenGL.hpp>
#include "Superglue.hpp"

GluePassElement::GluePassElement(const GluePassElement::SGlueData& data_) : data(data_) {
    ;
}

void GluePassElement::draw(const CRegion& damage) {
    // Call back into Superglue to do the actual OpenGL drawing
    // We pass the monitor from the current render data
    data.deco->renderPass(g_pHyprOpenGL->m_renderData.pMonitor.lock(), data.a);
}

bool GluePassElement::needsLiveBlur() {
    return false;
}

std::optional<CBox> GluePassElement::boundingBox() {
    // We can expand this slightly if we fear occlusion
    return data.deco->assignedBoxGlobal();
}

bool GluePassElement::needsPrecomputeBlur() {
    return false;
}
