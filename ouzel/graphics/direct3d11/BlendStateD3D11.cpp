// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#include "BlendStateD3D11.h"
#include "RendererD3D11.h"
#include "core/Engine.h"
#include "utils/Log.h"

namespace ouzel
{
    namespace graphics
    {
        BlendStateD3D11::BlendStateD3D11()
        {
        }

        BlendStateD3D11::~BlendStateD3D11()
        {
            if (blendState)
            {
                blendState->Release();
            }
        }

        static D3D11_BLEND getBlendFactor(BlendState::BlendFactor blendFactor)
        {
            switch (blendFactor)
            {
                case BlendState::BlendFactor::ZERO: return D3D11_BLEND_ZERO;
                case BlendState::BlendFactor::ONE: return D3D11_BLEND_ONE;
                case BlendState::BlendFactor::SRC_COLOR: return D3D11_BLEND_SRC_COLOR;
                case BlendState::BlendFactor::INV_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
                case BlendState::BlendFactor::SRC_ALPHA: return D3D11_BLEND_SRC_ALPHA;
                case BlendState::BlendFactor::INV_SRC_ALPHA: return D3D11_BLEND_INV_SRC_ALPHA;
                case BlendState::BlendFactor::DEST_ALPHA: return D3D11_BLEND_DEST_ALPHA;
                case BlendState::BlendFactor::INV_DEST_ALPHA: return D3D11_BLEND_INV_DEST_ALPHA;
                case BlendState::BlendFactor::DEST_COLOR: return D3D11_BLEND_DEST_COLOR;
                case BlendState::BlendFactor::INV_DEST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
                case BlendState::BlendFactor::SRC_ALPHA_SAT: return D3D11_BLEND_SRC_ALPHA_SAT;
                case BlendState::BlendFactor::BLEND_FACTOR: return D3D11_BLEND_BLEND_FACTOR;
                case BlendState::BlendFactor::INV_BLEND_FACTOR: return D3D11_BLEND_INV_BLEND_FACTOR;
                default: return D3D11_BLEND_ZERO;
            }
        }

        static D3D11_BLEND_OP getBlendOperation(BlendState::BlendOperation blendOperation)
        {
            switch (blendOperation)
            {
                case BlendState::BlendOperation::ADD: return D3D11_BLEND_OP_ADD;
                case BlendState::BlendOperation::SUBTRACT: return D3D11_BLEND_OP_SUBTRACT;
                case BlendState::BlendOperation::REV_SUBTRACT: return D3D11_BLEND_OP_REV_SUBTRACT;
                case BlendState::BlendOperation::MIN: return D3D11_BLEND_OP_MIN;
                case BlendState::BlendOperation::MAX: return D3D11_BLEND_OP_MAX;
                default: return D3D11_BLEND_OP_ADD;
            }
        }

        bool BlendStateD3D11::upload()
        {
            if (!BlendStateResource::upload())
            {
                return false;
            }

            if (data.dirty)
            {
                RendererD3D11* rendererD3D11 = static_cast<RendererD3D11*>(sharedEngine->getRenderer());

                // Blending state
                D3D11_BLEND_DESC blendStateDesc;
                blendStateDesc.AlphaToCoverageEnable = FALSE;
                blendStateDesc.IndependentBlendEnable = FALSE;

                D3D11_RENDER_TARGET_BLEND_DESC targetBlendDesc;
                targetBlendDesc.BlendEnable = data.enableBlending ? TRUE : FALSE;
                targetBlendDesc.SrcBlend = getBlendFactor(data.colorBlendSource);
                targetBlendDesc.DestBlend = getBlendFactor(data.colorBlendDest);
                targetBlendDesc.BlendOp = getBlendOperation(data.colorOperation);
                targetBlendDesc.SrcBlendAlpha = getBlendFactor(data.alphaBlendSource);
                targetBlendDesc.DestBlendAlpha = getBlendFactor(data.alphaBlendDest);
                targetBlendDesc.BlendOpAlpha = getBlendOperation(data.alphaOperation);
                targetBlendDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

                blendStateDesc.RenderTarget[0] = targetBlendDesc;

                if (blendState) blendState->Release();

                HRESULT hr = rendererD3D11->getDevice()->CreateBlendState(&blendStateDesc, &blendState);
                if (FAILED(hr))
                {
                    Log(Log::Level::ERR) << "Failed to create Direct3D 11 blend state";
                    return false;
                }

                data.dirty = 0;
            }

            return true;
        }
    } // namespace graphics
} // namespace ouzel
