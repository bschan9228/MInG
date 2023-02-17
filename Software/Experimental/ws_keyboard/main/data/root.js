var ws = null;

function sendMsg() {
    ws.send(buildMsg());
}

//FIX  failed: Could not decode a text frame as UTF-8.
function buildMsg() {
    message = document.getElementById("message").value;
    if (message != "") return message;

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
        // console.log(String(e.data).split(","));
        var scuffedJson = String(e.data).split(",");

        if (scuffedJson.pop() == "temperature") {
            updateTemperature(scuffedJson[0]);
        }
    }        
}
