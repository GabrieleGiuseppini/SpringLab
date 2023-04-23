/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "RenderContext.h"

#include <cmath>

RenderContext::RenderContext(
    int canvasWidth,
    int canvasHeight)
    : mShaderManager()
    , mViewModel(1.0f, vec2f::zero(), canvasWidth, canvasHeight)
    // Settings
    , mIsCanvasSizeDirty(true)
    , mIsViewModelDirty(true)
    , mIsGridDirty(true)
    ////
    , mIsGridEnabled(false)
{
    GLuint tmpGLuint;

    //
    // Initialize OpenGL
    //

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
    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup3));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::PointAttributeGroup3), 2, GL_FLOAT, GL_FALSE, sizeof(PointVertex), (void *)(8 * sizeof(float)));
    static_assert(sizeof(PointVertex) == 10 * sizeof(float));

    glBindVertexArray(0);

    //
    // Springs
    //

    glGenVertexArrays(1, &tmpGLuint);
    mSpringVAO = tmpGLuint;
    glBindVertexArray(*mSpringVAO);

    glGenBuffers(1, &tmpGLuint);
    mSpringVertexVBO = tmpGLuint;
    glBindBuffer(GL_ARRAY_BUFFER, *mSpringVertexVBO);

    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup1));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup1), 4, GL_FLOAT, GL_FALSE, sizeof(SpringVertex), (void *)0);
    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup2));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup2), 4, GL_FLOAT, GL_FALSE, sizeof(SpringVertex), (void *)(4 * sizeof(float)));
    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup3));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::SpringAttributeGroup3), 1, GL_FLOAT, GL_FALSE, sizeof(SpringVertex), (void *)(8 * sizeof(float)));

    glBindVertexArray(0);

    //
    // Grid
    //

    glGenVertexArrays(1, &tmpGLuint);
    mGridVAO = tmpGLuint;
    glBindVertexArray(*mGridVAO);

    glGenBuffers(1, &tmpGLuint);
    mGridVBO = tmpGLuint;
    glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);

    glEnableVertexAttribArray(static_cast<GLuint>(ShaderManager::VertexAttributeType::GridAttributeGroup1));
    glVertexAttribPointer(static_cast<GLuint>(ShaderManager::VertexAttributeType::GridAttributeGroup1), 2, GL_FLOAT, GL_FALSE, sizeof(GridVertex), (void *)0);

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

    ProcessSettingChanges();
}

RgbImageData RenderContext::TakeScreenshot()
{
    //
    // Allocate buffer
    //

    int const canvasWidth = mViewModel.GetCanvasWidth();
    int const canvasHeight = mViewModel.GetCanvasHeight();

    auto pixelBuffer = std::make_unique<rgbColor[]>(canvasWidth * canvasHeight);

    //
    // Take screenshot
    //

    //
    // Flush draw calls
    //

    glFinish();

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
    //vec3f constexpr ClearColor = rgbColor(0xca, 0xf4, 0xf4).toVec3f();
    vec3f constexpr ClearColor = rgbColor(0xff, 0xff, 0xff).toVec3f();
    glClearColor(ClearColor.x, ClearColor.y, ClearColor.z, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Reset all buffers
    mPointVertexCount = 0;

    // Process setting changes
    ProcessSettingChanges();
}

void RenderContext::UploadPoints(
    size_t pointCount,
    vec2f const * pointPositions,
    vec4f const * pointColors,
    float const * pointNormRadii,
    float const * pointHighlights,
    float const * pointFrozenCoefficients)
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

    float constexpr WorldRadius = 0.3f;

    for (size_t p = 0; p < pointCount; ++p)
    {
        vec2f const & pointPosition = pointPositions[p];
        float const halfRadius = pointNormRadii[p] * WorldRadius / 2.0f;
        vec4f const & pointColor = pointColors[p];
        float const pointHighlight = pointHighlights[p];
        float const pointFrozenCoefficient = pointFrozenCoefficients[p];

        float const xLeft = pointPosition.x - halfRadius;
        float const xRight = pointPosition.x + halfRadius;
        float const yTop = pointPosition.y + halfRadius;
        float const yBottom = pointPosition.y - halfRadius;

        // Left, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yBottom),
            vec2f(-1.0f, -1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);

        // Left, top
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yTop),
            vec2f(-1.0f, 1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);

        // Right, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yBottom),
            vec2f(1.0f, -1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);

        // Left, top
        mPointVertexBuffer.emplace_back(
            vec2f(xLeft, yTop),
            vec2f(-1.0f, 1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);

        // Right, bottom
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yBottom),
            vec2f(1.0f, -1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);

        // Right, top
        mPointVertexBuffer.emplace_back(
            vec2f(xRight, yTop),
            vec2f(1.0f, 1.0f),
            pointColor,
            pointHighlight,
            pointFrozenCoefficient);
    }


    //
    // Unmap buffer
    //

    assert(mPointVertexBuffer.size() == mPointVertexCount);

    mPointVertexBuffer.unmap();

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderContext::UploadSpringsStart(size_t springCount)
{
    //
    // Prepare buffer
    //

    mSpringVertexBuffer.clear();
    mSpringVertexBuffer.reserve(springCount * 6);
}

void RenderContext::UploadSpring(
    vec2f const & springEndpointAPosition,
    vec2f const & springEndpointBPosition,
    vec4f const & springColor,
    float springNormThickness,
    float springHighlight)
{
    float constexpr WorldThickness = 0.1f;

    vec2f const springVector = springEndpointBPosition - springEndpointAPosition;
    vec2f const springNormal = springVector.to_perpendicular().normalise()
        * springNormThickness * WorldThickness / 2.0f;

    vec2f const bottomLeft = springEndpointAPosition - springNormal;
    vec2f const bottomRight = springEndpointAPosition + springNormal;
    vec2f const topLeft = springEndpointBPosition - springNormal;
    vec2f const topRight = springEndpointBPosition + springNormal;

    // Left, bottom
    mSpringVertexBuffer.emplace_back(
        bottomLeft,
        vec2f(-1.0f, -1.0f),
        springColor,
        springHighlight);

    // Left, top
    mSpringVertexBuffer.emplace_back(
        topLeft,
        vec2f(-1.0f, 1.0f),
        springColor,
        springHighlight);

    // Right, bottom
    mSpringVertexBuffer.emplace_back(
        bottomRight,
        vec2f(1.0f, -1.0f),
        springColor,
        springHighlight);

    // Left, top
    mSpringVertexBuffer.emplace_back(
        topLeft,
        vec2f(-1.0f, 1.0f),
        springColor,
        springHighlight);

    // Right, bottom
    mSpringVertexBuffer.emplace_back(
        bottomRight,
        vec2f(1.0f, -1.0f),
        springColor,
        springHighlight);

    // Right, top
    mSpringVertexBuffer.emplace_back(
        topRight,
        vec2f(1.0f, 1.0f),
        springColor,
        springHighlight);
}

void RenderContext::UploadSpringsEnd()
{
    //
    // Upload buffer, if needed
    //

    if (!mSpringVertexBuffer.empty())
    {
        glBindBuffer(GL_ARRAY_BUFFER, *mSpringVertexVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(SpringVertex) * mSpringVertexBuffer.size(), mSpringVertexBuffer.data(), GL_STREAM_DRAW);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }
}

void RenderContext::RenderEnd()
{
    ////////////////////////////////////////////////////////////////
    // Render springs
    ////////////////////////////////////////////////////////////////

    if (!mSpringVertexBuffer.empty())
    {
        glBindVertexArray(*mSpringVAO);

        mShaderManager->ActivateProgram<ShaderManager::ProgramType::Springs>();

        assert((mSpringVertexBuffer.size() % 6) == 0);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mSpringVertexBuffer.size()));

        CheckOpenGLError();

        glBindVertexArray(0);
    }

    ////////////////////////////////////////////////////////////////
    // Render points
    ////////////////////////////////////////////////////////////////

    glBindVertexArray(*mPointVAO);

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Points>();

    assert((mPointVertexCount % 6) == 0);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mPointVertexCount));

    CheckOpenGLError();

    glBindVertexArray(0);

    ////////////////////////////////////////////////////////////////
    // Grid
    ////////////////////////////////////////////////////////////////
    
    if (mIsGridEnabled)
    {
        glBindVertexArray(*mGridVAO);

        mShaderManager->ActivateProgram<ShaderManager::ProgramType::Grid>();

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        CheckOpenGLError();

        glBindVertexArray(0);
    }

    ////////////////////////////////////////////////////////////////
    // Terminate
    ////////////////////////////////////////////////////////////////

    // Flush all pending commands (but not the GPU buffer)
    SLabOpenGL::Flush();
}

//////////////////////////////////////////////////////////////////////////////////////////////

void RenderContext::ProcessSettingChanges()
{
    if (mIsCanvasSizeDirty)
    {
        OnCanvasSizeUpdated();
        mIsCanvasSizeDirty = false;
    }

    if (mIsViewModelDirty)
    {
        OnViewModelUpdated();
        mIsGridDirty = true;
        mIsViewModelDirty = false;
    }

    if (mIsGridDirty)
    {
        OnGridUpdated();
        mIsGridDirty = false;
    }
}

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

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Springs>();
    mShaderManager->SetProgramParameter<ShaderManager::ProgramType::Springs, ShaderManager::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ShaderManager::ProgramType::Grid, ShaderManager::ProgramParameterType::OrthoMatrix>(
        orthoMatrix);
}

void RenderContext::OnGridUpdated()
{
    //
    // Calculate vertex attributes
    //

    // Visible world coordinates
    vec2f const visibleWorldTopLeft = mViewModel.GetVisibleWorldTopLeft();
    vec2f const visibleWorldBottomRight = mViewModel.GetVisibleWorldBottomRight();

    // Vertices

    std::array<GridVertex, 4> vertexBuffer;

    // Bottom-left
    vertexBuffer[0] = GridVertex(
        vec2f(
            visibleWorldTopLeft.x,
            visibleWorldBottomRight.y));

    // Top left
    vertexBuffer[1] = GridVertex(
        visibleWorldTopLeft);

    // Bottom-right
    vertexBuffer[2] = GridVertex(
        visibleWorldBottomRight);

    // Top-right
    vertexBuffer[3] = GridVertex(
        vec2f(
            visibleWorldBottomRight.x,
            visibleWorldTopLeft.y));

    // Upload vertices
    glBindBuffer(GL_ARRAY_BUFFER, *mGridVBO);
    glBufferData(GL_ARRAY_BUFFER, vertexBuffer.size() * sizeof(GridVertex), vertexBuffer.data(), GL_STATIC_DRAW);
    CheckOpenGLError();
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    //
    // Calculate aspect
    //

    float const pixelWorldWidth = mViewModel.ScreenOffsetToWorldOffset(vec2f(1.0f, -1.0f)).x; // x and y are the same here
    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ShaderManager::ProgramType::Grid, ShaderManager::ProgramParameterType::PixelWorldWidth>(
        pixelWorldWidth);

    int constexpr ExtraGridEnlargement = 2;
    float const worldStepSize = std::max(std::ldexp(1.0f, static_cast<int>(std::floor(std::log2f(pixelWorldWidth))) + 2 + ExtraGridEnlargement), 1.0f);

    mShaderManager->ActivateProgram<ShaderManager::ProgramType::Grid>();
    mShaderManager->SetProgramParameter<ShaderManager::ProgramType::Grid, ShaderManager::ProgramParameterType::WorldStep>(
        worldStepSize);
}