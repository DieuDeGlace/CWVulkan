#pragma once
#include "PlasmaEdit.h"
#include "PlasmaInput.h"
#include "PlasmaText.h"
#include <optional>

namespace plasma {
// Ordinary scene/owner adapters. The editor keeps no font or scene ownership.
class EditScene {
  public:
    using Measure = std::function<Edit::Metric(const SceneNode &,std::u16string_view,int32_t,const glm::mat4 &)>;
    using Bound = std::function<glm::vec2(const SceneNode &,bool upper)>;
    EditScene(Scene &,SceneNode &,Input &,Measure,Bound);
    Edit edit;
    void attach();
    void pointer(Input::Hook);
    void keyboard(Input::Key,uint16_t);
    struct Caret { float x=0,y=0,height=0; };
    // Performs two endpoint measurements even though the ordinary selection draw is a no-op.
    std::optional<Caret> drawMetrics() const;
    std::optional<TextStyle> style() const;
  private:
    Scene &scene;
    SceneNode &node;
    Input &input;
    Measure measure;
    Bound bound;
    bool caption=false;
    float baseline=0;
    Edit::Caption captionPorts();
    Edit::Pointer pointerPorts();
    void layout();
    std::u16string text() const;
};
}
