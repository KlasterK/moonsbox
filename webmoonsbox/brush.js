"use strict";

var Module;

class Brush {
    constructor(canvas) {
        this.material = 'Sand';
        this.isHolded = false;
        this.thickness = 1;
        this.previousPos = {x: 0, y: 0};
        this.isCircular = true;
        this.canvas = canvas;

        document.addEventListener('mousedown', e => this.onPressed(e.clientX, e.clientY));
        document.addEventListener('mousemove', e => this.onMoved(e.clientX, e.clientY));
        document.addEventListener('mouseup', () => this.onReleased());
    }

    mouseToMapCoords(clientX, clientY) {
        const rect = this.canvas.getBoundingClientRect();
        const canvasX = clientX - rect.left;
        const canvasY = rect.height - (clientY - rect.top);
        
        const scaleMapPerOnscreenX = this.canvas.width  / rect.width;
        const scaleMapPerOnscreenY = this.canvas.height / rect.height;

        return {
            x: Math.ceil(canvasX * scaleMapPerOnscreenX),
            y: Math.ceil(canvasY * scaleMapPerOnscreenY),
        };
    }
            
    onPressed(clientX, clientY) {
        if(this.externOnActivationCB != null)
            this.externOnActivationCB();

        this.isHolded = true;
        this.previousPos = this.mouseToMapCoords(clientX, clientY);
        this.updateDrawing();
    }
            
    onMoved(clientX, clientY) {
        if(!this.isHolded)
            return;

        let point = this.mouseToMapCoords(clientX, clientY);
        Module.ccall(
            'draw_line', 'boolean', 
            ['number', 'number', 'number', 'number', 'number', 'string', 'number'], 
            [
                this.previousPos.x,
                this.previousPos.y,
                point.x,
                point.y,
                this.thickness,
                this.material,
                0, // drawing::LineEnds::None
            ],
        );
        Module.ccall(
            'play_place_sound', 'boolean', 
            ['number', 'number', 'string'], 
            [point.x, point.y, this.material]
        );
        this.previousPos = point;
    }

    onReleased() {
        if(this.externOnActivationCB != null)
            this.externOnActivationCB();

        this.isHolded = false;
    }

    updateDrawing() {
        if(!this.isHolded)
            return;

        Module.ccall(
            'draw_rect_or_ellipse', 'boolean', 
            ['number', 'number', 'number', 'number', 'boolean', 'string'], 
            [
                this.previousPos.x - this.thickness / 2,
                this.previousPos.y - this.thickness / 2,
                this.thickness,
                this.thickness,
                this.isCircular,
                this.material,
            ],
        );
        Module.ccall(
            'play_place_sound', 'boolean', 
            ['number', 'number', 'string'], 
            [this.previousPos.x, this.previousPos.y, this.material]
        );
    }

    setOnActivationCB(cb) {
        this.externOnActivationCB = cb;
    }
}