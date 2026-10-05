#pragma once

#include <hyprland/src/render/pass/PassElement.hpp>
#include <hyprland/src/render/Texture.hpp>
#include <optional>

class CCursorPassElement : public IPassElement {
  public:
    struct SRenderData {
        SP<Render::ITexture> tex;
        CBox                 box;
        CRegion              damage;

        Vector2D hotspot;
        bool     nearest;
        double   stretchAngle;
        Vector2D stretchMagnitude;
    };

    CCursorPassElement(const SRenderData& data);
    virtual ~CCursorPassElement() = default;

    virtual std::vector<UP<IPassElement>> draw(Render::CRenderContext& ctx);
    virtual bool                          needsLiveBlur(Render::CRenderContext& ctx);
    virtual bool                          needsPrecomputeBlur(Render::CRenderContext& ctx);
    virtual std::optional<CBox>           boundingBox(Render::CRenderContext& ctx);
    virtual CRegion                       opaqueRegion(Render::CRenderContext& ctx);
    virtual void                          discard(Render::CRenderContext& ctx);

    virtual const char* passName() {
        return "CCursorPassElement";
    }

    virtual ePassElementType type() {
        return EK_CUSTOM;
    };

  private:
    SRenderData m_data;
};
