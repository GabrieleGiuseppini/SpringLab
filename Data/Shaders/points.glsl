###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inPointAttributeGroup1; // Position, VertexSpacePosition
in vec4 inPointAttributeGroup2; // Color
in vec2 inPointAttributeGroup3; // Highlight, Frozen

// Outputs        
out vec2 vertexSpacePosition;
out vec4 pointColor;
out float pointHighlight;
out float pointFrozen;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{  
    vertexSpacePosition = inPointAttributeGroup1.zw;
    pointColor = inPointAttributeGroup2;
    pointHighlight = inPointAttributeGroup3.x;
    pointFrozen = inPointAttributeGroup3.y;

    gl_Position = paramOrthoMatrix * vec4(inPointAttributeGroup1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in vec4 pointColor;
in float pointHighlight;
in float pointFrozen;

void main()
{
    float d1 = distance(vertexSpacePosition, vec2(.0, .0));
    float alpha = 1.0 - smoothstep(0.9, 1.0, d1);

    float d2 = distance(vertexSpacePosition, vec2(-0.3, 0.3));
    float reflectionRegion = 1.0 - smoothstep(0.0, 0.5, d2);

    float highlightRegion = smoothstep(0.8, 0.9, d1) - smoothstep(0.9, 1.0, d1);

    vec3 pointColor2 =  mix(
        vec3(pointColor.xyz) * pointFrozen, 
        vec3(1., 1., 1.),
        reflectionRegion);

    gl_FragColor = vec4(
        mix(pointColor2, 
            vec3(.55, .0, .0),
            highlightRegion * pointHighlight),
        alpha * pointColor.w);
} 
