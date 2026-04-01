'use strict';

var defaultSize = window.innerWidth > window.innerHeight ? {x: 240, y: 135} : {x: 200, y: 200};
document.getElementById('brush-size-slider').value = 1;
document.getElementById('reset-form-width').value  = defaultSize.x;
document.getElementById('reset-form-height').value = defaultSize.y;
document.getElementById('saving-file-input').value = '';

var soundSystem = new SoundSystem();
soundSystem.loadSounds();

var canvas = document.getElementById('game-surface');

const defaultSizeAspectRatio = defaultSize.x / defaultSize.y;
let adaptiveCanvasWidth = Math.min(
    canvas.clientWidth,
    window.innerHeight * 0.5,
    window.innerHeight - document.getElementById('all-controls-div').clientHeight * 2,
);

canvas.style.width = `${defaultSizeAspectRatio * adaptiveCanvasWidth}px`;
canvas.style.height = 'auto';

const gl = canvas.getContext('webgl2');
if(gl == null) {
    console.error('Cannot get WebGL 2 context.');
}
const rendererGL = new RendererGL(gl);

var brush = new Brush(canvas);
brush.setOnActivationCB(() => soundSystem.audioCtx.resume());

var framesCount = 0;
var buffers;

let isInited = false;
let u8colors;
let f32temps;
let currentSize = defaultSize;
let currentVisionMode = VisionMode.Normal;

function initGame(size, doReinit = true) {
    canvas.width = size.x;
    canvas.height = size.y;
    canvas.style.aspectRatio = `${size.x} / ${size.y}`;

    gl.viewport(0, 0, size.x, size.y);

    buffers = JSON.parse(Module.UTF8ToString(Module._init_game(size.x, size.y, doReinit)));
    u8colors = new Uint8Array(wasmMemory.buffer, buffers.colors_ptr, size.x * size.y * 4);
    f32temps = new Float32Array(wasmMemory.buffer, buffers.temps_ptr, size.x * size.y);

    initMaterialPalette(brush);
    let playSoundCBID = Module.addFunction(soundSystem.playSoundCB.bind(soundSystem), 'vppi');
    Module._register_play_sound_callback(playSoundCBID);

    isInited = true;
    currentSize = size;
}

function tickRender() {
    if(isInited) {
        Module._tick(true, false, false);
        rendererGL.render(gl, u8colors, f32temps, currentVisionMode, currentSize);
        brush.updateDrawing();
    }
    ++framesCount;
    requestAnimationFrame(tickRender);
}
tickRender();

var Module = {
    onRuntimeInitialized: function() {
        initGame(defaultSize);
    },
};

const pauseBtn = document.getElementById('pause-btn');

function pauseGame() {
    const v = Module._tick(false, false, false);
    Module._tick(false, true, !v);

    pauseBtn.textContent = v ? 'Pause' : 'Unpause';
}

const resetFormWidth = document.getElementById('reset-form-width');
const resetFormHeight = document.getElementById('reset-form-height');

function resetMap() {
    const size = {
        x: parseInt(resetFormWidth.value),
        y: parseInt(resetFormHeight.value),
    };

    if(size.x == null || size.y == null) {
        window.alert('adsa');
        return;
    }

    initGame(size);
}

function formatSaveFileNameWithDate(date) {
    const zfill2 = (n, width = 2) => String(n).padStart(width, '0');
    
    const year    = date.getFullYear();
    const month   = zfill2(date.getMonth() + 1);
    const day     = zfill2(date.getDate());
    const hours   = zfill2(date.getHours());
    const minutes = zfill2(date.getMinutes());
    const seconds = zfill2(date.getSeconds());
    
    return `moonsbox ${year}-${month}-${day} ${hours}-${minutes}-${seconds}.kk-save`;
}

let lastSavingBlobURL = null;
const savingLink = document.getElementById('saving-download-link');

function saveGame() {
    try {
        FS.unlink('/out.kk-save');
    } catch {}

    const error = Module.ccall('minizip_load_store', 'string', ['boolean', 'string'], [true, '/out.kk-save']);
    if(error.trim() != '') {
        window.alert(`Error when saving your game! ${error}`);
        return;
    }

    const zipData = FS.readFile('/out.kk-save');
    const blob = new Blob([zipData], {type: 'application/x-moonsbox-save'});

    if(lastSavingBlobURL != null) {
        URL.revokeObjectURL(lastSavingBlobURL);
    }
    lastSavingBlobURL = URL.createObjectURL(blob);

    const now = new Date();
    savingLink.textContent = `Download save file (from ${now.toLocaleString()})`;
    savingLink.href = lastSavingBlobURL;
    savingLink.download = formatSaveFileNameWithDate(now);
}

function onSavingFileInputChange(e, input) {
    const file = e.target.files[0];
    if(!file) {
        return;
    }

    try {
        FS.unlink('/out.kk-save');
    } catch {}

    const reader = new FileReader();
    reader.onload = (loadE) => {
        const arrayBuffer = loadE.target.result;
        const u8array = new Uint8Array(arrayBuffer);
        FS.writeFile('/in.kk-save', u8array);
    };

    reader.readAsArrayBuffer(file);
}

function loadGame() {
    if(!FS.analyzePath('/in.kk-save').exists) {
        window.alert("A save file wasn't loaded yet.");
        return;
    }

    const result = Module.ccall('minizip_load_store', 'string', ['boolean', 'string'], [false, '/in.kk-save']);
    if(result[0] === 'k') {
        const sep = result.indexOf(';');

        const width = parseInt(result.substring(1, sep));
        const height = parseInt(result.substring(sep + 1));

        if(width == null || height == null) {
            console.error('Cannot parse width and height from save loading result');
            return;
        }

        initGame({x: width, y: height}, false);
    } else if(result[0] === 'e') {
        window.alert(`Error when loading your game! ${result.substring(1)}`);
    } else {
        console.error('Invalid load error message', result);
    }
}

const currentVisionModeSpan = document.getElementById('current-vision-mode-span');
currentVisionModeSpan.textContent = 'Normal';

function switchVisionMode() {
    if(currentVisionMode == VisionMode.Normal) {
        currentVisionMode                 = VisionMode.Thermal;
        currentVisionModeSpan.textContent = 'Thermal';
    } else if(currentVisionMode == VisionMode.Thermal) {
        currentVisionMode                 = VisionMode.Normal;
        currentVisionModeSpan.textContent = 'Normal';
    }
}
