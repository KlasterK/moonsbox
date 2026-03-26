"use strict";

var Module;

class Brush {
    constructor(canvas) {
        this.material = 'Sand';
        this.thickness = 1;
        this.isCircular = true;
        this.canvas = canvas;

        this.mousePreviousPos = {x: 0, y: 0};
        this.isMouseHolded = false;
        this.activeTouches = new Map();

        canvas.addEventListener('mousedown',   e => this.onMousePressed(e));
        document.addEventListener('mousemove', e => this.onMouseMoved(e));
        document.addEventListener('mouseup',  () => this.onMouseReleased());

        canvas.addEventListener('touchstart',    e => this.onTouchStarted(e), {passive: true});
        document.addEventListener('touchmove',   e => this.onTouchMoved(e),   {passive: true});
        document.addEventListener('touchend',    e => this.onTouchStopped(e), {passive: true});
        document.addEventListener('touchcancel', e => this.onTouchStopped(e), {passive: true});
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
            
    onMousePressed(e) {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        this.isMouseHolded = true;
        this.mousePreviousPos = this.mouseToMapCoords(e.clientX, e.clientY);
        this.updateDrawing();
    }
            
    onMouseMoved(e) {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        if(!this.isMouseHolded) {
            return;
        }

        let point = this.mouseToMapCoords(e.clientX, e.clientY);
        Module.ccall(
            'draw_line_non_destructive', 'boolean', 
            ['number', 'number', 'number', 'number', 'number', 'string', 'number'], 
            [
                this.mousePreviousPos.x,
                this.mousePreviousPos.y,
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
        this.mousePreviousPos = point;
    }

    onMouseReleased() {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        this.isMouseHolded = false;
    }

    onTouchStarted(e) {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        for(let touch of e.changedTouches) {
            this.activeTouches.set(touch.identifier, {
                previousPos: this.mouseToMapCoords(touch.clientX, touch.clientY),
            });
        }

        this.updateDrawing();
    }

    onTouchMoved(e) {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        if(this.activeTouches.size > 1) {
            return;
        }

        for(let touch of e.changedTouches) {
            if(!this.activeTouches.has(touch.identifier)) {
                continue;
            }

            let point = this.mouseToMapCoords(touch.clientX, touch.clientY);
            Module.ccall(
                'draw_line_non_destructive', 'boolean', 
                ['number', 'number', 'number', 'number', 'number', 'string', 'number'], 
                [
                    this.activeTouches.get(touch.identifier).previousPos.x,
                    this.activeTouches.get(touch.identifier).previousPos.y,
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
            this.activeTouches.get(touch.identifier).previousPos = point;
        }
    }

    onTouchStopped(e) {
        if(this.externOnActivationCB != null) {
            this.externOnActivationCB();
        }

        for (let touch of e.changedTouches) {
            this.activeTouches.delete(touch.identifier);
        }
    }

    updateDrawing() {
        let points = [];
        if(this.isMouseHolded) {
            points.push(this.mousePreviousPos);
        }

        if(this.activeTouches.size !== 2) {
            for(let touch of this.activeTouches.values()) {
                points.push(touch.previousPos);
            }
        }

        for(let point of points) {
            Module.ccall(
                'draw_rect_or_ellipse_non_destructive', 'boolean', 
                ['number', 'number', 'number', 'number', 'boolean', 'string'], 
                [
                    point.x - this.thickness / 2,
                    point.y - this.thickness / 2,
                    this.thickness,
                    this.thickness,
                    this.isCircular,
                    this.material,
                ],
            );
            Module.ccall(
                'play_place_sound', 'boolean', 
                ['number', 'number', 'string'], 
                [point.x, point.y, this.material]
            );
        }
    }

    setOnActivationCB(cb) {
        this.externOnActivationCB = cb;
    }
}