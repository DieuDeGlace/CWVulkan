// The Steam look's sky (docs/specs/sky-and-clouds.md §B.5): the screen-space gradient and the
// sun's glow, shared by the sky, the world's fog and the clouds. Fragment shaders only (it reads
// gl_FragCoord). Needs common.glsl. The constants are f.steamSky: sky1, sky2, fog, the sun
// colour, the sun position (x, y, width / height, visible) and the gradient shift in [5].x.

// 3x^2 - 2x^3 of x clamped to 0..1.
float steamSmooth(float x) {
    x = clamp(x, 0, 1);
    return x * x * (3 - 2 * x);
}

// This pixel's screen position: x right, y up, -1..1.
vec2 steamScreen() { return vec2(gl_FragCoord.x * f.alphaView.y - 1, 1 - gl_FragCoord.y * f.alphaView.z); }

// Step 2: the gradient at a screen position, without the sun.
vec3 steamSkyAt(vec2 screen) {
    float Y = screen.y + f.steamSky[5].x;
    vec3 top = mix(f.steamSky[0].rgb, f.steamSky[1].rgb, steamSmooth(.4 * Y + .2));
    vec3 bottom = .4 * f.steamSky[0].rgb + .6 * f.steamSky[2].rgb;
    return mix(bottom, top, steamSmooth(2 * Y + 1.5));
}

// Step 3: the sun's glow over a sky colour: pure sun colour at its centre, gone half a screen
// height away.
vec3 steamSunGlow(vec3 colour, vec2 screen) {
    float Y = screen.y + f.steamSky[5].x;
    vec4 sun = f.steamSky[4];
    vec2 d = vec2((sun.x - screen.x) * sun.z, sun.y - Y);
    float q = min(4 * dot(d, d), 1);
    float g = (1 - q) * sun.w;
    g *= g;
    return mix(f.steamSky[3].rgb, colour, 1 - g * g);
}
