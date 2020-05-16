###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inPointAttributeGroup1; // Position, VertexSpacePosition
in vec4 inPointAttributeGroup2; // Color

// Outputs        
out vec2 vertexSpacePosition;
out vec4 pointColor;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{  
    vertexSpacePosition = inPointAttributeGroup1.zw;
    pointColor = inPointAttributeGroup2;

    gl_Position = paramOrthoMatrix * vec4(inPointAttributeGroup1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in vec4 pointColor;

void main()
{
    float d1 = distance(vertexSpacePosition, vec2(.0, .0));
    float alpha = 1.0 - smoothstep(0.85, 1.0, d1);

    float d2 = distance(vertexSpacePosition, vec2(-0.3, 0.3));
    float reflection = 1.0 - smoothstep(0.0, 0.5, d2);

    gl_FragColor = vec4(
        mix(
            vec3(pointColor.xyz), 
            vec3(1., 1.,1.),
            reflection),
        alpha * pointColor.w);
} 
