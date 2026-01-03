#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>

class Superglue;

/**
 * Custom render pass element for Superglue overlays.
 */
class GluePassElement : public IPassElement {
 public:
  struct SGlueData {
    Superglue* deco = nullptr;
    float a = 1.0f;
  };

  explicit GluePassElement(const SGlueData& data);
  virtual ~GluePassElement() = default;

  virtual void draw(const CRegion& damage) override;
  virtual bool needsLiveBlur() override;
  virtual bool needsPrecomputeBlur() override;
  virtual std::optional<CBox> boundingBox() override;

  virtual const char* passName() override {
    return "GluePassElement";
  }

 private:
  SGlueData m_data;
};
