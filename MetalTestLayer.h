//
//  MetalTestLayer.mm
//  TestGui
//
//  Created by Yejneshwar on 30/08/25.
//

#pragma once
#include "Core/Layer.h"

class MetalTestLayer : public GUI::Layer {
public:
    MetalTestLayer();
    
    ~MetalTestLayer();
    
    void OnAttach() override;
    
    void OnDetach() override;
    
    void OnUpdateLayer() override;
    
    void OnDrawUpdate() override;
    
    void OnEvent(Application::Event &event) override;
    
    void OnSelection(int objectId, bool state) override;
    
    void OnImGuiRender() override;
private:

    void* _pDevice;
    void* _pCommandQueue;
    void* _pPipelineState;
    void* _pVertexBuffer;
    void* _pRenderTargetTexture;
};
