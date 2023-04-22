###VERTEX

#define in attribute
#define out varying

// Inputs
in vec2 inGridAttributeGroup1; // Vertex position (object space)

// Outputs
out vec2 vertexObjectSpaceCoords;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{
    vertexObjectSpaceCoords = inGridAttributeGroup1;
    gl_Position = paramOrthoMatrix * vec4(inGridAttributeGroup1, -1.0, 1.0);
}

###FRAGMENT

#define in varying

// Inputs from previous shader
in vec2 vertexObjectSpaceCoords;

// Parameters
uniform float paramWorldStep;
uniform vec2 paramPixelWorldWidth;

void main()
{
    // 0.0 at edge of grid, 1.0 at other side
    vec2 gridUnary = fract(vertexObjectSpaceCoords / paramWorldStep);
    
    // 1. on grid, 0. otherwise
    float gridDepth = 1.0 -
        (step(paramPixelWorldWidth.x * 0.51, gridUnary.x) - step(1.0 - paramPixelWorldWidth.x * 0.49, gridUnary.x))
        * (step(paramPixelWorldWidth.y * 0.51, gridUnary.y) - step(1.0 - paramPixelWorldWidth.y * 0.49, gridUnary.y));
    
    gl_FragColor = vec4(.7, .7, .7, 
        gridDepth);
} 
