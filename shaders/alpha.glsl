// Cube World Alpha's shading, shared by the Alpha shaders. Written from
// docs/specs/alpha-rendering.md; the section numbers point there. Needs common.glsl.

float saturate(float x) { return clamp(x, 0, 1); }
// 3x^2 - 2x^3 on an already saturated x.
float smoothCurve(float x) { return x * x * 3 - x * x * x * 2; }

// §6: the sky gradient for a screen height Y (+1 at the top, -1 at the bottom, shifted with
// the camera's tilt): fog and the horizon colour low down, the zenith colour high up.
vec3 alphaSkyAt(float Y) {
    vec3 top = mix(f.alphaSky1.rgb, f.alphaSky2.rgb, smoothCurve(saturate(.6 * Y)));
    vec3 bottom = .2 * f.alphaSky1.rgb + .8 * f.alphaFog.rgb;
    return mix(bottom, top, smoothCurve(saturate(2 * Y + 1)));
}

