###VERTEX

#version 120

#define in attribute
#define out varying

// Inputs
in vec4 inSpringAttributeGroup1; // Position, VertexSpacePosition
in vec4 inSpringAttributeGroup2; // Color
in float inSpringAttributeGroup3; // Highlight

// Outputs        
out vec2 vertexSpacePosition;
out vec4 springColor;
out float springHighlight;

// Params
uniform mat4 paramOrthoMatrix;

void main()
{  
    vertexSpacePosition = inSpringAttributeGroup1.zw;
    springColor = inSpringAttributeGroup2;
    springHighlight = inSpringAttributeGroup3;

    gl_Position = paramOrthoMatrix * vec4(inSpringAttributeGroup1.xy, -1.0, 1.0);
}

###FRAGMENT

#version 120

#define in varying

// Inputs from previous shader        
in vec2 vertexSpacePosition; // [(-1.0, -1.0), (1.0, 1.0)]
in vec4 springColor;
in float springHighlight;

void main()
{
    /* TODO
    float d1 = distance(vertexSpacePosition, vec2(.0, .0));
    float alpha = 1.0 - smoothstep(0.85, 1.0, d1);

    float d2 = distance(vertexSpacePosition, vec2(-0.3, 0.3));
    float reflection = 1.0 - smoothstep(0.0, 0.5, d2);

    float highlight = smoothstep(0.8, 0.9, d1) - smoothstep(0.9, 1.0, d1);

    vec3 pointColor2 =  mix(
        vec3(pointColor.xyz), 
        vec3(1., 1., 1.),
        reflection);

    gl_FragColor = vec4(
        mix(pointColor2, 
            vec3(.55, .0, .0),
            highlight * pointHighlight),
        alpha * pointColor.w);
    */

    gl_FragColor = vec4(
        mix(springColor.xyz, 
            vec3(.55, .0, .0),
            springHighlight),
        .1 * springColor.w);    
} 
