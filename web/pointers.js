
//// Mouse/Touchscreen
function Pointer() {
  this.id = -1;
  this.pos = [0, 0];
  this.delta = [0, 0];
  this.deltaY = 0;
  this.isDown = false;
}
var pointers = [new Pointer()];
var canvas = document.getElementsByTagName("canvas")[0];

function updatePointer(pointer, pos, isDown, isDelta) {
  var lastPos = pointer.pos;
  pointer.pos = [Math.floor(pos[0] * window.devicePixelRatio) / canvas.width,
                 1.0 - Math.floor(pos[1] * window.devicePixelRatio) / canvas.height];
  pointer.isDown = isDown;
  if (isDelta) {
    pointer.delta = [pointer.pos[0] - lastPos[0], pointer.pos[1] - lastPos[1]];
  } else {
    pointer.delta = [0.0, 0.0];
  }
}

canvas.addEventListener('mousedown', e => {
  let p = pointers.find(p => p.id == -1); 
  updatePointer(p, [e.offsetX, e.offsetY], true, false);
});
canvas.addEventListener('mousemove', e => {
  let p = pointers.find(p => p.id == -1); 
  if (!p.isDown)
    return;
  updatePointer(p, [e.offsetX, e.offsetY], true, true);
});
window.addEventListener('mouseup', () => {
  let p = pointers.find(p => p.id == -1);
  p.isDown = false;
});

canvas.addEventListener('touchstart', e => {
  e.preventDefault();
  const touches = e.targetTouches;
  while (touches.length >= pointers.length)
    pointers.push(new Pointer());
  for (let i = 0; i < touches.length; i++) {
    pointers[i+1].id = touches[i].identifier;
    updatePointer(pointers[i+1], [touches[i].pageX, touches[i].pageY], true, false);
  }
});
canvas.addEventListener('touchmove', e => {
  e.preventDefault();
  const touches = e.targetTouches;
  for (let i = 0; i < touches.length; i++) {
    let p = pointers[i+1];
    if (!p.isDown) continue;
    updatePointer(p, [touches[i].pageX, touches[i].pageY], true, true);
  }
}, false);
window.addEventListener('touchend', e => {
  const touches = e.changedTouches;
  for (let i = 0; i < touches.length; i++) {
    let p = pointers.find(p => p.id == touches[i].identifier);
    if (p == null) continue;
    p.isDown = false;
  }
});

module.exports = {Pointer: Pointer, pointers: pointers};