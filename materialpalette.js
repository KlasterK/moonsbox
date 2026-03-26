'use strict';

var buffers;

function createMaterialCard(materialName, brush) {
    const card = document.createElement('div');
    card.className = 'material-palette-card';

    const img = document.createElement('img');
    img.className = 'material-palette-icon';
    img.loading = 'lazy';
    img.src = `assets/materials/${materialName}`;
    card.appendChild(img);

    const label = document.createElement('div');
    label.className = 'material-palette-label';
    label.textContent = materialName;
    card.appendChild(label);

    card.addEventListener('click', () => {
        for (let el of document.getElementsByClassName('material-palette-card')) {
            el.classList.remove('material-palette-selected');
        }
        card.classList.add('material-palette-selected');
        brush.material = materialName;
    });

    return card;
}

function initMaterialPalette(brush) {
    const grid = document.getElementById('material-palette-grid');
    if (!grid) {
        console.error('Cannot find element #material-palette-grid');
        return;
    }
    grid.innerHTML = '';

    if (!buffers || !buffers.material_names || !buffers.material_names.length) {
        console.warn('Cannot find any materials. Grid will be empty.');
        return;
    }

    for (const name of buffers.material_names) {
        const card = createMaterialCard(name, brush);
        grid.appendChild(card);

        if(brush.material == name) {
            card.classList.add('material-palette-selected');
        }
    }
}
