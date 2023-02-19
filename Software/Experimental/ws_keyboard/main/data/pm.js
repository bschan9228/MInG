// Websockets
var ws = null;

function sendUser(user) {
    console.log("sendUser data: " + user);
    ws.send(JSON.stringify({
        app: "pm",
        type: "user",
        data: user
      }));
}

function sendPass(pass) {
    console.log("sendPass data: " + data);
    ws.send(JSON.stringify({
        app: "pm",
        type: "pass",
        data: user
      }));
}



function beginSocket() {
    ws = new WebSocket('ws://' + document.location.host + '/ws');    
}

// Sorting
function search() {
  // Declare variables
  var input, filter, table, tr, td, i, txtValue;
  input = document.getElementById("myInput");
  filter = input.value.toUpperCase();
  table = document.getElementById("myTable");
  tr = table.getElementsByTagName("tr");

  // Loop through all table rows, and hide those who don't match the search query
  for (i = 0; i < tr.length; i++) {
    td = tr[i].getElementsByTagName("td")[0];
    if (td) {
      txtValue = td.textContent || td.innerText;
      if (txtValue.toUpperCase().indexOf(filter) > -1) {
        tr[i].style.display = "";
      } else {
        tr[i].style.display = "none";
      }
    }
  }
}