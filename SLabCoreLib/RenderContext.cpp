/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

RenderContext::RenderContext(
    std::function<void()> makeRenderContextCurrentFunction,
    std::function<void()> swapRenderBuffersFunction)
    : mMakeRenderContextCurrentFunction(std::move(makeRenderContextCurrentFunction))
    , mSwapRenderBuffersFunction(std::move(swapRenderBuffersFunction))
    , mViewModel(1.0f, vec2f::zero(), 100, 100)
{
    GLuint tmpGLuint;

    //
    // Initialize OpenGL
    //

    // Make render context current
    mMakeRenderContextCurrentFunction();

    // Initialize OpenGL
    try
    {
        SLabOpenGL::InitOpenGL();
    }
    catch (std::exception const & e)
    {
        throw std::runtime_error("Error during OpenGL initialization: " + std::string(e.what()));
    }

    ////////////////////////////////////////////////////////////////
    // Initialize shaders, VAO's, and VBOs
    ////////////////////////////////////////////////////////////////

    mShaderManager = ShaderManager::CreateInstance();

    //
    // Points
    //

    mPointVertexCount = 0;

    glGenVertexArrays(1, &tmpGLuint);
    mPointVAO = tmpGLuint;

    glBindVertexArray(*mPointVAO);

    glGenBuffers(1, &tmpGLuint);
    mPointVertexVBO = tmpGLuint;
    glBindBuffer(GL_ARRAY_BUFFER, *mPointVertexVBO);

    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup1));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup1), 4, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup2));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void *)(4 * sizeof(float)));

    glBindVertexArray(0);

    ////////////////////////////////////////////////////////////////
    // Initialize global settings
    ////////////////////////////////////////////////////////////////

    // Enable blend for alpha transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Disable depth test
    glDisable(GL_DEPTH_TEST);

    ////////////////////////////////////////////////////////////////
    // Set parameters in all shaders
    ////////////////////////////////////////////////////////////////

    OnViewModelUpdated();
}

RgbImageData RenderContext::TakeScreenshot()
{
    //
    // Flush draw calls
    //

    glFinish();

    //
    // Allocate buffer
    //

    int const canvasWidth = mViewModel.GetCanvasWidth();
    int const canvasHeight = mViewModel.GetCanvasHeight();

    auto pixelBuffer = std::make_unique<rgbColor[]>(canvasWidth * canvasHeight);

    //
    // Read pixels
    //

    // Alignment is byte
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    CheckOpenGLError();

    // Read the front buffer
    glReadBuffer(GL_FRONT);
    CheckOpenGLError();

    // Read
    glReadPixels(0, 0, canvasWidth, canvasHeight, GL_RGB, GL_UNSIGNED_BYTE, pixelBuffer.get());
    CheckOpenGLError();

    return RgbImageData(
        ImageSize(canvasWidth, canvasHeight),
        std::move(pixelBuffer));
}

void RenderContext::RenderStart()
{
    // Set polygon mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Clear canvas - and depth buffer
    vec3f const clearColor(1.0f, 1.0f, 1.0f);
    glClearColor(clearColor.x, clearColor.y, clearColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset all counts
    mPointVertexCount = 0;
}

void RenderContext::UploadPoints(
    size_t pointCount,
    vec2f const * pointPositions,
    vec4f const * pointColors,
    float const * pointNormRadii)
{
    //
    // Map buffer
    //

    glBindBuffer(GL_ARRAY_BUFFER, *mPointVertexVBO);

    // Check whether we need to re-allocate the buffers
    if (pointCount * 6 != mPointVertexCount)
    {
        mPointVertexCount = pointCount * 6;

        glBufferData(GL_ARRAY_BUFFER, mPointVertexCount * sizeof(PointVertex), nullptr, GL_STREAM_DRAW);
        CheckOpenGLError();
    }

    mPointVertexBuffer.map(mPointVertexCount);


    //
    // Upload buffer
    //

    for (size_t p = 0; p < pointCount; ++p)
    {
        vec2f const & pointPosition = pointPositions[p];
        float const halfRadius = pointNormRadii[p] * 0.3f / 2.0f; // World size of a point radius
        vec4f const & pointColor = pointColors[p];

        float const xLeft = pointPosition.x - halfRadius;
        float const xRight = pointPosition.x + halfRadius;
        float const yTop = pointPosition.y + halfRadius;
        float const yBottom = pointPosition.y - halfRadius;

        // Left, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yBottom),
            vec2f(-1.0f, -1.0f),
            pointColor);

        // Left, top
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yTop),
            vec2f(-1.0f, 1.0f),
            pointColor);

        // Right, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yBottom),
            vec2f(1.0f, -1.0f),
            pointColor);

        // Left, top
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yTop),
            vec2f(-1.0f, 1.0f),
            pointColor);

        // Right, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yBottom),
            vec2f(1.0f, -1.0f),
            pointColor);

        // Right, top
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yTop),
            vec2f(1.0f, 1.0f),
            pointColor);
    }


    //
    // Unmap
    //

    mPointVertexBuffer.unmap();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::RenderEnd()
{
    ////////////////////////////////////////////////////////////////
    // Render points
    ////////////////////////////////////////////////////////////////

    glBindVertexArray(*mPointVAO);

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Points>();

    assert((mPointVertexCount % 6) == 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mPointVertexCount));

    glBindVertexArray(0);

    ////////////////////////////////////////////////////////////////
    // Terminate
    ////////////////////////////////////////////////////////////////

    // Flush all pending commands (but not the GPU buffer)
    SLabOpenGL::Flush();

    // Swap buffer
    mSwapRenderBuffersFunction();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void RenderContext::OnCanvasSizeUpdated()
{
    glViewport(0, 0, mViewModel.GetCanvasWidth(), mViewModel.GetCanvasHeight());
}

void RenderContext::OnViewModelUpdated()
{
    //
    // Update ortho matrix
    //

    ViewModel::ProjectionMatrix const & orthoMatrix = mViewModel.GetOrthoMatrix();

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Points>();
    mShaderManager->SetProgramParameter<ShaderManager::ProgramType::Points, ShaderManager::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
}