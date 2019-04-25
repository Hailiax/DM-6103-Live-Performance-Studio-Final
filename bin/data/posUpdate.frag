#version 150

uniform sampler2DRect prevPosData;
uniform sampler2DRect origPosData;
uniform sampler2DRect velData;
uniform sampler2DRect ageData;
uniform sampler2DRect fractalData;

uniform int phase;
uniform vec2 screen;
uniform float timestep;
uniform float velocityScale;

in vec2 vTexCoord;

out vec4 vFragColor;

void main(void){
    // Get the position and velocity from the pixel color.
    vec2 pos = texture( prevPosData, vTexCoord ).rg;
    vec2 origPos = texture( origPosData, vTexCoord ).rg;
    vec2 vel = texture( velData, vTexCoord ).rg;
    float age = texture( ageData, vTexCoord ).r;
    
    // Update the position.
    if (age < timestep){
        pos = origPos;
    } else{
        
        vec2 fractal = vec2(1.0,1.0);
        
        if (pos.x > 0.0 && pos.x < 1.0 && pos.y > 0.0 && pos.y < 1.0){
            fractal = texture( fractalData, vec2(pos.x*screen.x,pos.y*screen.y) ).rg;
            
            if (phase == 1){ // For phase 2 do not engage this if statement and perhaps decrease life
                fractal += 1; // Maps 0-2
                fractal *= 0.5; // Maps 0-1
                fractal += 0.5; // Should be 1-above to center around 1. Now: 0.5-1.5
            }
        }
        
        pos += vel * fractal * timestep * velocityScale;
        
    }
    
    // And finally store it on the position FBO.
    vFragColor = vec4(pos.x,pos.y,1.0,1.0);
}
