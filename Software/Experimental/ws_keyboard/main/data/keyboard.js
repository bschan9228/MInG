var ws = null;

function sendMsg(keypress) {
    ws.send(JSON.stringify({
        app: "keyboard",
        data: keypress
      }));
}

function beginSocket() {
    ws = new WebSocket('ws://' + document.location.host + '/ws');    
}

function getKey (e) {
    var location = e.location;
    var selector;
    if (location === KeyboardEvent.DOM_KEY_LOCATION_RIGHT) {
        selector = ['[data-key="' + e.keyCode + '-R"]']
    } else {
        var code = e.keyCode || e.which;
        selector = [
            '[data-key="' + code + '"]',
            '[data-char*="' + encodeURIComponent(String.fromCharCode(code)) + '"]'
            
        ].join(',');
        // console.log("code: " + String.fromCharCode(code));
    }
    return document.querySelector(selector);
}

function pressKey (char) {
    var key = document.querySelector('[data-char*="' + char.toUpperCase() + '"]');
    if (!key) {
        return console.warn('No key for', char);
    }
    key.setAttribute('data-pressed', 'on');
    setTimeout(function () {
        key.removeAttribute('data-pressed');
    }, 200);
}

var h1 = document.querySelector('h1');
var originalQueue = h1.innerHTML;
var queue = h1.innerHTML;

function next () {
    var c = queue[0];
    queue = queue.slice(1);
    h1.innerHTML = originalQueue.slice(0, originalQueue.length - queue.length);
    pressKey(c);
    if (queue.length) {
        setTimeout(next, Math.random() * 200 + 50);
    }
}

h1.innerHTML = "&nbsp;";
setTimeout(next, 500);

document.body.addEventListener('keydown', function (e) {
    var key = getKey(e);
    if (!key) {
        return console.warn('No key for', e.keyCode);
    }
    // console.log("clic" + key);
    // console.log("code: " + String.fromCharCode(e.keyCode || e.which));
    console.log("code: " + e.keyCode || e.which);
    sendMsg(e.keyCode || e.which);
    key.setAttribute('data-pressed', 'on');
});

document.body.addEventListener('keyup', function (e) {
    var key = getKey(e);
    key && key.removeAttribute('data-pressed');
});

// Not sure how to make touch work
document.querySelector('body').addEventListener('click', e => {
    console.log('Tapped:', e);
  })

function size () {
    var size = keyboard.parentNode.clientWidth / 90;
    keyboard.style.fontSize = size + 'px';
    console.log(size);
}

var keyboard = document.querySelector('.keyboard');
window.addEventListener('resize', function (e) {
    size();
});
size();