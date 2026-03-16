"use strict";

const soundsToLoad = {
    "material.Sand": [
        "../assets/sounds/material.Sand.1.wav",
        "../assets/sounds/material.Sand.3.wav",
    ],
};

class SoundSystem {
    constructor() {
        this.audioCtx = new (window.AudioContext || window.webkitAudioContext)();
        this.tracksCache = new Map();
        this.currentSounds = new Map();
        this.currentCategories = new Map();
    }

    async loadSounds() {
        for(let [soundName, tracksURLs] of Object.entries(soundsToLoad)) {
            let tracks = [];
            for(let trackURL of tracksURLs) {
                const response = await fetch(trackURL);
                const arrayBuffer = await response.arrayBuffer();
                const audioBuffer = await this.audioCtx.decodeAudioData(arrayBuffer);
                tracks.push(audioBuffer);
            }
            
            this.tracksCache.set(soundName, tracks);
        }
    }

    playSoundCB(name, category, doOverride) {
        name = Module.UTF8ToString(name);
        category = Module.UTF8ToString(category);

        if(!this.tracksCache.has(name)) {
            console.warn('No sound for', name);
            return;
        }

        const mapping = category ? this.currentCategories : this.currentSounds;
        const key = category ? category : name;

        if(mapping.has(key) && !doOverride) {
            return;
        }

        const idx = Math.floor(Math.random() * this.tracksCache.get(name).length);

        const source = this.audioCtx.createBufferSource();
        source.buffer = this.tracksCache.get(name)[idx];
        source.connect(this.audioCtx.destination);
        source.start();
        source.onended = () => {
            source.disconnect();
            mapping.delete(key);
        };
        mapping.set(key, source);
    }
};
