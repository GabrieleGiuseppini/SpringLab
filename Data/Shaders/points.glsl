###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inPointAttributeGroup1; // QuadCorner, PointCenter
in vec4 inPointAttributeGroup2; // Color

// Outputs        
out vec2 pointCenter;
out vec4 pointColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{  
    pointCenter = inPointAttributeGroup1.zw;
    pointColor = inPointAttributeGroup2;

    gl_Position = paramOrthoMatrix * vec4(inPointAttributeGroup1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec2 pointCenter;
in vec4 pointColor;

void main()
{
    // TODO
    gl_FragColor = pointColor;
} 
