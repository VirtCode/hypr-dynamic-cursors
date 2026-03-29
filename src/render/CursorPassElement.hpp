#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <optional>

class CCursorPassElement : public IPassElement {
  public:
    struct SRenderData {
        SP<Render::ITexture>   tex;
        CBox                   box;
        CRegion                damage;

        Vector2D hotspot;
        bool nearest;
        double stretchAngle;
        Vector2D stretchMagnitude;
    };

    CCursorPassElement(const SRenderData& data);
    virtual ~CCursorPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw();
    virtual bool                needsLiveBlur();
    virtual bool                needsPrecomputeBlur();
    virtual std::optional<CBox> boundingBox();
    virtual CRegion             opaqueRegion();
    virtual void                discard();

    virtual const char*         passName() {
        return "CCursorPassElement";
    }

    virtual ePassElementType type() {
        return EK_CUSTOM;
    };

  private:
    SRenderData m_data;
};
