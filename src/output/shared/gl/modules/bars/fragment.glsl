uniform vec4 foreground_color;
uniform vec4 background_color;
uniform float gradient_sections;
uniform vec4 gradient_color[8];
uniform vec2 resolution;
uniform float intensity;

void main() {
    if (gradient_sections > 0.0) {
        float across = (gl_FragCoord.y / resolution.y) * gradient_sections;
        int section = int(floor(across));
        float off = mod(across, 1.0);

        // Use the built-in gl_FragColor instead of defining a custom FragColor
        gl_FragColor = mix(gradient_color[section], gradient_color[section+1], off);
    } else {
        gl_FragColor = foreground_color;
    }
}
