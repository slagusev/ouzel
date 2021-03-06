// Copyright (C) 2017 Elviss Strazdins
// This file is part of the Ouzel engine.

#include <sstream>

#include "RendererOGL.h"
#include "TextureOGL.h"
#include "ShaderOGL.h"
#include "MeshBufferOGL.h"
#include "BufferOGL.h"
#include "BlendStateOGL.h"
#include "core/Engine.h"
#include "core/Window.h"
#include "core/Cache.h"
#include "utils/Log.h"
#include "stb_image_write.h"

#if OUZEL_SUPPORTS_OPENGL
#include "ColorPSGL2.h"
#include "ColorVSGL2.h"
#include "TexturePSGL2.h"
#include "TextureVSGL2.h"
#include "ColorPSGL3.h"
#include "ColorVSGL3.h"
#include "TexturePSGL3.h"
#include "TextureVSGL3.h"
#elif OUZEL_SUPPORTS_OPENGLES
#include "ColorPSGLES2.h"
#include "ColorVSGLES2.h"
#include "TexturePSGLES2.h"
#include "TextureVSGLES2.h"
#include "ColorPSGLES3.h"
#include "ColorVSGLES3.h"
#include "TexturePSGLES3.h"
#include "TextureVSGLES3.h"
#endif

#if OUZEL_OPENGL_INTERFACE_EGL
#include "EGL/egl.h"
#elif OUZEL_OPENGL_INTERFACE_XGL
#define GL_GLEXT_PROTOTYPES 1
#include <GL/glx.h>
#include "GL/glxext.h"
#endif

#if OUZEL_PLATFORM_MACOS
#include "mach-o/dyld.h"
#endif

#if OUZEL_SUPPORTS_OPENGL
    PFNGLGENVERTEXARRAYSPROC genVertexArraysProc;
    PFNGLBINDVERTEXARRAYPROC bindVertexArrayProc;
    PFNGLDELETEVERTEXARRAYSPROC deleteVertexArraysProc;
    PFNGLMAPBUFFERPROC mapBufferProc;
    PFNGLUNMAPBUFFERPROC unmapBufferProc;
    PFNGLMAPBUFFERRANGEPROC mapBufferRangeProc;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEPROC renderbufferStorageMultisampleProc;
#elif OUZEL_SUPPORTS_OPENGLES
    PFNGLGENVERTEXARRAYSOESPROC genVertexArraysProc;
    PFNGLBINDVERTEXARRAYOESPROC bindVertexArrayProc;
    PFNGLDELETEVERTEXARRAYSOESPROC deleteVertexArraysProc;
    PFNGLMAPBUFFEROESPROC mapBufferProc;
    PFNGLUNMAPBUFFEROESPROC unmapBufferProc;
    PFNGLMAPBUFFERRANGEEXTPROC mapBufferRangeProc;
    PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC renderbufferStorageMultisampleProc;
    PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC framebufferTexture2DMultisampleProc;
#endif

namespace ouzel
{
    namespace graphics
    {
        RendererOGL::RendererOGL():
            Renderer(Driver::OPENGL)
        {
            projectionTransform = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                                          0.0f, 1.0f, 0.0f, 0.0f,
                                          0.0f, 0.0f, 2.0f, -1.0f,
                                          0.0f, 0.0f, 0.0f, 1.0f);

            renderTargetProjectionTransform = Matrix4(1.0f, 0.0f, 0.0f, 0.0f,
                                                      0.0f, -1.0f, 0.0f, 0.0f,
                                                      0.0f, 0.0f, 2.0f, -1.0f,
                                                      0.0f, 0.0f, 0.0f, 1.0f);

            stateCache.bufferId[GL_ELEMENT_ARRAY_BUFFER] = 0;
            stateCache.bufferId[GL_ARRAY_BUFFER] = 0;
        }

        RendererOGL::~RendererOGL()
        {
            resourceDeleteSet.clear();
            resources.clear();
            
            if (colorRenderBufferId)
            {
                glDeleteRenderbuffers(1, &colorRenderBufferId);
            }

            if (depthRenderBufferId)
            {
                glDeleteRenderbuffers(1, &depthRenderBufferId);
            }

            if (frameBufferId)
            {
                glDeleteFramebuffers(1, &frameBufferId);
            }
        }

        bool RendererOGL::init(Window* newWindow,
                               const Size2& newSize,
                               uint32_t newSampleCount,
                               Texture::Filter newTextureFilter,
                               PixelFormat newBackBufferFormat,
                               bool newVerticalSync,
                               bool newDepth)
        {
            if (!Renderer::init(newWindow, newSize, newSampleCount, newTextureFilter, newBackBufferFormat, newVerticalSync, newDepth))
            {
                return false;
            }

            //const GLubyte* deviceVendor = glGetString(GL_VENDOR);
            const GLubyte* deviceName = glGetString(GL_RENDERER);

            if (checkOpenGLError() || !deviceName)
            {
                Log(Log::Level::WARN) << "Failed to get OpenGL renderer";
            }
            else
            {
                Log(Log::Level::INFO) << "Using " << reinterpret_cast<const char*>(deviceName) << " for rendering";
            }

            if (apiMajorVersion >= 3)
            {
#if OUZEL_OPENGL_INTERFACE_EGL
                genVertexArraysProc = reinterpret_cast<PFNGLGENVERTEXARRAYSOESPROC>(getProcAddress("glGenVertexArraysOES"));
                bindVertexArrayProc = reinterpret_cast<PFNGLBINDVERTEXARRAYOESPROC>(getProcAddress("glBindVertexArrayOES"));
                deleteVertexArraysProc = reinterpret_cast<PFNGLDELETEVERTEXARRAYSOESPROC>(getProcAddress("glDeleteVertexArraysOES"));
                mapBufferProc = reinterpret_cast<PFNGLMAPBUFFEROESPROC>(getProcAddress("glMapBufferOES"));
                unmapBufferProc = reinterpret_cast<PFNGLUNMAPBUFFEROESPROC>(getProcAddress("glUnmapBufferOES"));
                mapBufferRangeProc = reinterpret_cast<PFNGLMAPBUFFERRANGEEXTPROC>(getProcAddress("glMapBufferRangeEXT"));
                renderbufferStorageMultisampleProc = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>(getProcAddress("glRenderbufferStorageMultisampleIMG"));
                framebufferTexture2DMultisampleProc = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC>(getProcAddress("glFramebufferTexture2DMultisampleIMG"));
#endif // OUZEL_OPENGL_INTERFACE_EGL
            }
            else
            {
                npotTexturesSupported = false;
                multisamplingSupported = false;

                const GLubyte* extensionPtr = glGetString(GL_EXTENSIONS);

                if (checkOpenGLError() || !extensionPtr)
                {
                    Log(Log::Level::WARN) << "Failed to get OpenGL extensions";
                }
                else
                {
                    std::string extensions(reinterpret_cast<const char*>(extensionPtr));

                    Log(Log::Level::ALL) << "Supported OpenGL extensions: " << extensions;

                    std::istringstream extensionStringStream(extensions);

                    for (std::string extension; extensionStringStream >> extension;)
                    {
                        if (extension == "GL_OES_texture_npot" ||
                            extension == "GL_ARB_texture_non_power_of_two")
                        {
                            npotTexturesSupported = true;
                        }
#if OUZEL_OPENGL_INTERFACE_EAGL
                        if (extension == "GL_APPLE_framebuffer_multisample")
                        {
                            multisamplingSupported = true;
                        }
#elif OUZEL_OPENGL_INTERFACE_EGL
                        else if (extension == "GL_OES_vertex_array_object")
                        {
                            genVertexArraysProc = reinterpret_cast<PFNGLGENVERTEXARRAYSOESPROC>(getProcAddress("glGenVertexArraysOES"));
                            bindVertexArrayProc = reinterpret_cast<PFNGLBINDVERTEXARRAYOESPROC>(getProcAddress("glBindVertexArrayOES"));
                            deleteVertexArraysProc = reinterpret_cast<PFNGLDELETEVERTEXARRAYSOESPROC>(getProcAddress("glDeleteVertexArraysOES"));
                        }
                        else if (extension == "GL_OES_mapbuffer")
                        {
                            mapBufferProc = reinterpret_cast<PFNGLMAPBUFFEROESPROC>(getProcAddress("glMapBufferOES"));
                            unmapBufferProc = reinterpret_cast<PFNGLUNMAPBUFFEROESPROC>(getProcAddress("glUnmapBufferOES"));
                        }
                        else if (extension == "GL_EXT_map_buffer_range")
                        {
                            mapBufferRangeProc = reinterpret_cast<PFNGLMAPBUFFERRANGEEXTPROC>(getProcAddress("glMapBufferRangeEXT"));
                        }
                        else if (extension == "GL_IMG_multisampled_render_to_texture")
                        {
                            multisamplingSupported = true;
                            renderbufferStorageMultisampleProc = reinterpret_cast<PFNGLRENDERBUFFERSTORAGEMULTISAMPLEEXTPROC>(getProcAddress("glRenderbufferStorageMultisampleIMG"));
                            framebufferTexture2DMultisampleProc = reinterpret_cast<PFNGLFRAMEBUFFERTEXTURE2DMULTISAMPLEEXTPROC>(getProcAddress("glFramebufferTexture2DMultisampleIMG"));
                        }
#endif // OUZEL_OPENGL_INTERFACE_EGL
                    }

                    if (!multisamplingSupported)
                    {
                        sampleCount = 1;
                    }
                }
            }

            frameBufferWidth = static_cast<GLsizei>(newSize.v[0]);
            frameBufferHeight = static_cast<GLsizei>(newSize.v[1]);

            if (!createFrameBuffer())
            {
                return false;
            }

            std::shared_ptr<Shader> textureShader = std::make_shared<Shader>();

            switch (apiMajorVersion)
            {
                case 2:
#if OUZEL_SUPPORTS_OPENGL
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGL2_glsl), std::end(TexturePSGL2_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGL2_glsl), std::end(TextureVSGL2_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                   {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#elif OUZEL_SUPPORTS_OPENGLES
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGLES2_glsl), std::end(TexturePSGLES2_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGLES2_glsl), std::end(TextureVSGLES2_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                   {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#endif
                    break;
                case 3:
#if OUZEL_SUPPORTS_OPENGL
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGL3_glsl), std::end(TexturePSGL3_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGL3_glsl), std::end(TextureVSGL3_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                   {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#elif OUZEL_SUPPORTS_OPENGLES
                    textureShader->initFromBuffers(std::vector<uint8_t>(std::begin(TexturePSGLES3_glsl), std::end(TexturePSGLES3_glsl)),
                                                   std::vector<uint8_t>(std::begin(TextureVSGLES3_glsl), std::end(TextureVSGLES3_glsl)),
                                                   VertexPCT::ATTRIBUTES,
                                                   {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                   {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#endif
                    break;
                default:
                    Log(Log::Level::ERR) << "Unsupported OpenGL version";
                    return false;
            }

            sharedEngine->getCache()->setShader(SHADER_TEXTURE, textureShader);

            std::shared_ptr<Shader> colorShader = std::make_shared<Shader>();

            switch (apiMajorVersion)
            {
                case 2:
#if OUZEL_SUPPORTS_OPENGL
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGL2_glsl), std::end(ColorPSGL2_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGL2_glsl), std::end(ColorVSGL2_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                 {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#elif OUZEL_SUPPORTS_OPENGLES
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGLES2_glsl), std::end(ColorPSGLES2_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGLES2_glsl), std::end(ColorVSGLES2_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                 {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#endif
                    break;
                case 3:
#if OUZEL_SUPPORTS_OPENGL
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGL3_glsl), std::end(ColorPSGL3_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGL3_glsl), std::end(ColorVSGL3_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                 {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#elif OUZEL_SUPPORTS_OPENGLES
                    colorShader->initFromBuffers(std::vector<uint8_t>(std::begin(ColorPSGLES3_glsl), std::end(ColorPSGLES3_glsl)),
                                                 std::vector<uint8_t>(std::begin(ColorVSGLES3_glsl), std::end(ColorVSGLES3_glsl)),
                                                 VertexPC::ATTRIBUTES,
                                                 {{"color", Shader::DataType::FLOAT_VECTOR4}},
                                                 {{"modelViewProj", Shader::DataType::FLOAT_MATRIX4}});
#endif
                    break;
                default:
                    Log(Log::Level::ERR) << "Unsupported OpenGL version";
                    return false;
            }

            sharedEngine->getCache()->setShader(SHADER_COLOR, colorShader);

            std::shared_ptr<BlendState> noBlendState = std::make_shared<BlendState>();

            noBlendState->init(false,
                               BlendState::BlendFactor::ONE, BlendState::BlendFactor::ZERO,
                               BlendState::BlendOperation::ADD,
                               BlendState::BlendFactor::ONE, BlendState::BlendFactor::ZERO,
                               BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_NO_BLEND, noBlendState);

            std::shared_ptr<BlendState> addBlendState = std::make_shared<BlendState>();

            addBlendState->init(true,
                                BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                BlendState::BlendOperation::ADD,
                                BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_ADD, addBlendState);

            std::shared_ptr<BlendState> multiplyBlendState = std::make_shared<BlendState>();

            multiplyBlendState->init(true,
                                     BlendState::BlendFactor::DEST_COLOR, BlendState::BlendFactor::ZERO,
                                     BlendState::BlendOperation::ADD,
                                     BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                     BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_MULTIPLY, multiplyBlendState);

            std::shared_ptr<BlendState> alphaBlendState = std::make_shared<BlendState>();

            alphaBlendState->init(true,
                                  BlendState::BlendFactor::SRC_ALPHA, BlendState::BlendFactor::INV_SRC_ALPHA,
                                  BlendState::BlendOperation::ADD,
                                  BlendState::BlendFactor::ONE, BlendState::BlendFactor::ONE,
                                  BlendState::BlendOperation::ADD);

            sharedEngine->getCache()->setBlendState(BLEND_ALPHA, alphaBlendState);

            std::shared_ptr<Texture> whitePixelTexture = std::make_shared<Texture>();
            whitePixelTexture->initFromBuffer({255, 255, 255, 255}, Size2(1.0f, 1.0f), false, false);
            sharedEngine->getCache()->setTexture(TEXTURE_WHITE_PIXEL, whitePixelTexture);

            glDepthFunc(GL_LEQUAL);

            if (RendererOGL::checkOpenGLError())
            {
                Log() << "Failed to set depth function";
                return false;
            }

#if OUZEL_SUPPORTS_OPENGL
            if (sampleCount > 1)
            {
                glEnable(GL_MULTISAMPLE);

                if (RendererOGL::checkOpenGLError())
                {
                    Log() << "Failed to enable multi-sampling";
                    return false;
                }
            }
#endif

            return true;
        }

        bool RendererOGL::update()
        {
            clearMask = 0;
            if (uploadData.clearColorBuffer) clearMask |= GL_COLOR_BUFFER_BIT;
            if (uploadData.clearDepthBuffer) clearMask |= GL_DEPTH_BUFFER_BIT;

            frameBufferClearColor[0] = uploadData.clearColor.normR();
            frameBufferClearColor[1] = uploadData.clearColor.normG();
            frameBufferClearColor[2] = uploadData.clearColor.normB();
            frameBufferClearColor[3] = uploadData.clearColor.normA();

            if (frameBufferWidth != static_cast<GLsizei>(uploadData.size.v[0]) ||
                frameBufferHeight != static_cast<GLsizei>(uploadData.size.v[1]))
            {
                frameBufferWidth = static_cast<GLsizei>(uploadData.size.v[0]);
                frameBufferHeight = static_cast<GLsizei>(uploadData.size.v[1]);

                if (!createFrameBuffer())
                {
                    return false;
                }

#if OUZEL_PLATFORM_IOS || OUZEL_PLATFORM_TVOS
                Size2 backBufferSize = Size2(static_cast<float>(frameBufferWidth),
                                             static_cast<float>(frameBufferHeight));

                window->setSize(backBufferSize);
#endif
            }

            return true;
        }

        bool RendererOGL::process()
        {
            if (!lockContext())
            {
                return false;
            }

            return Renderer::process();
        }

        bool RendererOGL::draw()
        {
            if (drawQueue.empty())
            {
                frameBufferClearedFrame = currentFrame;

                if (clearMask)
                {
                    if (!bindFrameBuffer(frameBufferId))
                    {
                        return false;
                    }

                    if (!setViewport(0, 0,
                                     frameBufferWidth,
                                     frameBufferHeight))
                    {
                        return false;
                    }

                    glClearColor(frameBufferClearColor[0],
                                 frameBufferClearColor[1],
                                 frameBufferClearColor[2],
                                 frameBufferClearColor[3]);

                    glClear(clearMask);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to clear frame buffer";
                        return false;
                    }
                }

                if (!swapBuffers())
                {
                    return false;
                }
            }
            else for (const DrawCommand& drawCommand : drawQueue)
            {
#ifdef OUZEL_SUPPORTS_OPENGL
                setPolygonFillMode(drawCommand.wireframe ? GL_LINE : GL_FILL);
#else
                if (drawCommand.wireframe)
                {
                    continue;
                }
#endif

                // blend state
                BlendStateOGL* blendStateOGL = static_cast<BlendStateOGL*>(drawCommand.blendState);

                if (!blendStateOGL)
                {
                    // don't render if invalid blend state
                    continue;
                }

                if (!setBlendState(blendStateOGL->isGLBlendEnabled(),
                                   blendStateOGL->getModeRGB(),
                                   blendStateOGL->getModeAlpha(),
                                   blendStateOGL->getSourceFactorRGB(),
                                   blendStateOGL->getDestFactorRGB(),
                                   blendStateOGL->getSourceFactorAlpha(),
                                   blendStateOGL->getDestFactorAlpha()))
                {
                    return false;
                }

                // textures
                for (uint32_t layer = 0; layer < Texture::LAYERS; ++layer)
                {
                    TextureOGL* textureOGL = nullptr;

                    if (drawCommand.textures.size() > layer)
                    {
                        textureOGL = static_cast<TextureOGL*>(drawCommand.textures[layer]);
                    }

                    if (textureOGL)
                    {
                        if (!textureOGL->getTextureId())
                        {
                            return false;
                        }

                        if (!bindTexture(textureOGL->getTextureId(), layer))
                        {
                            return false;
                        }
                    }
                    else
                    {
                        if (!bindTexture(0, layer))
                        {
                            return false;
                        }
                    }
                }

                // shader
                ShaderOGL* shaderOGL = static_cast<ShaderOGL*>(drawCommand.shader);

                if (!shaderOGL || !shaderOGL->getProgramId())
                {
                    // don't render if invalid shader
                    continue;
                }

                useProgram(shaderOGL->getProgramId());

                // pixel shader constants
                const std::vector<ShaderOGL::Location>& pixelShaderConstantLocations = shaderOGL->getPixelShaderConstantLocations();

                if (drawCommand.pixelShaderConstants.size() > pixelShaderConstantLocations.size())
                {
                    Log(Log::Level::ERR) << "Invalid pixel shader constant size";
                    return false;
                }

                for (size_t i = 0; i < drawCommand.pixelShaderConstants.size(); ++i)
                {
                    const ShaderOGL::Location& pixelShaderConstantLocation = pixelShaderConstantLocations[i];
                    const std::vector<float>& pixelShaderConstant = drawCommand.pixelShaderConstants[i];

                    switch (pixelShaderConstantLocation.dataType)
                    {
                        case Shader::DataType::FLOAT:
                            glUniform1fv(pixelShaderConstantLocation.location, 1, pixelShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR2:
                            glUniform2fv(pixelShaderConstantLocation.location, 1, pixelShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR3:
                            glUniform3fv(pixelShaderConstantLocation.location, 1, pixelShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR4:
                            glUniform4fv(pixelShaderConstantLocation.location, 1, pixelShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_MATRIX3:
                            glUniformMatrix3fv(pixelShaderConstantLocation.location, 1, GL_FALSE, pixelShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_MATRIX4:
                            glUniformMatrix4fv(pixelShaderConstantLocation.location, 1, GL_FALSE, pixelShaderConstant.data());
                            break;
                        default:
                            Log(Log::Level::ERR) << "Unsupported uniform size";
                            return false;
                    }
                }

                // vertex shader constants
                const std::vector<ShaderOGL::Location>& vertexShaderConstantLocations = shaderOGL->getVertexShaderConstantLocations();

                if (drawCommand.vertexShaderConstants.size() > vertexShaderConstantLocations.size())
                {
                    Log(Log::Level::ERR) << "Invalid vertex shader constant size";
                    return false;
                }

                for (size_t i = 0; i < drawCommand.vertexShaderConstants.size(); ++i)
                {
                    const ShaderOGL::Location& vertexShaderConstantLocation = vertexShaderConstantLocations[i];
                    const std::vector<float>& vertexShaderConstant = drawCommand.vertexShaderConstants[i];

                    switch (vertexShaderConstantLocation.dataType)
                    {
                        case Shader::DataType::FLOAT:
                            glUniform1fv(vertexShaderConstantLocation.location, 1, vertexShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR2:
                            glUniform2fv(vertexShaderConstantLocation.location, 1, vertexShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR3:
                            glUniform3fv(vertexShaderConstantLocation.location, 1, vertexShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_VECTOR4:
                            glUniform4fv(vertexShaderConstantLocation.location, 1, vertexShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_MATRIX3:
                            glUniformMatrix3fv(vertexShaderConstantLocation.location, 1, GL_FALSE, vertexShaderConstant.data());
                            break;
                        case Shader::DataType::FLOAT_MATRIX4:
                            glUniformMatrix4fv(vertexShaderConstantLocation.location, 1, GL_FALSE, vertexShaderConstant.data());
                            break;
                        default:
                            Log(Log::Level::ERR) << "Unsupported uniform size";
                            return false;
                    }
                }

                // render target
                GLuint newFrameBufferId = 0;
                GLbitfield newClearMask = 0;
                const float* newClearColor = frameBufferClearColor;

                if (drawCommand.renderTarget)
                {
                    TextureOGL* renderTargetOGL = static_cast<TextureOGL*>(drawCommand.renderTarget);

                    if (!renderTargetOGL->getFrameBufferId())
                    {
                        continue;
                    }

                    newFrameBufferId = renderTargetOGL->getFrameBufferId();

                    if (renderTargetOGL->getFrameBufferClearedFrame() != currentFrame)
                    {
                        renderTargetOGL->setFrameBufferClearedFrame(currentFrame);
                        newClearMask = renderTargetOGL->getClearMask();
                        newClearColor = renderTargetOGL->getFrameBufferClearColor();
                    }
                }
                else
                {
                    newFrameBufferId = frameBufferId;

                    if (frameBufferClearedFrame != currentFrame)
                    {
                        frameBufferClearedFrame = currentFrame;
                        newClearMask = clearMask;
                        newClearColor = frameBufferClearColor;
                    }
                }

                if (!bindFrameBuffer(newFrameBufferId))
                {
                    return false;
                }

                setViewport(static_cast<GLint>(drawCommand.viewport.position.v[0]),
                            static_cast<GLint>(drawCommand.viewport.position.v[1]),
                            static_cast<GLsizei>(drawCommand.viewport.size.v[0]),
                            static_cast<GLsizei>(drawCommand.viewport.size.v[1]));

                if (newClearMask)
                {
                    if (newClearMask & GL_DEPTH_BUFFER_BIT)
                    {
                        // allow clearing the depth buffer
                        depthMask(true);
                        glClearDepthf(1.0f);
                    }

                    if (newClearMask & GL_COLOR_BUFFER_BIT)
                    {
                        glClearColor(newClearColor[0],
                                     newClearColor[1],
                                     newClearColor[2],
                                     newClearColor[3]);
                    }

                    glClear(newClearMask);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to clear frame buffer";
                        return false;
                    }
                }

                enableDepthTest(drawCommand.depthTest);

                GLint writeMask;
                glGetIntegerv(GL_DEPTH_WRITEMASK, &writeMask);
                depthMask(drawCommand.depthWrite);

                // scissor test
                setScissorTest(drawCommand.scissorTestEnabled,
                               static_cast<GLint>(drawCommand.scissorTest.position.v[0]),
                               static_cast<GLint>(drawCommand.scissorTest.position.v[1]),
                               static_cast<GLsizei>(drawCommand.scissorTest.size.v[0]),
                               static_cast<GLsizei>(drawCommand.scissorTest.size.v[1]));

                // mesh buffer
                MeshBufferOGL* meshBufferOGL = static_cast<MeshBufferOGL*>(drawCommand.meshBuffer);

                if (!meshBufferOGL)
                {
                    // don't render if invalid mesh buffer
                    continue;
                }

                BufferOGL* indexBufferOGL = static_cast<BufferOGL*>(meshBufferOGL->getIndexBuffer());
                BufferOGL* vertexBufferOGL = static_cast<BufferOGL*>(meshBufferOGL->getVertexBuffer());

                if (!indexBufferOGL || !indexBufferOGL->getBufferId() ||
                    !vertexBufferOGL || !vertexBufferOGL->getBufferId())
                {
                    continue;
                }

                // draw
                GLenum mode;

                switch (drawCommand.drawMode)
                {
                    case DrawMode::POINT_LIST: mode = GL_POINTS; break;
                    case DrawMode::LINE_LIST: mode = GL_LINES; break;
                    case DrawMode::LINE_STRIP: mode = GL_LINE_STRIP; break;
                    case DrawMode::TRIANGLE_LIST: mode = GL_TRIANGLES; break;
                    case DrawMode::TRIANGLE_STRIP: mode = GL_TRIANGLE_STRIP; break;
                    default: Log(Log::Level::ERR) << "Invalid draw mode"; return false;
                }

                if (!meshBufferOGL->bindBuffers())
                {
                    return false;
                }

                glDrawElements(mode,
                               static_cast<GLsizei>(drawCommand.indexCount),
                               meshBufferOGL->getIndexType(),
                               static_cast<const char*>(nullptr) + (drawCommand.startIndex * meshBufferOGL->getBytesPerIndex()));

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to draw elements";
                    return false;
                }
            }

#if OUZEL_SUPPORTS_OPENGL
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, systemFrameBufferId); // draw to default frame buffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, frameBufferId); // read from FBO
            glDrawBuffer(GL_BACK); // set the back buffer as the draw buffer

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to bind frame buffer";
                return false;
            }

            glBlitFramebuffer(0, 0, frameBufferWidth, frameBufferHeight,
                              0, 0, frameBufferWidth, frameBufferHeight,
                              GL_COLOR_BUFFER_BIT, GL_NEAREST);

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to blit framebuffer";
                return false;
            }

            // reset framebuffer
            glBindFramebuffer(GL_READ_FRAMEBUFFER, systemFrameBufferId);
            stateCache.frameBufferId = systemFrameBufferId;
#endif

            if (!swapBuffers())
            {
                return false;
            }

            return true;
        }

        bool RendererOGL::lockContext()
        {
            return true;
        }

        bool RendererOGL::swapBuffers()
        {
            return true;
        }

        std::vector<Size2> RendererOGL::getSupportedResolutions() const
        {
            return std::vector<Size2>();
        }

        BlendStateResource* RendererOGL::createBlendState()
        {
            std::lock_guard<std::mutex> lock(resourceMutex);

            BlendStateResource* blendState = new BlendStateOGL();
            return blendState;
        }

        TextureResource* RendererOGL::createTexture()
        {
            std::lock_guard<std::mutex> lock(resourceMutex);

            TextureResource* texture = new TextureOGL();
            return texture;
        }

        ShaderResource* RendererOGL::createShader()
        {
            std::lock_guard<std::mutex> lock(resourceMutex);

            ShaderResource* shader = new ShaderOGL();
            return shader;
        }

        MeshBufferResource* RendererOGL::createMeshBuffer()
        {
            std::lock_guard<std::mutex> lock(resourceMutex);

            MeshBufferResource* meshBuffer = new MeshBufferOGL();
            return meshBuffer;
        }

        BufferResource* RendererOGL::createBuffer()
        {
            std::lock_guard<std::mutex> lock(resourceMutex);

            BufferResource* buffer = new BufferOGL();
            return buffer;
        }

        bool RendererOGL::generateScreenshot(const std::string& filename)
        {
            bindFrameBuffer(systemFrameBufferId);

            const GLsizei width = frameBufferWidth;
            const GLsizei height = frameBufferHeight;
            const GLsizei depth = 4;

            std::vector<uint8_t> data(static_cast<size_t>(width * height * depth));

            glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data.data());

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to read pixels from frame buffer";
                return false;
            }

            uint8_t temp;
            for (GLsizei row = 0; row < height / 2; ++row)
            {
                for (GLsizei col = 0; col < width; ++col)
                {
                    for (GLsizei z = 0; z < depth; ++z)
                    {
                        temp = data[static_cast<size_t>(((height - row - 1) * width + col) * depth + z)];
                        data[static_cast<size_t>(((height - row - 1) * width + col) * depth + z)] = data[static_cast<size_t>((row * width + col) * depth + z)];
                        data[static_cast<size_t>((row * width + col) * depth + z)] = temp;
                    }
                }
            }

            if (!stbi_write_png(filename.c_str(), width, height, depth, data.data(), width * depth))
            {
                Log(Log::Level::ERR) << "Failed to save image to file";
                return false;
            }

            return true;
        }

        bool RendererOGL::createFrameBuffer()
        {
#if !OUZEL_OPENGL_INTERFACE_EGL // don't create MSAA frambeuffer for EGL target
            if (!frameBufferId)
            {
                glGenFramebuffers(1, &frameBufferId);
            }

            if (checkOpenGLError())
            {
                Log(Log::Level::ERR) << "Failed to generate frame buffer object";
                return false;
            }

            if (sampleCount > 1)
            {
                if (!colorRenderBufferId) glGenRenderbuffers(1, &colorRenderBufferId);

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to create render buffer";
                    return false;
                }

                glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBufferId);

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to bind render buffer";
                    return false;
                }

    #if OUZEL_SUPPORTS_OPENGL
                glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, GL_RGBA, frameBufferWidth, frameBufferHeight);
    #elif OUZEL_OPENGL_INTERFACE_EAGL
                glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, sampleCount, GL_RGBA8_OES, frameBufferWidth, frameBufferHeight);
    #elif OUZEL_OPENGL_INTERFACE_EGL
                renderbufferStorageMultisampleIMG(GL_RENDERBUFFER, sampleCount, GL_RGBA, frameBufferWidth, frameBufferHeight);
    #endif

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to create color render buffer";
                    return false;
                }

                if (depth)
                {
#ifdef OUZEL_SUPPORTS_OPENGL
                    GLuint depthFormat = GL_DEPTH_COMPONENT24;
#elif OUZEL_SUPPORTS_OPENGLES
                    GLuint depthFormat = GL_DEPTH_COMPONENT24_OES;
#endif

                    if (!depthFormat)
                    {
                        Log(Log::Level::ERR) << "Unsupported depth buffer format";
                        return false;
                    }

                    if (!depthRenderBufferId) glGenRenderbuffers(1, &depthRenderBufferId);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to create render buffer";
                        return false;
                    }

                    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferId);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to bind render buffer";
                        return false;
                    }

    #if OUZEL_SUPPORTS_OPENGL
                    glRenderbufferStorageMultisample(GL_RENDERBUFFER, sampleCount, depthFormat, frameBufferWidth, frameBufferHeight);
    #elif OUZEL_OPENGL_INTERFACE_EAGL
                    glRenderbufferStorageMultisampleAPPLE(GL_RENDERBUFFER, sampleCount, depthFormat, frameBufferWidth, frameBufferHeight);
    #elif OUZEL_OPENGL_INTERFACE_EGL
                    renderbufferStorageMultisampleIMG(GL_RENDERBUFFER, sampleCount, depthFormat, frameBufferWidth, frameBufferHeight);
    #endif

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to create depth render buffer";
                        return false;
                    }
                }

                bindFrameBuffer(frameBufferId);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBufferId);

                if (depth)
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId);
                }

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    Log(Log::Level::ERR) << "Failed to create frame buffer object " << glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    return false;
                }
            }
#endif // !OUZEL_OPENGL_INTERFACE_EGL
#if OUZEL_SUPPORTS_OPENGL // create frambeuffer only for OpenGL targets
            else
            {

                if (!colorRenderBufferId) glGenRenderbuffers(1, &colorRenderBufferId);

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to create render buffer";
                    return false;
                }

                glBindRenderbuffer(GL_RENDERBUFFER, colorRenderBufferId);

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to bind render buffer";
                    return false;
                }
                
                glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, frameBufferWidth, frameBufferHeight);

                if (checkOpenGLError())
                {
                    Log(Log::Level::ERR) << "Failed to create color render buffer";
                    return false;
                }

                if (depth)
                {
#ifdef OUZEL_SUPPORTS_OPENGL
                    GLuint depthFormat = GL_DEPTH_COMPONENT24;
#elif OUZEL_SUPPORTS_OPENGLES
                    GLuint depthFormat = GL_DEPTH_COMPONENT24_OES;
#endif

                    if (!depthFormat)
                    {
                        Log(Log::Level::ERR) << "Unsupported depth buffer format";
                        return false;
                    }

                    if (!depthRenderBufferId) glGenRenderbuffers(1, &depthRenderBufferId);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to create render buffer";
                        return false;
                    }

                    glBindRenderbuffer(GL_RENDERBUFFER, depthRenderBufferId);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to bind render buffer";
                        return false;
                    }

                    glRenderbufferStorage(GL_RENDERBUFFER, depthFormat, frameBufferWidth, frameBufferHeight);

                    if (checkOpenGLError())
                    {
                        Log(Log::Level::ERR) << "Failed to create depth render buffer";
                        return false;
                    }
                }

                bindFrameBuffer(frameBufferId);
                glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderBufferId);

                if (depth)
                {
                    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderBufferId);
                }

                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
                {
                    Log(Log::Level::ERR) << "Failed to create framebuffer object %x", glCheckFramebufferStatus(GL_FRAMEBUFFER);
                    return false;
                }
            }
#endif // OUZEL_SUPPORTS_OPENGL

            return true;
        }

        void* RendererOGL::getProcAddress(const std::string& name) const
        {
#if OUZEL_PLATFORM_MACOS
            NSSymbol symbol = nullptr;
            std::string symbolName = "_" + name;
            if (NSIsSymbolNameDefined(symbolName.c_str()))
                symbol = NSLookupAndBindSymbol(symbolName.c_str());
            return symbol ? NSAddressOfSymbol(symbol) : nullptr;
#elif OUZEL_PLATFORM_LINUX
            return reinterpret_cast<void*>(glXGetProcAddress(reinterpret_cast<const GLubyte*>(name.c_str())));
#elif OUZEL_OPENGL_INTERFACE_EGL
            return reinterpret_cast<void*>(eglGetProcAddress(name.c_str()));
#else
            OUZEL_UNUSED(name);
            return nullptr;
#endif
        }

        RendererOGL::StateCache RendererOGL::stateCache;
    } // namespace graphics
} // namespace ouzel
