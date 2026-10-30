// Ordinary world pixel stages from docs/specs/steam-world-shader-output.md.
// Geometry RGB arrives lit and clamped by the vertex stage. Opacity stays separate.
vec3 steamWorldPixel(vec3 colour, vec2 screen, vec3 viewPosition, vec2 viewNormal) {
    float Y = screen.y + f.steamSky[5].x;
    if (pc.look.z > 0)
        colour *= 1 + .3 * cos(5 * screen.x + 10 * Y + 2 * (viewNormal.x + viewNormal.y));
    colour = vanillaBoost(colour);

    float distance = f.steamSky[5].z > .5 ? min(f.fog.w, 150) : f.fog.w;
    float scale = 1 / distance;
    float q = dot(viewPosition, viewPosition) * (scale * scale);
    float exponent = 1 - .75 * exp2(-403.9546203613281 * scale);
    float t = min(exp2(log2(q) * exponent), 1);
    float haze = 3 * t * t - 2 * t * t * t;
    vec3 sky = steamSunGlow(steamSkyAt(screen), screen);
    colour = mix(colour, f.steamSky[2].rgb, haze);
    float haze2 = haze * haze;
    colour = clamp(mix(colour, sky, haze2 * haze2), 0, 1);

    float grey = dot(LUMA, colour);
    colour.gb = .65 * colour.gb + .35 * grey;
    vec3 square = colour * colour;
    return 3 * square - 2 * (colour * square);
}
