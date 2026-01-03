#pragma once
#include <hyprland/src/render/pass/PassElement.hpp>

class Superglue;

class GluePassElement : public IPassElement {
  public:
    struct SGlueData {
        Superglue* deco = nullptr;
        float      a    = 1.F;
    };

    GluePassElement(const SGlueData& data_);
    virtual ~GluePassElement() = default;

    virtual void                draw(const CRegion& damage);
    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();

    virtual const char*         passName() {
        return "GluePassElement";
    }

  private:
    SGlueData data;
};
