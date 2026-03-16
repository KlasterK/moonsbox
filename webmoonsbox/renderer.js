"use strict";

// Vertex shader: passes position and texture coordinates
const vertexShaderText = `
    attribute vec4 a_position;
    attribute vec2 a_texCoord;
    varying vec2 v_texCoord;
    
    void main() {
        gl_Position = a_position;
        v_texCoord = a_texCoord; // forward to fragment shader
    }
`;

// Fragment shader: samples texture and converts pixel format
const fragmentShaderText = `
    precision mediump float;
    uniform sampler2D u_image;      // texture containing pixels (from WASM)
    varying vec2 v_texCoord;        // interpolated texture coordinates

    void main() {
        // Flip vertically and convert color from little-endian RGBA to big-endian RGBA
        gl_FragColor = texture2D(u_image, vec2(v_texCoord.x, 1.0 - v_texCoord.y)).abgr;
    }
`;

function compileShader(gl, text, type) {
    const shader = gl.createShader(type);
    gl.shaderSource(shader, text);
    gl.compileShader(shader);
    if (!gl.getShaderParameter(shader, gl.COMPILE_STATUS)) {
        console.error(gl.getShaderInfoLog(shader));
        gl.deleteShader(shader);
        return null;
    }
    return shader;
}

function initShaders(gl) {
    const vertexShader = compileShader(gl, vertexShaderText, gl.VERTEX_SHADER);
    const fragmentShader = compileShader(gl, fragmentShaderText, gl.FRAGMENT_SHADER);

    let shaderProgram = gl.createProgram();
    gl.attachShader(shaderProgram, vertexShader);
    gl.attachShader(shaderProgram, fragmentShader);
    gl.linkProgram(shaderProgram);

    if(!gl.getProgramParameter(shaderProgram, gl.LINK_STATUS)) {
        console.error(gl.getProgramInfoLog(shaderProgram));
        gl.deleteProgram(shaderProgram);
        return null;
    }

    return shaderProgram;
}

function createVertexBuffer(gl) {
    // Vertex data for a full‑screen quad (x, y, u, v)
    const vertices = new Float32Array([
        -1, -1, 0, 1,  // bottom left
         1, -1, 1, 1,  // bottom right
        -1,  1, 0, 0,  // top left
         1,  1, 1, 0   // top right
    ]);

    const buffer = gl.createBuffer();
    gl.bindBuffer(gl.ARRAY_BUFFER, buffer);
    gl.bufferData(gl.ARRAY_BUFFER, vertices, gl.STATIC_DRAW);
    return buffer;
}

class RendererGL {
    constructor(gl, width, height) {
        this.width = width;
        this.height = height;

        this.program = initShaders(gl);
        gl.useProgram(this.program);
        
        const vertexBuffer = createVertexBuffer(gl);

        // Attribute locations
        const posLoc = gl.getAttribLocation(this.program, 'a_position');
        const texLoc = gl.getAttribLocation(this.program, 'a_texCoord');
        const imageLoc = gl.getUniformLocation(this.program, 'u_image');

        // Setup vertex attributes (interleaved: x, y, u, v)
        const stride = 4 * 4; // 4 floats * 4 bytes each
        gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);

        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 2, gl.FLOAT, false, stride, 0);

        gl.enableVertexAttribArray(texLoc);
        gl.vertexAttribPointer(texLoc, 2, gl.FLOAT, false, stride, 8); // offset to u,v

        // Create and configure texture
        this.texture = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);

        // Bind texture unit 0 to the sampler
        gl.uniform1i(imageLoc, 0);
    }

    render(gl, pixels) {
        // Upload new pixel data to the texture
        gl.bindTexture(gl.TEXTURE_2D, this.texture);
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.RGBA,
            this.width, this.height, 0,
            gl.RGBA, gl.UNSIGNED_BYTE, pixels
        );

        // Draw the quad (4 vertices as a triangle strip)
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    }
}
