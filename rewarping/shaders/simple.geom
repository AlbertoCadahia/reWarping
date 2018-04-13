#version 420

highp in vec2 vTexCoord[];
highp in vec3 vColor[];

highp out vec2 gTexCoord;
highp out vec3 gColor;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;
layout (max_vertices = 4) out;

layout (binding = 1) uniform sampler2D depthTex;

uniform mat4 viewProj;
uniform vec3 eye;
uniform vec3 gridRight;
uniform vec3 gridUp;
uniform vec2 screenSizeInverse;
uniform float gridSize;

float threshold=8.0f;
float aHalf=0.5f;
float aThird=0.33333333f;
float aQuarter=0.25f;

float quadFactor= 1.1f;
float background= 9000000.0f;

//method that calculates the average of 4 pixels depth and coordinate textures
float  calculateNeighboursAverage(float origin,float depth0, float depth1, float diagonal, vec2 originTexCoord, vec2 neightbour0TexCoord, vec2 neightbour1TexCoord, vec2 diagonalTexCoord)
{

    if(abs(origin-depth0)<threshold)
    {
        if(abs(origin-depth1)<threshold)
        {
            if(abs(origin-diagonal)<threshold)
            {
                gTexCoord=(originTexCoord+neightbour0TexCoord+neightbour1TexCoord+diagonalTexCoord)*aQuarter;
                return(origin+depth0+depth1+diagonal)*aQuarter;
                //return(mix(origin,mix(depth0,mix(depth1,diagonal,aHalf),aHalf),aHalf));
            }

            gTexCoord=(originTexCoord+neightbour0TexCoord+neightbour1TexCoord)*aThird;
            return(origin+depth0+depth1)*aThird;
        }

        if(abs(origin-diagonal)<threshold)
        {
            gTexCoord=(originTexCoord+neightbour0TexCoord+diagonalTexCoord)*aThird;
            return(origin+depth0+diagonal)*aThird;
        }

        gTexCoord=(originTexCoord+neightbour0TexCoord)*aHalf;
        return(origin+depth0)*aHalf;
        //return(mix(origin,depth0,aHalf));

    }

    if(abs(origin-depth1)<threshold)
    {
        if(abs(origin-diagonal)<threshold)
        {
            gTexCoord=(originTexCoord+neightbour1TexCoord+diagonalTexCoord)*aThird;
            return(origin+depth1+diagonal)*aThird;
        }

        gTexCoord=(originTexCoord+neightbour1TexCoord)*aHalf;
        return(origin+depth1)*aHalf;
        //return(mix(origin,depth1,aHalf));
    }

    gTexCoord=originTexCoord;
    return origin;
}

void calculatePosition(float depth, vec3 depthDir)
{
    //clamping the depth to avoid problems with depth in the infinity
    depth=clamp(depth,1.0f,background);

    //apply  depth offset to the position of the vertex
    gl_Position.xyz=eye+depth*depthDir;

    //Projection
    gl_Position = viewProj*gl_Position;

}

void main()
{


    //Tex coordinates of texture in all the axis. MINMAX to make the out of border quads take the tex coord of the edge
    float xTexOriginal=max(0.0f,min(vTexCoord[0].x,gridSize));
    float xTexRight=max(0.0f,min(vTexCoord[0].x+1.0f,gridSize));
    float xTexLeft=max(0.0f,min(vTexCoord[0].x-1.0f,gridSize));
    float yTexOriginal=max(0.0f,min(vTexCoord[0].y,gridSize));
    float yTexTop=max(0.0f,min(vTexCoord[0].y+1.0f,gridSize));
    float yTexBottom=max(0.0f,min(vTexCoord[0].y-1.0f,gridSize));

    //tex coordinates of every neighbour normalized
    vec2 originTexCoor=vec2(xTexOriginal,yTexOriginal)*screenSizeInverse;
    vec2 rightTexCoor=vec2(xTexRight,yTexOriginal)*screenSizeInverse;
    vec2 leftTexCoor=vec2(xTexLeft,yTexOriginal)*screenSizeInverse;
    vec2 topTexCoor=vec2(xTexOriginal,yTexTop)*screenSizeInverse;
    vec2 bottomTexCoor=vec2(xTexOriginal,yTexBottom)*screenSizeInverse;
    vec2 diagonalTRTexCoor=vec2(xTexRight,yTexTop)*screenSizeInverse;
    vec2 diagonalTLTexCoor=vec2(xTexLeft,yTexTop)*screenSizeInverse;
    vec2 diagonalBRTexCoor=vec2(xTexRight,yTexBottom)*screenSizeInverse;
    vec2 diagonalBLTexCoor=vec2(xTexLeft,yTexBottom)*screenSizeInverse;

    //depth values of every neighbour of the pixel
    float originDepth = texture2D(depthTex,originTexCoor).r;
    float rightDepth = texture2D(depthTex,rightTexCoor).r;
    float leftDepth = texture2D(depthTex,leftTexCoor).r;
    float topDepth = texture2D(depthTex,topTexCoor).r;
    float bottomDepth = texture2D(depthTex,bottomTexCoor).r;
    float diagonalTRDepth = texture2D(depthTex,diagonalTRTexCoor).r;
    float diagonalTLDepth = texture2D(depthTex,diagonalTLTexCoor).r;
    float diagonalBRDepth = texture2D(depthTex,diagonalBRTexCoor).r;
    float diagonalBLDepth = texture2D(depthTex,diagonalBLTexCoor).r;


    ///////////vertex 1. Top Right///////////

    //Calculate the depth of the vertex
    float depth1=calculateNeighboursAverage(originDepth,rightDepth,topDepth,diagonalTRDepth,originTexCoor,rightTexCoor,topTexCoor,diagonalTRTexCoor);

    //original position of the vertex
    gl_Position = gl_in[0].gl_Position;
    //gl_Position.xyz+=gridRight+gridUp;
    gl_Position.xyz+=gridRight*quadFactor+gridUp*quadFactor;

    //direction of the translation in depth
    vec3 depthDir1=normalize(gl_Position.xyz-eye);

    //Calculate the position of the vertex
    calculatePosition(depth1,depthDir1);

    //OUT
    gColor = vColor[0];


    EmitVertex();

    ///////////vertex 2. Top Left///////////

    //Calculate the depth of the vertex
    float depth2=calculateNeighboursAverage(originDepth,leftDepth,topDepth,diagonalTLDepth,originTexCoor,leftTexCoor,topTexCoor,diagonalTLTexCoor);

    //original position of the vertex
    gl_Position = gl_in[0].gl_Position;
    //gl_Position.xyz+=-gridRight+gridUp;
    gl_Position.xyz+=-gridRight*quadFactor+gridUp*quadFactor;

    //direction of the translation in depth
    vec3 depthDir2= normalize(gl_Position.xyz-eye);

    //Calculate the position of the vertex
    calculatePosition(depth2,depthDir2);

    //OUT
    gColor = vColor[0];


    EmitVertex();

    ///////////vertex 3. Bottom Right///////////


    //Calculate the depth of the vertex
    float depth3=calculateNeighboursAverage(originDepth,rightDepth,bottomDepth,diagonalBRDepth,originTexCoor,rightTexCoor,bottomTexCoor,diagonalBRTexCoor);

    //original position of the vertex
    gl_Position = gl_in[0].gl_Position;
    //gl_Position.xyz+=gridRight-gridUp;
    gl_Position.xyz+=gridRight*quadFactor-gridUp*quadFactor;


    //direction of the translation in depth
    vec3 depthDir3= normalize(gl_Position.xyz-eye);

    //Calculate the position of the vertex
    calculatePosition(depth3,depthDir3);

    //OUT
    gColor = vColor[0];


    EmitVertex();

    ///////////vertex 4. Bottom Left///////////


    //depth of the vertex
    float depth4=calculateNeighboursAverage(originDepth,leftDepth,bottomDepth,diagonalBLDepth,originTexCoor,leftTexCoor,bottomTexCoor,diagonalBLTexCoor);

    //original position of the vertex
    gl_Position = gl_in[0].gl_Position;
    //gl_Position.xyz+=-gridRight-gridUp;
    gl_Position.xyz+=-gridRight*quadFactor-gridUp*quadFactor;


    //direction of the translation in depth
    vec3 depthDir4= normalize(gl_Position.xyz-eye);

    //Calculate the position of the vertex
    calculatePosition(depth4,depthDir4);

    //OUT
    gColor = vColor[0];

    EmitVertex();



EndPrimitive();
}
