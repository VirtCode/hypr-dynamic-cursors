#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include <optional>

class CWLSurfaceResource;
class CTexture;
class CSyncTimeline;

class CCursorPassElement : public IPassElement {
  public:
    struct SRenderData {
        SP<CTexture>          tex;
        CBox                  box;
        CRegion               damage;

        Vector2D hotspot;
        bool nearest;
        double stretchAngle;
        Vector2D stretchMagnitude;
    };

    CCursorPassElement(const SRenderData& data);
    virtual ~CCursorPassElement() = default;

    virtual void                draw(const CRegion& damage);
    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();
    virtual CRegion             opaqueRegion();
    virtual void                discard();

    virtual const char*         passName() {
        return "CCursorPassElement";
    }

  private:
    SRenderData data;
};
