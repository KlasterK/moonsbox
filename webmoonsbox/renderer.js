"use strict";

// Vertex shader: passes position and texture coordinates
const vertexShaderText = `
    attribute vec4 a_position;
    attribute vec2 a_texCoord;
    varying vec2 v_texCoord;
    
    void main() {
        gl_Position = a_position;
        v_texCoord = a_texCoord;
    }
`;

// Fragment shader: samples texture and converts pixel format
const fragmentShaderText = `
    precision mediump float;
    uniform sampler2D u_colors; // ABGR8888
    uniform sampler2D u_temps;  // R32F
    uniform int u_visionMode;
    varying vec2 v_texCoord;    // interpolated texture coordinates

    void main() {
        vec2 texCoord = vec2(v_texCoord.x, 1.0 - v_texCoord.y);
        vec4 color = texture2D(u_colors, texCoord).abgr;

        if(u_visionMode == 0) {
            gl_FragColor = color;
        } else if(u_visionMode == 1) {
            // 50 % of grayscale
            float darkscale = (color.r + color.g + color.b) / 6.0;

            float temp = texture2D(u_temps, texCoord).r;
            float tempFactor = temp / 500.0;

            gl_FragColor = vec4(
                clamp(darkscale + tempFactor * 0.75,         0.0, 1.0),
                clamp(darkscale + (tempFactor - 1.0) * 0.25, 0.0, 1.0),
                darkscale,
                1.0
            );
        }
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

const VisionMode = {
    Normal:  0,
    Thermal: 1,
};

class RendererGL {
    constructor(gl) {
        this.program = initShaders(gl);
        gl.useProgram(this.program);
        
        const vertexBuffer = createVertexBuffer(gl);

        // Attribute locations
        const posLoc = gl.getAttribLocation(this.program, 'a_position');
        const texLoc = gl.getAttribLocation(this.program, 'a_texCoord');
        const colorsLoc = gl.getUniformLocation(this.program, 'u_colors');
        const tempsLoc = gl.getUniformLocation(this.program, 'u_temps');
        this.visionModeLoc = gl.getUniformLocation(this.program, 'u_visionMode');

        // Setup vertex attributes (interleaved: x, y, u, v)
        const stride = 4 * 4; // 4 floats * 4 bytes each
        gl.bindBuffer(gl.ARRAY_BUFFER, vertexBuffer);

        gl.enableVertexAttribArray(posLoc);
        gl.vertexAttribPointer(posLoc, 2, gl.FLOAT, false, stride, 0);

        gl.enableVertexAttribArray(texLoc);
        gl.vertexAttribPointer(texLoc, 2, gl.FLOAT, false, stride, 8); // offset to u,v

        this.colorsTex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.colorsTex);
        // scaling method for minimising
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        // scaling method for maximising
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        // how to wrap the texture horisontally
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        // vertically
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        // bind as gl.TEXTURE0
        gl.uniform1i(colorsLoc, 0);

        this.tempsTex = gl.createTexture();
        gl.bindTexture(gl.TEXTURE_2D, this.tempsTex);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.NEAREST);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
        gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
        gl.uniform1i(tempsLoc, 1); // bind as gl.TEXTURE1

        gl.enable(gl.BLEND);
        gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
    }

    render(gl, u8colors, f32temps, visionMode, mapSize) {
        gl.uniform1i(this.visionModeLoc, visionMode);

        gl.activeTexture(gl.TEXTURE0);
        gl.bindTexture(gl.TEXTURE_2D, this.colorsTex);
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.RGBA,
            mapSize.x, mapSize.y, 0,
            gl.RGBA, gl.UNSIGNED_BYTE, u8colors,
        );

        gl.activeTexture(gl.TEXTURE1);
        gl.bindTexture(gl.TEXTURE_2D, this.tempsTex);
        gl.texImage2D(
            gl.TEXTURE_2D, 0, gl.R32F,
            mapSize.x, mapSize.y, 0,
            gl.RED, gl.FLOAT, f32temps,
        );

        // Draw the quad (4 vertices as a triangle strip)
        gl.drawArrays(gl.TRIANGLE_STRIP, 0, 4);
    }
}
