var ws = null;

function sendMsg() {
    ws.send(JSON.stringify({
        app: "root",
        data: document.getElementById("message").value
      }));
}

function isJsonString(str) {
    try {
        JSON.parse(str);
    } catch (e) {
        return false;
    }
    return true;
}

function beginSocket() {
    ws = new WebSocket('ws://' + document.location.host + '/ws');

    ws.onmessage = function(e){
        console.log("Server returned: " + e.data);
        var json = JSON.parse(e.data);

        // if ("to" in json) {
        //     console.log("Typed: " + json[json.to]);
        // }
        switch (json.type) {
            case "keyboardInput":
                console.log("keyboardInput: " + json.data);
        }
    }        
}

function sendSSID() {
    console.log("SSID: " + document.getElementById("SSID").value);
    ws.send(JSON.stringify({
        app: "config",
        type: "s",
        data: document.getElementById("SSID").value
      }));
}

function sendPassword() {
    console.log("Password: " + document.getElementById("SSID").value);
    ws.send(JSON.stringify({
        app: "config",
        type: "p",
        data: document.getElementById("Password").value
      }));
}