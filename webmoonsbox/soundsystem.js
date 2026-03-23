"use strict";

const soundsToLoad = {
    "material.Sand": [
        "../assets/sounds/material.Sand.1.wav",
        "../assets/sounds/material.Sand.3.wav",
    ],
    "material.Water": [
        "../assets/sounds/material.Water.1.wav",
        "../assets/sounds/material.Water.2.wav",
        "../assets/sounds/material.Water.3.wav",
        "../assets/sounds/material.Water.4.wav",
    ],
    "material.Ice": [
        "../assets/sounds/material.Ice.wav",
    ],
    "material.Steam": [
        "../assets/sounds/material.Steam.wav",
    ],
    "material.Lava": [
        "../assets/sounds/material.Lava.wav",
    ],
    "material.Fire": [
        "../assets/sounds/material.Fire.wav",
    ],
    "convert.Ice_melts": [
        "../assets/sounds/convert.Ice_melts.wav",
    ],
    "convert.Water_freezes": [
        "../assets/sounds/convert.Water_freezes.wav",
    ],
    "convert.Water_evaporates": [
        "../assets/sounds/convert.Water_evaporates.wav",
    ],
    "convert.Steam_condensates": [
        "../assets/sounds/convert.Steam_condensates.wav",
    ],
    "convert.Sand_to_glass": [
        "../assets/sounds/convert.Sand_to_glass.wav",
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
