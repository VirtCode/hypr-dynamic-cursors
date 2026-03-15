#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include <optional>

class CWLSurfaceResource;
class ITexture;
class CSyncTimeline;

class CCursorPassElement : public IPassElement {
  public:
    struct SRenderData {
        SP<ITexture>          tex;
        CBox                  box;
        CRegion               damage;

        Vector2D hotspot;
        bool nearest;
        double stretchAngle;
        Vector2D stretchMagnitude;
    };

    CCursorPassElement(const SRenderData& data);
    virtual ~CCursorPassElement() = default;

    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();
    virtual CRegion             opaqueRegion();
    virtual void                discard();

    virtual const char*         passName() {
        return "CCursorPassElement";
    }
    
    virtual ePassElementType type() override {
        return EK_TEXTURE;
    }

  private:
    SRenderData data;
};
