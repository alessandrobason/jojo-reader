@ctype mat4 mat4

@vs vs
in vec2 position;
in vec2 texc;
in vec4 colour;

out vec2 fs_texc;
out vec4 fs_colour;

uniform vs_params { 
    mat4 transform; 
};

void main() {
    gl_Position = transform * vec4(position, 0.0, 1.0);
    fs_texc = texc;
    fs_colour = colour;
}
@end

@fs fs
in vec2 fs_texc;
in vec4 fs_colour;

out vec4 frag_colour;

uniform sampler2D tex;

void main() {
    frag_colour = texture(tex, fs_texc) * fs_colour;
    if (frag_colour.a == 0) discard;
}

@end

@program base vs fs